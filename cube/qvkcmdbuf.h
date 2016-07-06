#ifndef QVKCMDBUF_H
#define QVKCMDBUF_H

#include "qvkutil.h"
#include <vulkan/vulkan.h>
#include <QColor>
#include "qvkimage.h"


class QVkCommandBufferRecorder {
public:
    QVkCommandBufferRecorder(
            VkCommandBuffer& cb,
            VkCommandBufferUsageFlags flags = 0)
        : m_cb(cb)
    {
        DEBUG_ENTRY;
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = flags;
        VkResult err = vkBeginCommandBuffer(cb, &info);
        Q_ASSERT(!err);
    }

    ~QVkCommandBufferRecorder() {
        DEBUG_ENTRY;
        VkResult err = vkEndCommandBuffer(m_cb);
        Q_ASSERT(!err);
    }

    QVkCommandBufferRecorder& viewport(QVkViewport viewport) {
        vkCmdSetViewport(m_cb, 0, 1, &viewport);
        return *this;
    }

    QVkCommandBufferRecorder& viewport(QVector<QVkViewport> rects) {
        vkCmdSetViewport(m_cb, 0, rects.size(), rects.data());
        return *this;
    }

    QVkCommandBufferRecorder& scissor(QVkRect scissor) {
        vkCmdSetScissor(m_cb, 0, 1, &scissor);
        return *this;
    }

    QVkCommandBufferRecorder& scissor(QVector<QVkRect> rects) {
        vkCmdSetScissor(m_cb, 0, rects.size(), rects.data());
        return *this;
    }

    QVkCommandBufferRecorder& draw(
            uint32_t vertices,
            uint32_t first_vertex = 0,
            uint32_t instances = 1,
            uint32_t first_instance = 0) {
        vkCmdDraw(m_cb, vertices, instances, first_vertex, first_instance);
        return *this;
    }

