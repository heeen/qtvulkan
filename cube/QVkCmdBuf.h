#ifndef QVKCMDBUF_H
#define QVKCMDBUF_H

#include "QVkUtil.h"
#include <vulkan/vulkan.h>

class QVkCommandBuffer: public QVkDeviceResource {
public:
    QVkCommandBuffer(VkDevice dev, VkCommandPool pool)
        : QVkDeviceResource(dev)
        , m_pool(pool)
        , m_cmdbuf {}
    {
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
        vkFreeCommandBuffers(m_device, m_pool, 1, &m_cmdbuf);
    }
    operator VkCommandBuffer& () { return m_cmdbuf; }
private:
    VkCommandPool m_pool;
    VkCommandBuffer m_cmdbuf;
    QVkCommandBuffer& operator=(const QVkCommandBuffer&) = delete;
    QVkCommandBuffer(const QVkCommandBuffer&) = delete;
};


class QVkCommandBufferRecorder {
public:
    QVkCommandBufferRecorder(
            VkCommandBuffer& cb,
            VkCommandBufferUsageFlags flags = 0)
        : m_cb(cb)
    {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = flags;
        VkResult err = vkBeginCommandBuffer(cb, &info);
        Q_ASSERT(!err);
    }

    ~QVkCommandBufferRecorder() {
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
            QVkRect area) {

        VkClearValue clear_values[2] = {{},{}};
        clear_values[0].color.float32[0] = 0.2f;
        clear_values[0].color.float32[1] = 0.2f;
        clear_values[0].color.float32[2] = 0.4f;
        clear_values[0].color.float32[3] = 0.2f;
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
        vkCmdEndRenderPass(m_cb);
        return *this;
    }

    QVkCommandBufferRecorder& bindPipeline(VkPipeline pipeline) {
        vkCmdBindPipeline(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        return *this;
    }

    QVkCommandBufferRecorder& bindDescriptorSet(
               VkPipelineLayout layout,
               VkDescriptorSet*  descSet,
               uint32_t         dynamicOffsetCount = 0,
               const uint32_t*  pDynamicOffsets = nullptr
            ) {
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
        vkCmdPipelineBarrier(m_cb,
                             srcStageMask, dstStageMask,
                             dependencyFlags,
                             0, nullptr,
                             0, nullptr,
                             1, pImageMemoryBarrier);
        return *this;
    }

private:
    VkCommandBuffer& m_cb;

};
#endif /* QVKCMDBUF_H */
