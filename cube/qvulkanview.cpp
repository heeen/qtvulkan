
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
#include <signal.h>
#include <stdarg.h>

#include "qvulkanview.h"

#include <QMessageBox>
#include <QResizeEvent>
#include <QApplication>

#include <QtMath>
#include <qpa/qplatformnativeinterface.h>

#include "qvkcmdbuf.h"


static const char *tex_files[] = {"lunarg.ppm"};
uint32_t ScopeDebug::stack = 0;


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


QVulkanView::QVulkanView() :
    m_inst(),
    m_gpu(m_inst.device(0))
    , m_graphics_queue_node_index(0)
    , m_device( new QVkDevice(m_inst, m_gpu, m_graphics_queue_node_index, m_deviceValidationLayers, m_extensionNames))
    , m_queue(m_device, m_graphics_queue_node_index)
{
    DEBUG_ENTRY;

    QVkQueue::fpQueuePresentKHR = m_device->fpQueuePresentKHR; // ugh!
    init_vk_swapchain();
    prepare();

}

QVulkanView::~QVulkanView()
{
    DEBUG_ENTRY;

    m_prepared = false;

    for (int i = 0; i < m_framebuffers.count(); i++) {
        vkDestroyFramebuffer(*m_device, m_framebuffers[i], nullptr);
    }
    vkDestroyDescriptorPool(*m_device, m_desc_pool, nullptr);

    vkDestroyPipeline(*m_device, m_pipeline, nullptr);
    vkDestroyPipelineCache(*m_device, m_pipelineCache, nullptr);
    vkDestroyRenderPass(*m_device, m_render_pass, nullptr);
    vkDestroyPipelineLayout(*m_device, m_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(*m_device, m_desc_layout, nullptr);

    for (int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(*m_device, m_textures[i].view, nullptr);
        vkDestroyImage(*m_device, m_textures[i].image, nullptr);
        vkFreeMemory(*m_device, m_textures[i].mem, nullptr);
        vkDestroySampler(*m_device, m_textures[i].sampler, nullptr);
    }

    vkDestroyImageView(*m_device, m_depth.view, nullptr);
    vkDestroyImage(*m_device, m_depth.image, nullptr);
    vkFreeMemory(*m_device, m_depth.mem, nullptr);

    for (int i = 0; i < m_buffers.count(); i++) {
        vkDestroyImageView(*m_device, m_buffers[i].view, nullptr);
        m_buffers[i].view = nullptr;
        vkFreeCommandBuffers(*m_device, m_cmd_pool, 1, &m_buffers[i].cmd);
        m_buffers[i].cmd = nullptr;
    }

    vkDestroyCommandPool(*m_device, m_cmd_pool, nullptr);

    vkDestroySurfaceKHR(m_inst, m_surface, nullptr);
}

void QVulkanView::flush_init_cmd() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

    if (m_cmd == nullptr)
        return;

    err = vkEndCommandBuffer(m_cmd);
    Q_ASSERT(!err);

    const VkCommandBuffer cmd_bufs[] = {m_cmd};
    VkFence nullFence = nullptr;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    err = vkQueueSubmit(m_queue, 1, &submit_info, nullFence);
    Q_ASSERT(!err);

    err = vkQueueWaitIdle(m_queue);
    Q_ASSERT(!err);

    vkFreeCommandBuffers(*m_device, m_cmd_pool, 1, cmd_bufs);
    m_cmd = nullptr;
}

void QVulkanView::set_image_layout(VkImage image,
                                  VkImageAspectFlags aspectMask,
                                  VkImageLayout old_image_layout,
                                  VkImageLayout new_image_layout,
                                  VkAccessFlagBits srcAccessMask) {
    DEBUG_ENTRY;
    DBG("image %p from %x to %x\n", (void*) image, old_image_layout, new_image_layout);
    VkResult U_ASSERT_ONLY err;

    if (m_cmd == nullptr) {
        VkCommandBufferAllocateInfo cmd_ai = {};
        cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_ai.pNext = nullptr;
        cmd_ai.commandPool = m_cmd_pool;
        cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_ai.commandBufferCount = 1;

        err = vkAllocateCommandBuffers(*m_device, &cmd_ai, &m_cmd);
        Q_ASSERT(!err);

        VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
            cmd_buf_hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            cmd_buf_hinfo.pNext = nullptr;
            cmd_buf_hinfo.renderPass = nullptr;
            cmd_buf_hinfo.subpass = 0;
            cmd_buf_hinfo.framebuffer = nullptr;
            cmd_buf_hinfo.occlusionQueryEnable = VK_FALSE;
            cmd_buf_hinfo.queryFlags = 0;
            cmd_buf_hinfo.pipelineStatistics = 0;
        VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = nullptr;
            cmd_buf_info.flags = 0;
            cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;
        err = vkBeginCommandBuffer(m_cmd, &cmd_buf_info);
        Q_ASSERT(!err);
    }

    VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
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
        image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(m_cmd, src_stages, dest_stages, 0, 0, nullptr, 0,
                         nullptr, 1, &image_memory_barrier);
}

