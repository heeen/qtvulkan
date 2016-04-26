/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Ian Elliott <ian@LunarG.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 */

#define VK_USE_PLATFORM_XCB_KHR 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <QTimer>

#include <cube.h>
#include <QApplication>
#include <QMessageBox>
#include <QResizeEvent>

#include <QtMath>
#include <qpa/qplatformnativeinterface.h>

static PFN_vkGetDeviceProcAddr g_gdpa = NULL;

static const char *tex_files[] = {"lunarg.ppm"};

static int validation_error = 0;

struct vktexcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};

static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 0.0f,  // -X side
    1.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Z side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Y side
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // +Y side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,

    1.0f, 1.0f,  // +X side
    0.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 1.0f,  // +Z side
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
};
// clang-format on

VKAPI_ATTR VkBool32 VKAPI_CALL
dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char *pLayerPrefix, const char *pMsg, void *pUserData) {

    Q_UNUSED(objType)
    Q_UNUSED(srcObject)
    Q_UNUSED(location)
    Q_UNUSED(pUserData)

    QDebug q(qDebug());

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        q<<"ERROR";
        validation_error = 1;
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        q<<"WARNING";
        validation_error = 1;
    } else {
        validation_error = 1;
        return false;
    }

    q<<": ["<<pLayerPrefix<<"] Code"<<msgCode<<pMsg;
    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg,
              void *pUserData) {
    Q_UNUSED(msgFlags)
    Q_UNUSED(objType)
    Q_UNUSED(srcObject)
    Q_UNUSED(location)
    Q_UNUSED(msgCode)
    Q_UNUSED(pLayerPrefix)
    Q_UNUSED(pMsg)
    Q_UNUSED(pUserData)

#ifndef WIN32
    raise(SIGTRAP);
#else
    DebugBreak();
#endif

    return false;
}

#if 0
#define DEBUG_ENTRY printf("===========%s=========\n", __PRETTY_FUNCTION__)
#else
#define DEBUG_ENTRY {}
#endif

static void demo_create_xcb_window(struct Demo *demo);
static void demo_run_xcb(struct Demo *demo);

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    DEBUG_ENTRY;
    QGuiApplication app(argc, argv);
    Demo demo;
    demo.show();

    demo.resize(500,500);
    demo.init_vk();
    demo.init_vk_swapchain();
    demo.prepare();
    QTimer t;
    t.setInterval(16);
    QObject::connect(&t, &QTimer::timeout, &demo, &Demo::redraw );
    t.start();
    app.exec();

    return validation_error;
}

Demo::Demo() :
    m_surface(0),
    m_prepared(false),
    m_use_staging_buffer(false),
    m_inst(0),
    m_gpu(0),
    m_device(0),
    m_queue(0),
    m_graphics_queue_node_index(0),
    m_gpu_props({}),
    m_queue_props(0),
    m_memory_properties({}),
    m_enabled_extension_count(0),
    m_enabled_layer_count(0),
//    m_extension_names({}),
//    m_device_validation_layers({}),
//    m_format({}),
//    m_color_space({}),
    m_swapchainImageCount(0),
    m_swapchain(0),
    m_buffers(0),
    m_cmd_pool(0),
    m_depth({}),
    m_uniform_data({}),
    m_cmd(0),
    m_pipeline_layout(0),
    m_desc_layout(0),
    m_pipelineCache(0),
    m_render_pass(0),
    m_pipeline(0),
    m_spin_angle(0.01f),
    m_spin_increment(0.01f),
    m_pause(false),
    m_vert_shader_module(0),
    m_frag_shader_module(0),
    m_desc_pool(0),
    m_desc_set(0),
    m_framebuffers(0),
    m_quit(false),
    m_curFrame(0),
    m_frameCount(0),
    m_validate(true),
    m_use_break(false),
    fpGetPhysicalDeviceSurfaceSupportKHR(0),
    fpGetPhysicalDeviceSurfaceCapabilitiesKHR(0),
    fpGetPhysicalDeviceSurfaceFormatsKHR(0),
    fpGetPhysicalDeviceSurfacePresentModesKHR(0),
    fpCreateSwapchainKHR(0),
    fpDestroySwapchainKHR(0),
    fpGetSwapchainImagesKHR(0),
    fpAcquireNextImageKHR(0),
    fpQueuePresentKHR(0)
{
    DEBUG_ENTRY;

    QVector3D eye(0.0f, 3.0f, 5.0f);
    QVector3D origin(0, 0, 0);
    QVector3D up(0.0f, 1.0f, 0.0);

    m_projection_matrix.perspective(45.0f,
                       1.0f, 0.1f, 100.0f);
    m_view_matrix.lookAt(eye, origin, up);
    m_model_matrix = QMatrix();
    frameCount = INT32_MAX;
    curFrame = 0;
    m_fpsTimer.start();
}

Demo::~Demo()
{
    cleanup();
}

