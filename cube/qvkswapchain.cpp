#include "qvkswapchain.h"

QVkSwapchain::QVkSwapchain(QVkDeviceHandle device,
                           VkSurfaceKHR surface,
                           uint32_t minImages,
                           VkFormat format,
                           VkColorSpaceKHR colorspace,
                           VkExtent2D extent,
                           VkPresentModeKHR presentMode,
                           VkSurfaceTransformFlagBitsKHR preTransform,
                           QSharedPointer<QVkSwapchain> oldSwapchain
                           )
    : QVkDeviceResource(device)
{
    Q_ASSERT(device);
    Q_ASSERT(surface);

    VkResult err;
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = nullptr;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = minImages;
    swapchain_ci.imageFormat = format;
    swapchain_ci.imageColorSpace = colorspace;
    swapchain_ci.imageExtent.width = extent.width;
    swapchain_ci.imageExtent.height = extent.height;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.preTransform = preTransform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;
    swapchain_ci.presentMode = presentMode;
    swapchain_ci.oldSwapchain = oldSwapchain ? oldSwapchain->handle() : nullptr;
    swapchain_ci.clipped = true;

    err = device->createSwapchain(&swapchain_ci, nullptr, &m_swapchain);
    Q_ASSERT(!err);
}

QVkSwapchain::~QVkSwapchain()
{
    device()->destroySwapchain(m_swapchain, nullptr);
}

VkResult QVkSwapchain::getSwapchainImages(uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    return device()->getSwapchainImages(m_swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult QVkSwapchain::acquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    return device()->acquireNextImage(m_swapchain, timeout,semaphore, fence, pImageIndex);
}