void QVulkanView::draw() {
    DEBUG_ENTRY;
    VkResult U_ASSERT_ONLY err;
    VkSemaphore presentCompleteSemaphore = nullptr;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = nullptr;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    VkFence nullFence = nullptr;

    err = vkCreateSemaphore(*m_device, &presentCompleteSemaphoreCreateInfo,
                            nullptr, &presentCompleteSemaphore);
    Q_ASSERT(!err);

    // Get the index of the next available swapchain image:
    err = m_swapchain->acquireNextImage(UINT64_MAX,
                                      presentCompleteSemaphore,
                                      nullptr, // TODO: Show use of fence
                                      &m_current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        qWarning("swapchain out of date!");
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize_vk();
        draw(); // FIXME recursive - bleh
        vkDestroySemaphore(*m_device, presentCompleteSemaphore, nullptr);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        qWarning("swapchain SUBOPTIMAL");
        // swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        Q_ASSERT(!err);
    }

    // Assume the command buffer has been run on current_buffer before so
    // we need to set the image layout back to COLOR_ATTACHMENT_OPTIMAL
    QVkCommandBuffer cmd(m_device, m_cmd_pool);
    cmd.record()
        .transformImage(m_buffers[m_current_buffer].image,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_queue.submit(cmd);
    m_queue.waitIdle();

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    // FIXME/TODO: DEAL WITH VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &presentCompleteSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_buffers[m_current_buffer].cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    err = vkQueueSubmit(m_queue, 1, &submit_info, nullFence);
    Q_ASSERT(!err);

    VkPresentInfoKHR present = {};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = nullptr;
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain->handle();
    present.pImageIndices = &m_current_buffer;
    present.waitSemaphoreCount = 0;
    present.pResults = nullptr;

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = m_device->queuePresent(m_queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize_vk();
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        Q_ASSERT(!err);
    }

    err = vkQueueWaitIdle(m_queue);
    Q_ASSERT(err == VK_SUCCESS);

    vkDestroySemaphore(*m_device, presentCompleteSemaphore, nullptr);
}

void QVulkanView::prepare_buffers() {

    DEBUG_ENTRY;
    DBG("view dimensions: %dx%d", width(), height());
    VkResult U_ASSERT_ONLY err;
    QSharedPointer<QVkSwapchain> oldSwapchain = m_swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities = {};
    err = m_inst.getPhysicalDeviceSurfaceCapabilities(
        m_gpu, m_surface, &surfCapabilities);
    Q_ASSERT(!err);

    auto getPresModes = [this](uint32_t* c, VkPresentModeKHR* d) {
            return m_inst.getPhysicalDeviceSurfacePresentModes(m_gpu, m_surface, c, d);
    };
    auto presentModes = getVk<VkPresentModeKHR>(getPresModes);

    VkExtent2D swapchainExtent = {};
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = 400; //(uint32_t) width();
        swapchainExtent.height = 400; //(uint32_t) height(); //FIXME protect against -1
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
    for (auto mode: presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = mode;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = mode;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform = {};
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    m_swapchain = QSharedPointer<QVkSwapchain>(new QVkSwapchain(
                                                   m_device,
                                                   m_surface,
                                                   desiredNumberOfSwapchainImages,
                                                   m_format,
                                                   m_color_space,
                                                   swapchainExtent,
                                                   swapchainPresentMode,
                                                   preTransform,
                                                   oldSwapchain
                                                   ));
    Q_ASSERT(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
//    if (oldSwapchain != nullptr) {
//        m_device->destroySwapchain(oldSwapchain, nullptr);
//    }

    auto getSwapchainImages = [this](uint32_t* c, VkImage* d) {
        return m_swapchain->getSwapchainImages(c, d);
    };

    auto swapchainImages = getVk<VkImage>(getSwapchainImages);
    Q_ASSERT(!swapchainImages.isEmpty());


    m_buffers.resize(swapchainImages.count());

    QVkCommandBuffer cmd(m_device, m_cmd_pool);
    {
    auto cbr=cmd.record();

    for (int i = 0; i < m_buffers.count(); i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = nullptr;
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
        cbr.transformImage(m_buffers[i].image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        color_image_view.image = m_buffers[i].image;

        err = vkCreateImageView(*m_device, &color_image_view, nullptr, &m_buffers[i].view);
        Q_ASSERT(!err);
    }
    }
    m_queue.submit(cmd);
    m_queue.waitIdle();
}

void QVulkanView::prepare_depth() {
    DEBUG_ENTRY;

    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.pNext = nullptr;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = depth_format;
        image.extent.width = 400; //width();
        image.extent.height = 400; //height();
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image.flags = 0;

    VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = nullptr;
        view.image = nullptr;
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

    m_depth.format = depth_format;

    /* create image */
    err = vkCreateImage(*m_device, &image, nullptr, &m_depth.image);
    qDebug()<<"depth image is"<<m_depth.image;
    Q_ASSERT(!err);

    vkGetImageMemoryRequirements(*m_device, m_depth.image, &mem_reqs);
    Q_ASSERT(!err);

    m_depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    m_depth.mem_alloc.pNext = nullptr;
    m_depth.mem_alloc.allocationSize = mem_reqs.size;
    m_depth.mem_alloc.memoryTypeIndex = 0;

    int index = device()->memoryType(mem_reqs.memoryTypeBits, 0 /* No requirements */);
    Q_ASSERT(index >= 0);
    m_depth.mem_alloc.memoryTypeIndex = index;

    /* allocate memory */
    err = vkAllocateMemory(*m_device, &m_depth.mem_alloc, nullptr,
                           &m_depth.mem);
    Q_ASSERT(!err);

    /* bind memory */
    err =
        vkBindImageMemory(*m_device, m_depth.image, m_depth.mem, 0);
    Q_ASSERT(!err);

    set_image_layout(m_depth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          (VkAccessFlagBits)0);

    /* create image view */
    view.image = m_depth.image;
    err = vkCreateImageView(*m_device, &view, nullptr, &m_depth.view);
    Q_ASSERT(!err);
}


void QVulkanView::prepare_texture_image(const char *filename,
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
    image_create_info.pNext = nullptr;
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

    err = vkCreateImage(*m_device, &image_create_info, nullptr, &tex_obj->image);
    Q_ASSERT(!err);

    vkGetImageMemoryRequirements(*m_device, tex_obj->image, &mem_reqs);

    tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tex_obj->mem_alloc.pNext = nullptr;
    tex_obj->mem_alloc.allocationSize = mem_reqs.size;
    tex_obj->mem_alloc.memoryTypeIndex = 0;

    int index =  device()->memoryType(mem_reqs.memoryTypeBits, required_props);

    Q_ASSERT(index >= 0);
    tex_obj->mem_alloc.memoryTypeIndex = index;
    /* allocate memory */
    err = vkAllocateMemory(*m_device, &tex_obj->mem_alloc, nullptr,
                           &(tex_obj->mem));
    Q_ASSERT(!err);

    /* bind memory */
    err = vkBindImageMemory(*m_device, tex_obj->image, tex_obj->mem, 0);
    Q_ASSERT(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        VkImageSubresource subres = {};
        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.mipLevel = 0;
        subres.arrayLayer = 0;

        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(*m_device, tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(*m_device, tex_obj->mem, 0,
                          tex_obj->mem_alloc.allocationSize, 0, &data);
        Q_ASSERT(!err);

        memcpy(data, img.bits(), img.byteCount() ); // FIXME - in place decoding wanted

        vkUnmapMemory(*m_device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    set_image_layout(tex_obj->image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_PREINITIALIZED, tex_obj->imageLayout,
                          VK_ACCESS_HOST_WRITE_BIT);
    /* setting the image layout does not reference the actual memory so no need
     * to add a mem ref */
}

void QVulkanView::destroy_texture_image(struct texture_object *tex_objs) {
    DEBUG_ENTRY;

    /* clean up staging resources */
    vkFreeMemory(*m_device, tex_objs->mem, nullptr);
    vkDestroyImage(*m_device, tex_objs->image, nullptr);
}

void QVulkanView::prepare_textures() {
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
            struct texture_object staging_texture = {};
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
            Q_ASSERT(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        VkSamplerCreateInfo sampler = {};
            sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler.pNext = nullptr;
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
        view.pNext = nullptr;
        view.image = nullptr;
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
        err = vkCreateSampler(*m_device, &sampler, nullptr,
                              &m_textures[i].sampler);
        Q_ASSERT(!err);

        /* create image view */
        view.image = m_textures[i].image;
        err = vkCreateImageView(*m_device, &view, nullptr,
                                &m_textures[i].view);
        Q_ASSERT(!err);
    }
}

void QVulkanView::prepare_descriptor_layout() {
    DEBUG_ENTRY;

    VkDescriptorSetLayoutBinding layout_bindings[2] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = nullptr;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = DEMO_TEXTURE_COUNT;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = nullptr;
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = layout_bindings;

    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(*m_device, &descriptor_layout, nullptr,
                                      &m_desc_layout);
    Q_ASSERT(!err);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &m_desc_layout;

    err = vkCreatePipelineLayout(*m_device, &pPipelineLayoutCreateInfo, nullptr,
                                 &m_pipeline_layout);
    Q_ASSERT(!err);
}

void QVulkanView::prepare_render_pass() {
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
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depth_reference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = nullptr;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = nullptr;
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(*m_device, &rp_info, nullptr, &m_render_pass);
    Q_ASSERT(!err);
}

VkShaderModule QVulkanView::createShaderModule(QString filename) {
    DEBUG_ENTRY;
    VkResult U_ASSERT_ONLY err;

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
        qFatal("could not read shader file!");

    QByteArray code = file.readAll();

    VkShaderModuleCreateInfo moduleCreateInfo = {};

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = nullptr;

    moduleCreateInfo.codeSize = code.size();
    moduleCreateInfo.pCode = (uint32_t*) code.data(); //possible alignment issue?
    moduleCreateInfo.flags = 0;

    VkShaderModule module = nullptr;
    err = vkCreateShaderModule(*m_device, &moduleCreateInfo, nullptr, &module);
    Q_ASSERT(!err);

    return module;
}

void QVulkanView::prepare_pipeline() {
    DEBUG_ENTRY;

    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState att_state = {};
    att_state.colorWriteMask = 0xf;
    att_state.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &att_state; // debug report layer has a bug where it reads from a stack ptr

    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDepthStencilStateCreateInfo ds = {};
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

    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = nullptr;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    const int numStages = 2;      // Two stages: vs and fs
    VkPipelineShaderStageCreateInfo shaderStages[numStages] = {};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = createShaderModule("cube-vert.spv");
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = createShaderModule("cube-frag.spv");
    shaderStages[1].pName = "main";

    VkPipelineCacheCreateInfo pipelineCache_ci = {};
    pipelineCache_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkResult U_ASSERT_ONLY err;
    err = vkCreatePipelineCache(*m_device, &pipelineCache_ci, nullptr, &m_pipelineCache);
    Q_ASSERT(!err);

    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.layout = m_pipeline_layout;
    pipeline_ci.stageCount = numStages;
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

    err = vkCreateGraphicsPipelines(*m_device, m_pipelineCache, 1, &pipeline_ci, nullptr, &m_pipeline);
    Q_ASSERT(!err);

    for ( uint32_t i = 0; i < pipeline_ci.stageCount; i++ ) {
        vkDestroyShaderModule(*m_device, shaderStages[i].module, nullptr);
    }
}

void QVulkanView::prepare_descriptor_pool() {
    DEBUG_ENTRY;

    VkDescriptorPoolSize type_counts[2] = {{},{}};
    type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_counts[0].descriptorCount = 1;
    type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_counts[1].descriptorCount = DEMO_TEXTURE_COUNT;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = nullptr;
    descriptor_pool.maxSets = 1;
    descriptor_pool.poolSizeCount = 2;
    descriptor_pool.pPoolSizes = type_counts;

    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(*m_device, &descriptor_pool, nullptr,
                                 &m_desc_pool);
    Q_ASSERT(!err);
}

void QVulkanView::prepare_framebuffers() {
    DEBUG_ENTRY;

    VkImageView attachments[2] = {{},{}};
    attachments[1] = m_depth.view;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;
    fb_info.renderPass = m_render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width = (uint32_t) width(); //FIXME unsigned cast
    fb_info.height = (uint32_t) height();
    fb_info.layers = 1;

    VkResult U_ASSERT_ONLY err;

    m_framebuffers.resize(m_buffers.count());

    for (int i = 0; i < m_buffers.count(); i++) {
        attachments[0] = m_buffers[i].view;
        err = vkCreateFramebuffer(*m_device, &fb_info, nullptr, &m_framebuffers[i]);
        Q_ASSERT(!err);
    }
}

void QVulkanView::prepare() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.queueFamilyIndex = m_graphics_queue_node_index;
    cmd_pool_info.flags = 0;

    err = vkCreateCommandPool(*m_device, &cmd_pool_info, nullptr,
                              &m_cmd_pool);
    Q_ASSERT(!err);


    prepare_buffers();
    prepare_depth();
    prepare_textures();

    prepare_descriptor_layout();
    prepare_render_pass();
    prepare_pipeline();

    VkCommandBufferAllocateInfo cmd_ai = {};
    cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_ai.pNext = nullptr;
    cmd_ai.commandPool = m_cmd_pool;
    cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_ai.commandBufferCount = 1;

    for (int i = 0; i < m_buffers.count(); i++) {
        err = vkAllocateCommandBuffers(*m_device, &cmd_ai, &m_buffers[i].cmd);
        Q_ASSERT(!err);
    }

    prepare_framebuffers();

    prepare_descriptor_pool();
    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    flush_init_cmd();

    m_current_buffer = 0;
    m_prepared = true;
}

void QVulkanView::resize_vk() {
    DEBUG_ENTRY;

    // Don't react to resize until after first initialization.
    if (!m_prepared) {
        return;
    }
    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
    m_prepared = false;

    for (int i = 0; i < m_framebuffers.count(); i++) {
        vkDestroyFramebuffer(*m_device, m_framebuffers[i], nullptr);
    }
    vkDestroyDescriptorPool(*m_device, m_desc_pool, nullptr);

    vkDestroyPipeline(*m_device, m_pipeline, nullptr);
    vkDestroyPipelineCache(*m_device, m_pipelineCache, nullptr);
    vkDestroyRenderPass(*m_device, m_render_pass, nullptr);
    vkDestroyPipelineLayout(*m_device, m_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(*m_device, m_desc_layout, nullptr);

    for (int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(*m_device, m_textures[i].view, nullptr);
        vkDestroyImage(*m_device, m_textures[i].image, nullptr);
        vkFreeMemory(*m_device, m_textures[i].mem, nullptr);
        vkDestroySampler(*m_device, m_textures[i].sampler, nullptr);
    }

    vkDestroyImageView(*m_device, m_depth.view, nullptr);
    vkDestroyImage(*m_device, m_depth.image, nullptr);
    vkFreeMemory(*m_device, m_depth.mem, nullptr);

    for (int i = 0; i < m_buffers.count(); i++) {
        vkDestroyImageView(*m_device, m_buffers[i].view, nullptr);
        m_buffers[i].view = nullptr;
        vkFreeCommandBuffers(*m_device, m_cmd_pool, 1, &m_buffers[i].cmd);
        m_buffers[i].cmd = nullptr;
    }
    vkDestroyCommandPool(*m_device, m_cmd_pool, nullptr);
    m_buffers.clear();

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    prepare();

    // TODO rethink calling down the inheritance from here
    // but we need to recreate the draw command buffers
    // maybe there is no need to destroy them in the first place...
    prepareDescriptorSet();
    for (int i = 0; i < m_buffers.count(); i++) {
        m_current_buffer = i;
        buildDrawCommand(m_buffers[i].cmd);
    }
}

void QVulkanView::resizeEvent(QResizeEvent *e)
{
    DEBUG_ENTRY;

    resize_vk();
    
    if(isVisible())
        draw();
    QWindow::resizeEvent(e);

    e->accept(); //FIXME?
}


void QVulkanView::init_vk_swapchain() {
    DEBUG_ENTRY;

    VkResult U_ASSERT_ONLY err;

// Create a WSI surface for the window:
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.hinstance = connection;
    createInfo.hwnd = window;

    err =
        vkCreateWin32SurfaceKHR(inst, &createInfo, nullptr, &surface);
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
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.connection = connection;
    createInfo.window = xcb_window;

    err = vkCreateXcbSurfaceKHR(m_inst, &createInfo, nullptr, &m_surface);

#endif // _WIN32
    Q_ASSERT(!err);

    auto queueProperties = m_gpu.queueProperties();
    // Iterate over each queue to learn whether it supports presenting:
    QVector<VkBool32> supportsPresent;
    supportsPresent.resize(queueProperties.count());
    for (int i = 0; i < supportsPresent.count(); i++) {
        m_inst.getPhysicalDeviceSurfaceSupport(m_gpu, i, m_surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (int i = 0; i < queueProperties.count(); i++) {
        if ((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
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
        for (int i = 0; i < queueProperties.count(); ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

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

    // Get the list of VkFormat's that are supported:
    auto getSurfForm = [this](uint32_t *c, VkSurfaceFormatKHR* d) {
        return m_inst.getPhysicalDeviceSurfaceFormats(m_gpu, m_surface, c, d);
    };
    auto surfFormats = getVk<VkSurfaceFormatKHR>(getSurfForm);
    Q_ASSERT(!surfFormats.isEmpty());

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (surfFormats.count() == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        m_format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        m_format = surfFormats[0].format;
    }
    m_color_space = surfFormats[0].colorSpace;

    m_curFrame = 0;
}
