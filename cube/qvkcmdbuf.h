#ifndef QVKCMDBUF_H
#define QVKCMDBUF_H

#include "qvkutil.h"
#include <vulkan/vulkan.h>
#include <QColor>
#include "qvkimage.h"
#include "qvulkanbuffer.h"

class QVkCommandBufferRecorder {
public:
    QVkCommandBufferRecorder(
            VkCommandBuffer& cb,
            VkCommandBufferUsageFlags flags = 0);

    ~QVkCommandBufferRecorder();

    QVkCommandBufferRecorder& viewport(QVkViewport viewport);

    QVkCommandBufferRecorder& viewport(QVector<QVkViewport> rects);

    QVkCommandBufferRecorder& scissor(QVkRect scissor);

    QVkCommandBufferRecorder& scissor(QVector<QVkRect> rects);

    QVkCommandBufferRecorder& draw(
            uint32_t vertices,
            uint32_t first_vertex = 0,
            uint32_t instances = 1,
            uint32_t first_instance = 0);

    QVkCommandBufferRecorder& beginRenderPass(
            VkRenderPass renderpass,
            VkFramebuffer framebuffer,
            QVkRect area,
            QColor clearColor);

    QVkCommandBufferRecorder& endRenderPass();

    QVkCommandBufferRecorder& bindPipeline(VkPipeline pipeline);

    QVkCommandBufferRecorder& bindDescriptorSet(
               VkPipelineLayout layout,
               VkDescriptorSet*  descSet,
               uint32_t         dynamicOffsetCount = 0,
               const uint32_t*  pDynamicOffsets = nullptr
            );

    QVkCommandBufferRecorder& bindDescriptorSets(
               VkPipelineLayout layout,
               uint32_t         firstSet,
                QVector<VkDescriptorSet> sets,
                QVector<uint32_t> dynamicOffsets
            );

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
            );

    QVkCommandBufferRecorder& pipelineBarrier(
            VkPipelineStageFlags         srcStageMask,
            VkPipelineStageFlags         dstStageMask,
            VkDependencyFlags            dependencyFlags,
            const VkImageMemoryBarrier*  pImageMemoryBarrier
            );

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

    QVkCommandBufferRecorder& copyBuffer(QVkStagingBuffer& src, QVkDeviceBuffer& dst) {
    DEBUG_ENTRY;
        VkBufferCopy copyRegion = {};
        Q_ASSERT(src.size() == dst.size());
        copyRegion.size = src.size();
        vkCmdCopyBuffer(m_cb, src.buffer(), dst.buffer(), 1, &copyRegion);
        return *this;
    }

private:
    VkCommandBuffer& m_cb;
};


class QVkCommandBuffer: public QVkDeviceResource {
public:
    QVkCommandBuffer(QVkDeviceHandle dev, VkCommandPool pool);
    ~QVkCommandBuffer();
    operator VkCommandBuffer& () { return m_cmdbuf; }

    QVkCommandBufferRecorder record(VkCommandBufferUsageFlags flags = 0);

private:
    VkCommandPool m_pool;
    VkCommandBuffer m_cmdbuf;
};

class QVkQueue : public QVkDeviceResource {
public:
/*    QVkQueue(): QVkDeviceResource(nullptr) {
        //FIXME get rid of this default constructor
    }*/
    QVkQueue(QVkDeviceHandle dev, uint32_t familyIndex, uint32_t queueIndex = 0)
        : QVkDeviceResource(dev) {
        vkGetDeviceQueue(device(), familyIndex, queueIndex, &m_queue);
        DEBUG_ENTRY;
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
        VkResult err = vkQueueSubmit(m_queue, 1, &submit_info, nullptr);
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