bool Demo::memory_type_from_properties(uint32_t typeBits,
                                        VkFlags requirements_mask,
                                        uint32_t *typeIndex) {
    DEBUG_ENTRY;

    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((m_memory_properties.memoryTypes[i].propertyFlags &
                 requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

void Demo::flush_init_cmd() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

    if (m_cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(m_cmd);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {m_cmd};
    VkFence nullFence = VK_NULL_HANDLE;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(m_queue, 1, &submit_info, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(m_queue);
    assert(!err);

    vkFreeCommandBuffers(m_device, m_cmd_pool, 1, cmd_bufs);
    m_cmd = VK_NULL_HANDLE;
}

void Demo::set_image_layout(VkImage image,
                                  VkImageAspectFlags aspectMask,
                                  VkImageLayout old_image_layout,
                                  VkImageLayout new_image_layout,
                                  VkAccessFlagBits srcAccessMask) {
    DEBUG_ENTRY;
    VkResult U_ASSERT_ONLY err;

    if (m_cmd == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo cmd_ai = {};
        cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_ai.pNext = NULL;
        cmd_ai.commandPool = m_cmd_pool;
        cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_ai.commandBufferCount = 1;

        err = vkAllocateCommandBuffers(m_device, &cmd_ai, &m_cmd);
        assert(!err);

        VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
            cmd_buf_hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            cmd_buf_hinfo.pNext = NULL;
            cmd_buf_hinfo.renderPass = VK_NULL_HANDLE;
            cmd_buf_hinfo.subpass = 0;
            cmd_buf_hinfo.framebuffer = VK_NULL_HANDLE;
            cmd_buf_hinfo.occlusionQueryEnable = VK_FALSE;
            cmd_buf_hinfo.queryFlags = 0;
            cmd_buf_hinfo.pipelineStatistics = 0;
        VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = NULL;
            cmd_buf_info.flags = 0;
            cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;
        err = vkBeginCommandBuffer(m_cmd, &cmd_buf_info);
        assert(!err);
    }

    VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = NULL;
        image_memory_barrier.srcAccessMask = srcAccessMask;
        image_memory_barrier.dstAccessMask = 0;
        image_memory_barrier.oldLayout = old_image_layout;
        image_memory_barrier.newLayout = new_image_layout;
        image_memory_barrier.image = image;
        image_memory_barrier.subresourceRange = {aspectMask, 0, 1, 0, 1};

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(m_cmd, src_stages, dest_stages, 0, 0, NULL, 0,
                         NULL, 1, pmemory_barrier);
}

void Demo::draw_build_cmd(VkCommandBuffer cmd_buf) {
    DEBUG_ENTRY;

    VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
        cmd_buf_hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        cmd_buf_hinfo.pNext = NULL;
        cmd_buf_hinfo.renderPass = VK_NULL_HANDLE;
        cmd_buf_hinfo.subpass = 0;
        cmd_buf_hinfo.framebuffer = VK_NULL_HANDLE;
        cmd_buf_hinfo.occlusionQueryEnable = VK_FALSE;
        cmd_buf_hinfo.queryFlags = 0;
        cmd_buf_hinfo.pipelineStatistics = 0;

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;


    VkClearValue clear_values[2] = {{},{}};
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.4f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = m_render_pass;
    rp_begin.framebuffer = m_framebuffers[m_current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = width();
    rp_begin.renderArea.extent.height = height();
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    VkResult U_ASSERT_ONLY err;

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);

    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipeline_layout, 0, 1, &m_desc_set, 0,
                            NULL);

    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)height();
    viewport.width = (float)width();
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor {};
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = (uint32_t)qMax(0, width());
    scissor.extent.height = (uint32_t)qMax(0, height());
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    prePresentBarrier.image = m_buffers[m_current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, pmemory_barrier);

    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
}

void Demo::update_data_buffer() {
    DEBUG_ENTRY;
    QMatrix4x4 MVP, VP;
    int matrixSize = 16 * sizeof(float);
    uint8_t *pData;
    VkResult U_ASSERT_ONLY err;

    VP = m_projection_matrix * m_view_matrix;

    // Rotate 22.5 degrees around the Y axis
    m_model_matrix.rotate(0.1f, QVector3D(0.0f, 1.0f, 0.0f));

    MVP = VP * m_model_matrix;

    err = vkMapMemory(m_device, m_uniform_data.mem, 0,
                      m_uniform_data.mem_alloc.allocationSize, 0,
                      (void **)&pData);
    assert(!err);

    memcpy(pData, (const void *)MVP.data(), matrixSize);

    vkUnmapMemory(m_device, m_uniform_data.mem);
}

void Demo::draw() {
    DEBUG_ENTRY;
    VkResult U_ASSERT_ONLY err;
    VkSemaphore presentCompleteSemaphore = 0;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    VkFence nullFence = VK_NULL_HANDLE;

    err = vkCreateSemaphore(m_device, &presentCompleteSemaphoreCreateInfo,
                            NULL, &presentCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = fpAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                      presentCompleteSemaphore,
                                      (VkFence)0, // TODO: Show use of fence
                                      &m_current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize_vk();
        draw(); // FIXME recursive - bleh
        vkDestroySemaphore(m_device, presentCompleteSemaphore, NULL);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    // Assume the command buffer has been run on current_buffer before so
    // we need to set the image layout back to COLOR_ATTACHMENT_OPTIMAL
    set_image_layout(m_buffers[m_current_buffer].image,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          (VkAccessFlagBits)0);
    flush_init_cmd();

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    // FIXME/TODO: DEAL WITH VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &presentCompleteSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_buffers[m_current_buffer].cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(m_queue, 1, &submit_info, nullFence);
    assert(!err);

    VkPresentInfoKHR present = {};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain;
    present.pImageIndices = &m_current_buffer;
    present.waitSemaphoreCount = 0;
    present.pResults = 0;

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = fpQueuePresentKHR(m_queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize_vk();
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    err = vkQueueWaitIdle(m_queue);
    assert(err == VK_SUCCESS);

    vkDestroySemaphore(m_device, presentCompleteSemaphore, NULL);
}

void Demo::prepare_buffers() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;
    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities = {};
    err = fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_gpu, m_surface, &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount = 0;
    err = fpGetPhysicalDeviceSurfacePresentModesKHR(
        m_gpu, m_surface, &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes =
        (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = fpGetPhysicalDeviceSurfacePresentModesKHR(
        m_gpu, m_surface, &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent = {};
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = (uint32_t) width();
        swapchainExtent.height = (uint32_t) height(); //FIXME protect against -1
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        resize(surfCapabilities.currentExtent.width, surfCapabilities.currentExtent.height); // FIXME why
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages =
        surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform = {};
    if (surfCapabilities.supportedTransforms &
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = NULL;
        swapchain_ci.surface = m_surface;
        swapchain_ci.minImageCount = desiredNumberOfSwapchainImages;
        swapchain_ci.imageFormat = m_format;
        swapchain_ci.imageColorSpace = m_color_space;
        swapchain_ci.imageExtent.width = swapchainExtent.width;
        swapchain_ci.imageExtent.height = swapchainExtent.height;
        swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_ci.preTransform = preTransform;
        swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = NULL;
        swapchain_ci.presentMode = swapchainPresentMode;
        swapchain_ci.oldSwapchain = oldSwapchain;
        swapchain_ci.clipped = true;
    uint32_t i;

    err = fpCreateSwapchainKHR(m_device, &swapchain_ci, NULL, &m_swapchain);
    assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
        fpDestroySwapchainKHR(m_device, oldSwapchain, NULL);
    }

    err = fpGetSwapchainImagesKHR(m_device, m_swapchain,
                                        &m_swapchainImageCount, NULL);
    assert(!err);

    VkImage *swapchainImages =
        (VkImage *)malloc(m_swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    err = fpGetSwapchainImagesKHR(m_device, m_swapchain,
                                        &m_swapchainImageCount,
                                        swapchainImages);
    assert(!err);

    m_buffers = (SwapchainBuffers *)malloc(sizeof(SwapchainBuffers) *
                                               m_swapchainImageCount);
    assert(m_buffers);

    for (i = 0; i < m_swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.format = m_format;
        color_image_view.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A,
        };
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.flags = 0;

        m_buffers[i].image = swapchainImages[i];

        // Render loop will expect image to have been used before and in
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        // layout and will change to COLOR_ATTACHMENT_OPTIMAL, so init the image
        // to that state
        set_image_layout(
            m_buffers[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            (VkAccessFlagBits)0);

        color_image_view.image = m_buffers[i].image;

        err = vkCreateImageView(m_device, &color_image_view, NULL,
                                &m_buffers[i].view);
        assert(!err);
    }

    if (NULL != presentModes) {
        free(presentModes);
    }
}

void Demo::prepare_depth() {
    DEBUG_ENTRY;
    DEBUG_ENTRY;

    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.pNext = NULL;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = depth_format;
        image.extent.width = width();
        image.extent.height = height();
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image.flags = 0;

    VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.image = VK_NULL_HANDLE;
        view.format = depth_format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
         view.subresourceRange.baseMipLevel = 0;
         view.subresourceRange.levelCount = 1;
         view.subresourceRange.baseArrayLayer = 0;
         view.subresourceRange.layerCount = 1;
        view.flags = 0;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    m_depth.format = depth_format;

    /* create image */
    err = vkCreateImage(m_device, &image, NULL, &m_depth.image);
    assert(!err);

    vkGetImageMemoryRequirements(m_device, m_depth.image, &mem_reqs);
    assert(!err);

    m_depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    m_depth.mem_alloc.pNext = NULL;
    m_depth.mem_alloc.allocationSize = mem_reqs.size;
    m_depth.mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
                                       0, /* No requirements */
                                       &m_depth.mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(m_device, &m_depth.mem_alloc, NULL,
                           &m_depth.mem);
    assert(!err);

    /* bind memory */
    err =
        vkBindImageMemory(m_device, m_depth.image, m_depth.mem, 0);
    assert(!err);

    set_image_layout(m_depth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          (VkAccessFlagBits)0);

    /* create image view */
    view.image = m_depth.image;
    err = vkCreateImageView(m_device, &view, NULL, &m_depth.view);
    assert(!err);
}

VkFormat QtFormat2vkFormat(QImage::Format f) {
    switch (f) {
/*
    case QImage::Format_Mono: return VK_FORMAT_UNDEFINED; //FIXME
    case QImage::Format_MonoLSB: return VK_FORMAT_UNDEFINED;
    case QImage::Format_Indexed8: return VK_FORMAT_UNDEFINED;
*/
    case QImage::Format_RGB32: return VK_FORMAT_R8G8B8A8_UNORM;
    case QImage::Format_ARGB32: return VK_FORMAT_R8G8B8A8_UNORM;
/*
    case QImage::Format_ARGB32_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGB16: return VK_FORMAT_;
    case QImage::Format_ARGB8565_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGB666: return VK_FORMAT_;
    case QImage::Format_ARGB6666_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGB555: return VK_FORMAT_;
    case QImage::Format_ARGB8555_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGB888: return VK_FORMAT_;
    case QImage::Format_RGB444: return VK_FORMAT_;
    case QImage::Format_ARGB4444_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGBX8888: return VK_FORMAT_;
    case QImage::Format_RGBA8888: return VK_FORMAT_;
    case QImage::Format_RGBA8888_Premultiplied: return VK_FORMAT_;
    case QImage::Format_BGR30: return VK_FORMAT_;
    case QImage::Format_A2BGR30_Premultiplied: return VK_FORMAT_;
    case QImage::Format_RGB30: return VK_FORMAT_;
    case QImage::Format_A2RGB30_Premultiplied: return VK_FORMAT_;
    case QImage::Format_Alpha8: return VK_FORMAT_;
    case QImage::Format_Grayscale8: return VK_FORMAT_;
*/
    }
}

void Demo::prepare_texture_image(const char *filename,
                                       struct texture_object *tex_obj,
                                       VkImageTiling tiling,
                                       VkImageUsageFlags usage,
                                       VkFlags required_props) {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

    QImage img(filename);
    const VkFormat tex_format = QtFormat2vkFormat(img.format());

    if (img.isNull()) {
        qFatal("Failed to load textures %s\n", filename);
    }

    tex_obj->tex_width = img.width();
    tex_obj->tex_height = img.height();

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = img.width();
    image_create_info.extent.height = img.height();
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = usage;
    image_create_info.flags = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkMemoryRequirements mem_reqs;

    err = vkCreateImage(m_device, &image_create_info, NULL, &tex_obj->image);
    assert(!err);

    vkGetImageMemoryRequirements(m_device, tex_obj->image, &mem_reqs);

    tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tex_obj->mem_alloc.pNext = NULL;
    tex_obj->mem_alloc.allocationSize = mem_reqs.size;
    tex_obj->mem_alloc.memoryTypeIndex = 0;

    bool U_ASSERT_ONLY pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
                                       required_props,
                                       &tex_obj->mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(m_device, &tex_obj->mem_alloc, NULL,
                           &(tex_obj->mem));
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(m_device, tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        VkImageSubresource subres = {};
        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.mipLevel = 0;
        subres.arrayLayer = 0;

        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(m_device, tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(m_device, tex_obj->mem, 0,
                          tex_obj->mem_alloc.allocationSize, 0, &data);
        assert(!err);

        memcpy(data, img.bits(), img.byteCount() ); // FIXME - in place decoding wanted

        vkUnmapMemory(m_device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    set_image_layout(tex_obj->image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_PREINITIALIZED, tex_obj->imageLayout,
                          VK_ACCESS_HOST_WRITE_BIT);
    /* setting the image layout does not reference the actual memory so no need
     * to add a mem ref */
}

void Demo::destroy_texture_image(struct texture_object *tex_objs) {
    DEBUG_ENTRY;

    /* clean up staging resources */
    vkFreeMemory(m_device, tex_objs->mem, NULL);
    vkDestroyImage(m_device, tex_objs->image, NULL);
}

void Demo::prepare_textures() {
    DEBUG_ENTRY;

    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM; // FIXME get from qimage format?
    VkFormatProperties props;
    uint32_t i;

    vkGetPhysicalDeviceFormatProperties(m_gpu, tex_format, &props);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        VkResult U_ASSERT_ONLY err;

        if ((props.linearTilingFeatures &
             VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
            !m_use_staging_buffer) {
            /* Device can texture using linear textures */
            prepare_texture_image(tex_files[i], &m_textures[i],
                                       VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures &
                   VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            prepare_texture_image(tex_files[i], &staging_texture,
                                       VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

            prepare_texture_image(
                tex_files[i], &m_textures[i], VK_IMAGE_TILING_OPTIMAL,
                (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            set_image_layout(staging_texture.image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  staging_texture.imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  (VkAccessFlagBits)0);

            set_image_layout(m_textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  m_textures[i].imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  (VkAccessFlagBits)0);

            VkImageCopy copy_region = {};

                copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                copy_region.srcOffset = {0, 0, 0};
                copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                copy_region.dstOffset = {0, 0, 0};
                copy_region.extent = {staging_texture.tex_width, staging_texture.tex_height, 1};

            vkCmdCopyImage(
                m_cmd, staging_texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_textures[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            set_image_layout(m_textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  m_textures[i].imageLayout,
                                  (VkAccessFlagBits)0);

            flush_init_cmd();

            destroy_texture_image(&staging_texture);
        } else {
            /* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        VkSamplerCreateInfo sampler = {};
            sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler.pNext = NULL;
            sampler.magFilter = VK_FILTER_NEAREST;
            sampler.minFilter = VK_FILTER_NEAREST;
            sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler.mipLodBias = 0.0f;
            sampler.anisotropyEnable = VK_FALSE;
            sampler.maxAnisotropy = 1;
            sampler.compareOp = VK_COMPARE_OP_NEVER;
            sampler.minLod = 0.0f;
            sampler.maxLod = 0.0f;
            sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            sampler.unnormalizedCoordinates = VK_FALSE;

        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.image = VK_NULL_HANDLE;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = tex_format;
        // attention: BGRA since that is what QImageReader gives us
        // does it depoend on the endian-ness of the platform?
        view.components =  {
            VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A,
        };
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        view.flags = 0;

        /* create sampler */
        err = vkCreateSampler(m_device, &sampler, NULL,
                              &m_textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = m_textures[i].image;
        err = vkCreateImageView(m_device, &view, NULL,
                                &m_textures[i].view);
        assert(!err);
    }
}

void Demo::prepare_cube_data_buffer() {
    DEBUG_ENTRY;

    VkBufferCreateInfo buf_info;
    VkMemoryRequirements mem_reqs;
    uint8_t *pData;
    int i;
    QMatrix4x4 MVP, VP;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;
    struct vktexcube_vs_uniform data;

    VP = m_projection_matrix * m_view_matrix;
    MVP = VP * m_model_matrix;
    memcpy(data.mvp, &MVP, sizeof(data.mvp));

    for (i = 0; i < 12 * 3; i++) {
        data.position[i][0] = g_vertex_buffer_data[i * 3];
        data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
        data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2 * i];
        data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);
    err =
        vkCreateBuffer(m_device, &buf_info, NULL, &m_uniform_data.buf);
    assert(!err);

    vkGetBufferMemoryRequirements(m_device, m_uniform_data.buf,
                                  &mem_reqs);

    m_uniform_data.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    m_uniform_data.mem_alloc.pNext = NULL;
    m_uniform_data.mem_alloc.allocationSize = mem_reqs.size;
    m_uniform_data.mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(
        mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &m_uniform_data.mem_alloc.memoryTypeIndex);
    assert(pass);

    err = vkAllocateMemory(m_device, &m_uniform_data.mem_alloc, NULL,
                           &(m_uniform_data.mem));
    assert(!err);

    err = vkMapMemory(m_device, m_uniform_data.mem, 0,
                      m_uniform_data.mem_alloc.allocationSize, 0,
                      (void **)&pData);
    assert(!err);

    memcpy(pData, &data, sizeof data);

    vkUnmapMemory(m_device, m_uniform_data.mem);

    err = vkBindBufferMemory(m_device, m_uniform_data.buf,
                             m_uniform_data.mem, 0);
    assert(!err);

    m_uniform_data.buffer_info.buffer = m_uniform_data.buf;
    m_uniform_data.buffer_info.offset = 0;
    m_uniform_data.buffer_info.range = sizeof(data);
}

void Demo::prepare_descriptor_layout() {
    DEBUG_ENTRY;

    VkDescriptorSetLayoutBinding layout_bindings[2] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = NULL;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = DEMO_TEXTURE_COUNT;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = layout_bindings;

    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(m_device, &descriptor_layout, NULL,
                                      &m_desc_layout);
    assert(!err);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &m_desc_layout;

    err = vkCreatePipelineLayout(m_device, &pPipelineLayoutCreateInfo, NULL,
                                 &m_pipeline_layout);
    assert(!err);
}

void Demo::prepare_render_pass() {
    DEBUG_ENTRY;

    VkAttachmentDescription attachments[2] = {{},{}};
    attachments[0].format = m_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = m_depth.format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = &depth_reference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(m_device, &rp_info, NULL, &m_render_pass);
    assert(!err);
}

VkShaderModule Demo::prepare_shader_module(const char *code, size_t size) {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;
    VkShaderModule module = 0;
    VkShaderModuleCreateInfo moduleCreateInfo = {};

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (uint32_t*)code;
    moduleCreateInfo.flags = 0;
    err = vkCreateShaderModule(m_device, &moduleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

char *demo_read_spv(const char *filename, size_t *psize) {
    long int size;
    size_t U_ASSERT_ONLY retval;
    char *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = (char*)malloc(size);
    retval = fread(shader_code, size, 1, fp);
    assert(retval == 1);

    *psize = size;

    fclose(fp);
    return shader_code;
}

VkShaderModule Demo::prepare_vs() {
    DEBUG_ENTRY;

    char *vertShaderCode;
    size_t size;

    vertShaderCode = demo_read_spv("cube-vert.spv", &size);

    assert(vertShaderCode);
    m_vert_shader_module =
        prepare_shader_module(vertShaderCode, size);

    free(vertShaderCode);

    return m_vert_shader_module;
}

VkShaderModule Demo::prepare_fs() {
    DEBUG_ENTRY;

    char *fragShaderCode;
    size_t size;

    fragShaderCode = demo_read_spv("cube-frag.spv", &size);
    assert(fragShaderCode);
    m_frag_shader_module =
        prepare_shader_module(fragShaderCode, size);

    free(fragShaderCode);

    return m_frag_shader_module;
}

void Demo::prepare_pipeline() {
    DEBUG_ENTRY;

    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    VkPipelineCacheCreateInfo pipelineCache_ci = {};
    VkPipelineVertexInputStateCreateInfo vi = {};
    VkPipelineInputAssemblyStateCreateInfo ia = {};
    VkPipelineRasterizationStateCreateInfo rs = {};
    VkPipelineColorBlendStateCreateInfo cb = {};
    VkPipelineDepthStencilStateCreateInfo ds = {};
    VkPipelineViewportStateCreateInfo vp = {};
    VkPipelineMultisampleStateCreateInfo ms = {};
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline_ci, 0, sizeof(pipeline_ci));
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.layout = m_pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState *att_state = (VkPipelineColorBlendAttachmentState*)malloc(sizeof(*att_state));
    memset(att_state, 0x0, sizeof(*att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;

    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two stages: vs and fs
    pipeline_ci.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = prepare_vs();
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = prepare_fs();
    shaderStages[1].pName = "main";

    memset(&pipelineCache_ci, 0, sizeof(pipelineCache_ci));
    pipelineCache_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(m_device, &pipelineCache_ci, NULL, &m_pipelineCache);
    assert(!err);

    pipeline_ci.pVertexInputState = &vi;
    pipeline_ci.pInputAssemblyState = &ia;
    pipeline_ci.pRasterizationState = &rs;
    pipeline_ci.pColorBlendState = &cb;
    pipeline_ci.pMultisampleState = &ms;
    pipeline_ci.pViewportState = &vp;
    pipeline_ci.pDepthStencilState = &ds;
    pipeline_ci.pStages = shaderStages;
    pipeline_ci.renderPass = m_render_pass;
    pipeline_ci.pDynamicState = &dynamicState;
    pipeline_ci.renderPass = m_render_pass;


    err = vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1,
                                    &pipeline_ci, NULL, &m_pipeline);
    assert(!err);

    vkDestroyShaderModule(m_device, m_frag_shader_module, NULL);
    vkDestroyShaderModule(m_device, m_vert_shader_module, NULL);
}

void Demo::prepare_descriptor_pool() {
    DEBUG_ENTRY;

    VkDescriptorPoolSize type_counts[2] = {{},{}};
    type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_counts[0].descriptorCount = 1;
    type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_counts[1].descriptorCount = DEMO_TEXTURE_COUNT;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.maxSets = 1;
    descriptor_pool.poolSizeCount = 2;
    descriptor_pool.pPoolSizes = type_counts;

    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(m_device, &descriptor_pool, NULL,
                                 &m_desc_pool);
    assert(!err);
}

void Demo::prepare_descriptor_set() {
    DEBUG_ENTRY;

    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2]={{},{}};
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = m_desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &m_desc_layout;

    err = vkAllocateDescriptorSets(m_device, &alloc_info, &m_desc_set);
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = m_textures[i].sampler;
        tex_descs[i].imageView = m_textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_desc_set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &m_uniform_data.buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    vkUpdateDescriptorSets(m_device, 2, writes, 0, NULL);
}

void Demo::prepare_framebuffers() {
    DEBUG_ENTRY;

    VkImageView attachments[2] = {{},{}};
    attachments[1] = m_depth.view;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = m_render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width = (uint32_t) width(); //FIXME unsigned cast
    fb_info.height = (uint32_t) height();
    fb_info.layers = 1;

    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    m_framebuffers = (VkFramebuffer *)malloc(m_swapchainImageCount *
                                                 sizeof(VkFramebuffer));
    assert(m_framebuffers);

    for (i = 0; i < m_swapchainImageCount; i++) {
        attachments[0] = m_buffers[i].view;
        err = vkCreateFramebuffer(m_device, &fb_info, NULL,
                                  &m_framebuffers[i]);
        assert(!err);
    }
}

void Demo::redraw()
{
    static uint32_t f = 0;
    f++;
    if(f == 100) {
        qDebug()<<(float)f / (float)m_fpsTimer.elapsed() * 1000.0f;
        f=0;
        m_fpsTimer.restart();

    }

    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(m_device);
    update_data_buffer();
    draw();
}

void Demo::prepare() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = m_graphics_queue_node_index;
    cmd_pool_info.flags = 0;

    err = vkCreateCommandPool(m_device, &cmd_pool_info, NULL,
                              &m_cmd_pool);
    assert(!err);

    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = NULL;
    cmd.commandPool = m_cmd_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.commandBufferCount = 1;

    prepare_buffers();
    prepare_depth();
    prepare_textures();
    prepare_cube_data_buffer();

    prepare_descriptor_layout();
    prepare_render_pass();
    prepare_pipeline();

    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
        err =
            vkAllocateCommandBuffers(m_device, &cmd, &m_buffers[i].cmd);
        assert(!err);
    }

    prepare_descriptor_pool();
    prepare_descriptor_set();

    prepare_framebuffers();

    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
        m_current_buffer = i;
        draw_build_cmd(m_buffers[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    flush_init_cmd();

    m_current_buffer = 0;
    m_prepared = true;
}

void Demo::cleanup() {
    DEBUG_ENTRY;

    uint32_t i;

    m_prepared = false;

    for (i = 0; i < m_swapchainImageCount; i++) {
        vkDestroyFramebuffer(m_device, m_framebuffers[i], NULL);
    }
    free(m_framebuffers);
    vkDestroyDescriptorPool(m_device, m_desc_pool, NULL);

    vkDestroyPipeline(m_device, m_pipeline, NULL);
    vkDestroyPipelineCache(m_device, m_pipelineCache, NULL);
    vkDestroyRenderPass(m_device, m_render_pass, NULL);
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device, m_desc_layout, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(m_device, m_textures[i].view, NULL);
        vkDestroyImage(m_device, m_textures[i].image, NULL);
        vkFreeMemory(m_device, m_textures[i].mem, NULL);
        vkDestroySampler(m_device, m_textures[i].sampler, NULL);
    }
    fpDestroySwapchainKHR(m_device, m_swapchain, NULL);

    vkDestroyImageView(m_device, m_depth.view, NULL);
    vkDestroyImage(m_device, m_depth.image, NULL);
    vkFreeMemory(m_device, m_depth.mem, NULL);

    vkDestroyBuffer(m_device, m_uniform_data.buf, NULL);
    vkFreeMemory(m_device, m_uniform_data.mem, NULL);

    for (i = 0; i < m_swapchainImageCount; i++) {
        vkDestroyImageView(m_device, m_buffers[i].view, NULL);
        vkFreeCommandBuffers(m_device, m_cmd_pool, 1,
                             &m_buffers[i].cmd);
    }
    free(m_buffers);

    free(m_queue_props);

    vkDestroyCommandPool(m_device, m_cmd_pool, NULL);
    vkDestroyDevice(m_device, NULL);
    if (m_validate) {
        DestroyDebugReportCallback(m_inst, msg_callback, NULL);
    }
    vkDestroySurfaceKHR(m_inst, m_surface, NULL);
    vkDestroyInstance(m_inst, NULL);
}

void Demo::resize_vk() {
    DEBUG_ENTRY;

    uint32_t i;

    // Don't react to resize until after first initialization.
    if (!m_prepared) {
        return;
    }
    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
    m_prepared = false;

    for (i = 0; i < m_swapchainImageCount; i++) {
        vkDestroyFramebuffer(m_device, m_framebuffers[i], NULL);
    }
    free(m_framebuffers);
    vkDestroyDescriptorPool(m_device, m_desc_pool, NULL);

    vkDestroyPipeline(m_device, m_pipeline, NULL);
    vkDestroyPipelineCache(m_device, m_pipelineCache, NULL);
    vkDestroyRenderPass(m_device, m_render_pass, NULL);
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device, m_desc_layout, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(m_device, m_textures[i].view, NULL);
        vkDestroyImage(m_device, m_textures[i].image, NULL);
        vkFreeMemory(m_device, m_textures[i].mem, NULL);
        vkDestroySampler(m_device, m_textures[i].sampler, NULL);
    }

    vkDestroyImageView(m_device, m_depth.view, NULL);
    vkDestroyImage(m_device, m_depth.image, NULL);
    vkFreeMemory(m_device, m_depth.mem, NULL);

    vkDestroyBuffer(m_device, m_uniform_data.buf, NULL);
    vkFreeMemory(m_device, m_uniform_data.mem, NULL);

    for (i = 0; i < m_swapchainImageCount; i++) {
        vkDestroyImageView(m_device, m_buffers[i].view, NULL);
        vkFreeCommandBuffers(m_device, m_cmd_pool, 1,
                             &m_buffers[i].cmd);
    }
    vkDestroyCommandPool(m_device, m_cmd_pool, NULL);
    free(m_buffers);

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    prepare();
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
VkBool32 Demo::check_layers(uint32_t check_count, const char **check_names,
                                  uint32_t layer_count,
                                  VkLayerProperties *layers) {
    DEBUG_ENTRY;

    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

void Demo::resizeEvent(QResizeEvent *e)
{
    qDebug()<<e;
    DEBUG_ENTRY;

    resize_vk();
    e->accept(); //FIXME?
    QWindow::resizeEvent(e);
}

void Demo::init_vk() {
    DEBUG_ENTRY;

    VkResult err;
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t device_validation_layer_count = 0;
    const char **instance_validation_layers = NULL;
    m_enabled_extension_count = 0;
    m_enabled_layer_count = 0;

    const char *instance_validation_layers_alt1[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    const char *instance_validation_layers_alt2[] = {
        "VK_LAYER_GOOGLE_threading",     "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_device_limits", "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_image",         "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_swapchain",     "VK_LAYER_GOOGLE_unique_objects"
    };

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    if (m_validate) {
        qDebug()<<"enabling validation";
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        assert(!err);

        instance_validation_layers = instance_validation_layers_alt1;
        if (instance_layer_count > 0) {
            VkLayerProperties *instance_layers =
                    (VkLayerProperties*) malloc(sizeof (VkLayerProperties) * instance_layer_count);
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
                    instance_layers);
            assert(!err);


            validation_found = check_layers(
                    ARRAY_SIZE(instance_validation_layers_alt1),
                    instance_validation_layers, instance_layer_count,
                    instance_layers);
            if (validation_found) {
                m_enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt1);
                m_device_validation_layers[0] = "VK_LAYER_LUNARG_standard_validation";
                device_validation_layer_count = 1;
            } else {
                // use alternative set of validation layers
                instance_validation_layers = instance_validation_layers_alt2;
                m_enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt2);
                validation_found = check_layers(
                    ARRAY_SIZE(instance_validation_layers_alt2),
                    instance_validation_layers, instance_layer_count,
                    instance_layers);
                device_validation_layer_count =
                        ARRAY_SIZE(instance_validation_layers_alt2);
                for (uint32_t i = 0; i < device_validation_layer_count; i++) {
                    m_device_validation_layers[i] =
                            instance_validation_layers[i];
                }
            }
            free(instance_layers);
        }

        if (!validation_found) {
            ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find "
                    "required validation layer.\n\n"
                    "Please look at the Getting Started guide for additional "
                    "information.\n",
                    "vkCreateInstance Failure");
        }
    }

    /* Look for instance extensions */
    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;
    memset(m_extension_names, 0, sizeof(m_extension_names));

    err = vkEnumerateInstanceExtensionProperties(
        NULL, &instance_extension_count, NULL);
    assert(!err);

    if (instance_extension_count > 0) {
        VkExtensionProperties *instance_extensions =
            (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * instance_extension_count);
        err = vkEnumerateInstanceExtensionProperties(
            NULL, &instance_extension_count, instance_extensions);
        assert(!err);
        for (uint32_t i = 0; i < instance_extension_count; i++) {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                surfaceExtFound = 1;
                m_extension_names[m_enabled_extension_count++] =
                    VK_KHR_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                platformSurfaceExtFound = 1;
                m_extension_names[m_enabled_extension_count++] =
                    VK_KHR_XCB_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                if (m_validate) {
                    m_extension_names[m_enabled_extension_count++] =
                        VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
                }
            }
            assert(m_enabled_extension_count < 64);
        }

        free(instance_extensions);
    }

    if (!surfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    if (!platformSurfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = NULL;
    app.pApplicationName = "cube";
    app.applicationVersion = 0;
    app.pEngineName = "cubenegine";
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = m_enabled_layer_count;
    inst_info.ppEnabledLayerNames = (const char *const *)instance_validation_layers;
    inst_info.enabledExtensionCount = m_enabled_extension_count;
    inst_info.ppEnabledExtensionNames = (const char *const *)m_extension_names;

    /*
     * This is info for a temp callback to use during CreateInstance.
     * After the instance is created, we use the instance-based
     * function to register the final callback.
     */
    VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
    if (m_validate) {
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = m_use_break ? BreakCallback : dbgFunc;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        // | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        inst_info.pNext = &dbgCreateInfo;
    }

    uint32_t gpu_count = 0;

    err = vkCreateInstance(&inst_info, NULL, &m_inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT("Cannot find a specified extension library"
                 ".\nMake sure your layers path is set appropriately.\n",
                 "vkCreateInstance Failure");
    } else if (err) {
        ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                 "installable client driver (ICD) installed?\nPlease look at "
                 "the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(m_inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);

    if (gpu_count > 0) {
        VkPhysicalDevice *physical_devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
        err = vkEnumeratePhysicalDevices(m_inst, &gpu_count, physical_devices);
        assert(!err);
        /* For cube demo we just grab the first physical device */
        m_gpu = physical_devices[0];
        free(physical_devices);
    } else {
        ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) "
                 "installed?\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkEnumeratePhysicalDevices Failure");
    }

    /* Look for validation layers */
    validation_found = 0;
    m_enabled_layer_count = 0;
    uint32_t device_layer_count = 0;
    err =
        vkEnumerateDeviceLayerProperties(m_gpu, &device_layer_count, NULL);
    assert(!err);

    if (device_layer_count > 0) {
        VkLayerProperties *device_layers =
            (VkLayerProperties*)malloc(sizeof(VkLayerProperties) * device_layer_count);
        err = vkEnumerateDeviceLayerProperties(m_gpu, &device_layer_count,
                                               device_layers);
        assert(!err);

        if (m_validate) {
            validation_found = check_layers(device_validation_layer_count,
                                                 m_device_validation_layers,
                                                 device_layer_count,
                                                 device_layers);
            m_enabled_layer_count = device_validation_layer_count;
        }

        free(device_layers);
    }

    if (m_validate && !validation_found) {
        ERR_EXIT("vkEnumerateDeviceLayerProperties failed to find "
                 "a required validation layer.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateDevice Failure");
    }

    /* Look for device extensions */
    uint32_t device_extension_count = 0;
    VkBool32 swapchainExtFound = 0;
    m_enabled_extension_count = 0;
    memset(m_extension_names, 0, sizeof(m_extension_names));

    err = vkEnumerateDeviceExtensionProperties(m_gpu, NULL,
                                               &device_extension_count, NULL);
    assert(!err);

    if (device_extension_count > 0) {
        VkExtensionProperties *device_extensions =
            (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * device_extension_count);
        err = vkEnumerateDeviceExtensionProperties(
            m_gpu, NULL, &device_extension_count, device_extensions);
        assert(!err);

        for (uint32_t i = 0; i < device_extension_count; i++) {
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        device_extensions[i].extensionName)) {
                swapchainExtFound = 1;
                m_extension_names[m_enabled_extension_count++] =
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            }
            assert(m_enabled_extension_count < 64);
        }

        free(device_extensions);
    }

    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
                 "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    if (m_validate) {
        CreateDebugReportCallback =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                m_inst, "vkCreateDebugReportCallbackEXT");
        DestroyDebugReportCallback =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                m_inst, "vkDestroyDebugReportCallbackEXT");
        if (!CreateDebugReportCallback) {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        if (!DestroyDebugReportCallback) {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        DebugReportMessage =
            (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                m_inst, "vkDebugReportMessageEXT");
        if (!DebugReportMessage) {
            ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
                     "vkGetProcAddr Failure");
        }

        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        PFN_vkDebugReportCallbackEXT callback;
        callback = m_use_break ? BreakCallback : dbgFunc;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = callback;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        //| VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        err = CreateDebugReportCallback(m_inst, &dbgCreateInfo, NULL,
                                              &msg_callback);
        switch (err) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
                     "CreateDebugReportCallback Failure");
            break;
        default:
            ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
                     "CreateDebugReportCallback Failure");
            break;
        }
    }
    vkGetPhysicalDeviceProperties(m_gpu, &m_gpu_props);

    /* Call with NULL data to get count */
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &m_queue_count,
                                             NULL);
    assert(m_queue_count >= 1);

    m_queue_props = (VkQueueFamilyProperties *)malloc(
        m_queue_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &m_queue_count,
                                             m_queue_props);
    // Find a queue that supports gfx
    uint32_t gfx_queue_idx = 0;
    for (gfx_queue_idx = 0; gfx_queue_idx < m_queue_count;
         gfx_queue_idx++) {
        if (m_queue_props[gfx_queue_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            break;
    }
    assert(gfx_queue_idx < m_queue_count);
    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(m_gpu, &physDevFeatures);

    GET_INSTANCE_PROC_ADDR(m_inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(m_inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(m_inst, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(m_inst, GetPhysicalDeviceSurfacePresentModesKHR);
    GET_INSTANCE_PROC_ADDR(m_inst, GetSwapchainImagesKHR);
}

void Demo::create_device() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;
    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queue = {};
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.pNext = NULL;
        queue.queueFamilyIndex = m_graphics_queue_node_index;
        queue.queueCount = 1;
        queue.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_ci = {};
        device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_ci.pNext = NULL;
        device_ci.queueCreateInfoCount = 1;
        device_ci.pQueueCreateInfos = &queue;
        device_ci.enabledLayerCount = m_enabled_layer_count;
        device_ci.ppEnabledLayerNames =
            (const char *const *)((m_validate)
                                      ? m_device_validation_layers
                                      : NULL);
        device_ci.enabledExtensionCount = m_enabled_extension_count;
        device_ci.ppEnabledExtensionNames = (const char *const *)m_extension_names;
        device_ci.pEnabledFeatures =
            NULL; // If specific features are required, pass them in here

    err = vkCreateDevice(m_gpu, &device_ci, NULL, &m_device);
    assert(!err);
}

void Demo::init_vk_swapchain() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;
    uint32_t i;

// Create a WSI surface for the window:
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = connection;
    createInfo.hwnd = window;

    err =
        vkCreateWin32SurfaceKHR(inst, &createInfo, NULL, &surface);
#else

    QPlatformNativeInterface *native =  QGuiApplication::platformNativeInterface();
    Q_ASSERT(native);
    xcb_connection_t* connection = static_cast<xcb_connection_t *>(native->nativeResourceForWindow("connection", this));
    Q_ASSERT(connection);
    xcb_window_t xcb_window = static_cast<xcb_window_t>(winId());
    Q_ASSERT(xcb_window);

    qDebug()<<"connection and winid:"<<connection<<xcb_window;
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = connection;
    createInfo.window = xcb_window;

    err = vkCreateXcbSurfaceKHR(m_inst, &createInfo, NULL, &m_surface);

#endif // _WIN32
    assert(!err);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *supportsPresent =
        (VkBool32 *)malloc(m_queue_count * sizeof(VkBool32));
    for (i = 0; i < m_queue_count; i++) {
        fpGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, m_surface,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (i = 0; i < m_queue_count; i++) {
        if ((m_queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < m_queue_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX ||
        presentQueueNodeIndex == UINT32_MAX) {
        ERR_EXIT("Could not find a graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    // TODO: Add support for separate queues, including presentation,
    //       synchronization, and appropriate tracking for QueueSubmit.
    // NOTE: While it is possible for an application to use a separate graphics
    //       and a present queues, this demo program assumes it is only using
    //       one:
    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        ERR_EXIT("Could not find a common graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    m_graphics_queue_node_index = graphicsQueueNodeIndex;

    create_device();

    GET_DEVICE_PROC_ADDR(m_device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(m_device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(m_device, QueuePresentKHR);

    vkGetDeviceQueue(m_device, m_graphics_queue_node_index, 0,
                     &m_queue);

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &formatCount, NULL);
    assert(formatCount);
    assert(!err);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface,
                                                     &formatCount, surfFormats);
    assert(!err);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        m_format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        assert(formatCount >= 1);
        m_format = surfFormats[0].format;
    }
    m_color_space = surfFormats[0].colorSpace;

    m_quit = false;
    m_curFrame = 0;

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_memory_properties);
}
