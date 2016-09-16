#ifndef QVKSWAPCHAIN_H
#define QVKSWAPCHAIN_H

#include "qvkdevice.h"
#include <vulkan/vulkan.h>

struct SwapchainBuffers {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
};

class QVkSwapchain : public QVkDeviceResource
{
public:
    QVkSwapchain(QVkDeviceHandle device,
                 VkSurfaceKHR surface, uint32_t minImages,
                 VkFormat format,
                 VkColorSpaceKHR colorspace,
                 VkExtent2D extent,
                 VkPresentModeKHR presentMode,
                 VkSurfaceTransformFlagBitsKHR preTransform,
                 QSharedPointer<QVkSwapchain> oldSwapchain);
    ~QVkSwapchain();

    VkResult getSwapchainImages(uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages);
    VkResult acquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);

    VkSwapchainKHR& handle() { return m_swapchain; }
private:
    VkSwapchainKHR m_swapchain {nullptr};
};

#endif // QVKSWAPCHAIN_H
