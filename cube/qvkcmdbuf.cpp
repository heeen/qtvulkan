#include "qvkcmdbuf.h"

PFN_vkQueuePresentKHR QVkQueue::fpQueuePresentKHR = nullptr;

QVkCommandBuffer::QVkCommandBuffer(QSharedPointer<QVkDevice> dev, VkCommandPool pool)
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
    err = vkAllocateCommandBuffers(device(), &cmd_buf_ai, &m_cmdbuf);
    Q_ASSERT(!err);
}

QVkCommandBuffer::~QVkCommandBuffer(){
    DEBUG_ENTRY;
    vkFreeCommandBuffers(device(), m_pool, 1, &m_cmdbuf);
}

QVkCommandBufferRecorder QVkCommandBuffer::record(VkCommandBufferUsageFlags flags) {
    DEBUG_ENTRY;
    return QVkCommandBufferRecorder(m_cmdbuf, flags);
}

QVkCommandBufferRecorder::QVkCommandBufferRecorder(VkCommandBuffer &cb, VkCommandBufferUsageFlags flags)
    : m_cb(cb)
{
    DEBUG_ENTRY;
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = flags;
    VkResult err = vkBeginCommandBuffer(cb, &info);
    Q_ASSERT(!err);
}

QVkCommandBufferRecorder::~QVkCommandBufferRecorder() {
    DEBUG_ENTRY;
    VkResult err = vkEndCommandBuffer(m_cb);
    Q_ASSERT(!err);
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::viewport(QVkViewport viewport) {
    vkCmdSetViewport(m_cb, 0, 1, &viewport);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::viewport(QVector<QVkViewport> rects) {
    vkCmdSetViewport(m_cb, 0, rects.size(), rects.data());
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::scissor(QVkRect scissor) {
    vkCmdSetScissor(m_cb, 0, 1, &scissor);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::scissor(QVector<QVkRect> rects) {
    vkCmdSetScissor(m_cb, 0, rects.size(), rects.data());
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::draw(uint32_t vertices, uint32_t first_vertex, uint32_t instances, uint32_t first_instance) {
    vkCmdDraw(m_cb, vertices, instances, first_vertex, first_instance);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::beginRenderPass(VkRenderPass renderpass, VkFramebuffer framebuffer, QVkRect area, QColor clearColor) {
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

QVkCommandBufferRecorder &QVkCommandBufferRecorder::endRenderPass() {
    DEBUG_ENTRY;
    vkCmdEndRenderPass(m_cb);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::bindPipeline(VkPipeline pipeline) {
    DEBUG_ENTRY;
    vkCmdBindPipeline(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::bindDescriptorSet(VkPipelineLayout layout, VkDescriptorSet *descSet, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets) {
    DEBUG_ENTRY;
    vkCmdBindDescriptorSets(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            0, 1, descSet,
                            dynamicOffsetCount, pDynamicOffsets);
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::bindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet, QVector<VkDescriptorSet> sets, QVector<uint32_t> dynamicOffsets) {
    DEBUG_ENTRY;
    vkCmdBindDescriptorSets(m_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout, firstSet,
                            sets.size(), sets.data(),
                            dynamicOffsets.size(), dynamicOffsets.data()
                            );
    return *this;
}

QVkCommandBufferRecorder &QVkCommandBufferRecorder::pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
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

QVkCommandBufferRecorder &QVkCommandBufferRecorder::pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkImageMemoryBarrier *pImageMemoryBarrier) {
    DEBUG_ENTRY;
    vkCmdPipelineBarrier(m_cb,
                         srcStageMask, dstStageMask,
                         dependencyFlags,
                         0, nullptr,
                         0, nullptr,
                         1, pImageMemoryBarrier);
    return *this;
}
