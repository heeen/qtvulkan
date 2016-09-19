#ifndef QVKIMAGE_H
#define QVKIMAGE_H
#include "qvkdevice.h"
#include "vulkan/vulkan.h"


class QVkImage : public QVkDeviceResource
{
public:
    QVkImage(QVkDeviceHandle dev, VkFormat format,
             uint32_t width, uint32_t height,
             VkImageUsageFlags usage,
             uint32_t mipLevels = 1,
             uint32_t arrayLayers = 1,
             VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
             VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
             VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED)
        : QVkDeviceResource(dev)
        , m_format(format)
        , m_layout(initialLayout)
    {
        DEBUG_ENTRY;
        VkImageCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = m_format;
        ci.extent.width = width;
        ci.extent.height = height;
        ci.extent.depth = 1;
        ci.mipLevels = mipLevels;
        ci.arrayLayers = arrayLayers;
        ci.samples = samples;
        ci.tiling = tiling;
        ci.usage = usage;
        ci.flags = 0;
        ci.initialLayout = m_layout;
        VkResult err;
        err = vkCreateImage(vkDevice(), &ci, nullptr, &m_image);
        Q_ASSERT(!err);
    }

    ~QVkImage();

    operator VkImage&() {
        return m_image;
    }

    VkImageLayout layout() {
        return m_layout;
    }

    VkMemoryRequirements requirements() {
        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(vkDevice(), m_image, &mem_reqs);
        return mem_reqs;
    }

    VkFormat format() const {
        return m_format;
    }

private:
    VkImage m_image;
    VkFormat m_format;
    VkImageLayout m_layout;
};

class QVkDepthImage: public QVkImage {
public:
    QVkDepthImage(QVkDeviceHandle dev, uint32_t width, uint32_t height)
        : QVkImage(dev, VK_FORMAT_D16_UNORM, width, height,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    }
};

class QVkImageView : public QVkDeviceResource {
public:
    QVkImageView(QVkDeviceHandle dev, QVkImage* image)
        : QVkDeviceResource(dev) {
        VkImageViewCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.pNext = nullptr;
        ci.image = *image;
        ci.format = image->format();
        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount = 1;
        ci.flags = 0;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkResult err = vkCreateImageView(vkDevice(), &ci, nullptr, &m_view);
        Q_ASSERT(!err);
    }
    ~QVkImageView() {
        vkDestroyImageView(vkDevice(), m_view, nullptr);
    }

private:
    VkImageView m_view;
};


#endif // QVKIMAGE_H
