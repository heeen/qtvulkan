#ifndef QVKIMAGE_H
#define QVKIMAGE_H
#include "qvkutil.h"
#include "vulkan/vulkan.h"

class QVkImage : public QVkDeviceResource
{
public:
    QVkImage(QVkDevice device, VkImage image)
        : QVkDeviceResource(device)
        , m_image(image) // FIXME create here
        , m_layout(VK_IMAGE_LAYOUT_UNDEFINED)
    {
    DEBUG_ENTRY;

    }

    ~QVkImage() {
    DEBUG_ENTRY;

    }

    operator VkImage&() {
        return m_image;
    }

    VkImageLayout layout() {
        return m_layout;
    }

private:
    VkImage m_image;
    VkImageLayout m_layout;
};

#endif // QVKIMAGE_H