    QVkCommandBufferRecorder& beginRenderPass(
            VkRenderPass renderpass,
            VkFramebuffer framebuffer,
            QVkRect area,
            QColor clearColor) {
        DEBUG_ENTRY;

        VkClearValue clear_values[2] = {{},{}};
        clear_values[0].color.float32[0] = (float)clearColor.redF();
        clear_values[0].color.float32[1] = (float)clearColor.greenF();
        clear_values[0].color.float32[2] = (float)clearColor.blueF();
        clear_values[0].color.float32[3] = (float)clearColor.alphaF();
        clear_values[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rp_begin = {};
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = nullptr;
        rp_begin.renderPass = renderpass;
        rp_begin.framebuffer = framebuffer;
        rp_begin.renderArea = area;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;
        vkCmdBeginRenderPass(m_cb, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
        return *this;
    }

    QVkCommandBufferRecorder& endRenderPass() {
        DEBUG_ENTRY;
        vkCmdEndRenderPass(m_cb);
        return *this;
    }

    QVkCommandBufferRecorder& bindPipeline(VkPipeline pipeline) {
        DEBUG_ENTRY;
        vkCmdBindPipeline(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        return *this;
    }

    QVkCommandBufferRecorder& bindDescriptorSet(
               VkPipelineLayout layout,
               VkDescriptorSet*  descSet,
               uint32_t         dynamicOffsetCount = 0,
               const uint32_t*  pDynamicOffsets = nullptr
            ) {
        DEBUG_ENTRY;
        vkCmdBindDescriptorSets(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout,
                                0, 1, descSet,
                                dynamicOffsetCount, pDynamicOffsets);
        return *this;
    }

    QVkCommandBufferRecorder& bindDescriptorSets(
               VkPipelineLayout layout,
               uint32_t         firstSet,
                QVector<VkDescriptorSet> sets,
                QVector<uint32_t> dynamicOffsets
            ) {
        DEBUG_ENTRY;
        vkCmdBindDescriptorSets(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout, firstSet,
                                sets.size(), sets.data(),
                                dynamicOffsets.size(), dynamicOffsets.data()
                                );
        return *this;
    }

    QVkCommandBufferRecorder& pipelineBarrier(
            VkPipelineStageFlags                        srcStageMask,
            VkPipelineStageFlags                        dstStageMask,
            VkDependencyFlags                           dependencyFlags,
            uint32_t                                    memoryBarrierCount,
            const VkMemoryBarrier*                      pMemoryBarriers,
            uint32_t                                    bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
            uint32_t                                    imageMemoryBarrierCount,
            const VkImageMemoryBarrier*                 pImageMemoryBarriers
            ) {
        DEBUG_ENTRY;
        vkCmdPipelineBarrier(m_cb,
                             srcStageMask, dstStageMask,
                             dependencyFlags,
                             memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers,
                             imageMemoryBarrierCount, pImageMemoryBarriers
                             );
        return *this;
    }

    QVkCommandBufferRecorder& pipelineBarrier(
            VkPipelineStageFlags         srcStageMask,
            VkPipelineStageFlags         dstStageMask,
            VkDependencyFlags            dependencyFlags,
            const VkImageMemoryBarrier*  pImageMemoryBarrier
            ) {
        DEBUG_ENTRY;
        vkCmdPipelineBarrier(m_cb,
                             srcStageMask, dstStageMask,
                             dependencyFlags,
                             0, nullptr,
                             0, nullptr,
                             1, pImageMemoryBarrier);
        return *this;
    }

    QVkCommandBufferRecorder& transformImage(VkImage image,
                                             VkImageLayout fromLayout,
                                             VkImageLayout toLayout,
                                             VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                             VkAccessFlags srcAccess = 0,
                                             VkAccessFlags dstAccess = 0) {

        DEBUG_ENTRY;
        DBG("image %p from %i to %i\n", (void*)image, fromLayout, toLayout);
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.oldLayout = fromLayout;
        barrier.newLayout = toLayout;
        barrier.image = image;
        barrier.subresourceRange = {aspectMask, 0, 1, 0, 1};
        switch(toLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            /* Make sure any Copy or CPU writes to image are flushed */
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;
        default:
            barrier.dstAccessMask = 0; // FIXME
        }

        VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        vkCmdPipelineBarrier(m_cb, src_stages, dest_stages, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        return *this;
    }

private:
    VkCommandBuffer& m_cb;
};


class QVkCommandBuffer: public QVkDeviceResource {
public:
    QVkCommandBuffer(VkDevice dev, VkCommandPool pool)
        : QVkDeviceResource(dev)
        , m_pool(pool)
        , m_cmdbuf {}
    {
        DEBUG_ENTRY;
        VkResult err;
        VkCommandBufferAllocateInfo cmd_buf_ai = {};
        cmd_buf_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_ai.pNext = nullptr;
        cmd_buf_ai.commandPool = pool;
        cmd_buf_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_ai.commandBufferCount = 1;
        err = vkAllocateCommandBuffers(m_device, &cmd_buf_ai, &m_cmdbuf);
        Q_ASSERT(!err);
    }
    ~QVkCommandBuffer(){
        DEBUG_ENTRY;
        vkFreeCommandBuffers(m_device, m_pool, 1, &m_cmdbuf);
    }
    operator VkCommandBuffer& () { return m_cmdbuf; }

    QVkCommandBufferRecorder record(VkCommandBufferUsageFlags flags = 0) {
        return QVkCommandBufferRecorder(m_cmdbuf, flags);
    }

private:
    VkCommandPool m_pool;
    VkCommandBuffer m_cmdbuf;
};





class QVkQueue : public QVkDeviceResource {
public:
    QVkQueue(): QVkDeviceResource(nullptr) {
        //FIXME get rid of this default constructor
    }
    QVkQueue(VkDevice device, uint32_t familyIndex, uint32_t queueIndex = 0)
        : QVkDeviceResource(device) {
        vkGetDeviceQueue(m_device, familyIndex, queueIndex, &m_queue);
    }
    ~QVkQueue() {
    }

    void waitIdle() {
        VkResult err = vkQueueWaitIdle(m_queue);
        Q_ASSERT(!err);
    }

    void submit(VkCommandBuffer cmdbuf) {
        DEBUG_ENTRY;
        DBG("submitting buffer: %p\n", (void*)cmdbuf);
        VkFence nullFence = nullptr;
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdbuf;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        VkResult err = vkQueueSubmit(m_queue, 1, &submit_info, nullFence);
        Q_ASSERT(!err);
    }

    void present(VkSwapchainKHR swapchain, uint32_t imageIndex) {
         VkPresentInfoKHR present = {};
         present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
         present.pNext = nullptr;
         present.swapchainCount = 1;
         present.pSwapchains = &swapchain;
         present.pImageIndices = &imageIndex;
         present.waitSemaphoreCount = 0;
         present.pResults = nullptr;

         VkResult err = fpQueuePresentKHR(m_queue, &present);
         if (err == VK_ERROR_OUT_OF_DATE_KHR) {
             // swapchain is out of date (e.g. the window was resized) and
             // must be recreated:
             qWarning("FIXME FIXME FIXME resize_vk();!!!");
         } else if (err == VK_SUBOPTIMAL_KHR) {
             // swapchain is not as optimal as it could be, but the platform's
             // presentation engine will still present the image correctly.
         } else {
             Q_ASSERT(!err);
         }
    }

    operator VkQueue&() {
        return m_queue;
    }

    static PFN_vkQueuePresentKHR fpQueuePresentKHR;
private:
    VkQueue m_queue;
};

#endif /* QVKCMDBUF_H */
