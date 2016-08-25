#ifndef QVKDEVICE_H
#define QVKDEVICE_H
#include <vulkan/vulkan.h>


class QVkPhysicalDevice {
public:
    ~QVkPhysicalDevice() {

    }
    operator VkPhysicalDevice*() {
        return &m_device;
    }
    operator VkPhysicalDevice() {
        return m_device;
    }
private:
    QVkPhysicalDevice() {

    }
    VkPhysicalDevice m_device;
};


class QVkDevice {
public:
    QVkDevice(QVkPhysicalDevice physicalDevice, uint32_t graphicsQueueIndex, QVector<const char*> layers, QVector<const char*> extensions);
    ~QVkDevice();

    int32_t memoryType(uint32_t typeBits, VkFlags requirements);

    operator VkDevice&() {
        return m_device;
    }

    inline VkResult createSwapChain(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
        return fpCreateSwapchainKHR(m_device, pCreateInfo, pAllocator, pSwapchain);
    }

    inline void destroySwapchain(VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) {
        fpDestroySwapchainKHR(m_device, swapchain, pAllocator);
    }

    inline VkResult getSwapChainImages(VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) {
        return fpGetSwapchainImagesKHR(m_device, swapchain, pSwapchainImageCount, pSwapchainImages);
    }

    inline VkResult acquireNextImage(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) {
        return fpAcquireNextImageKHR(m_device, swapchain, timeout, semaphore, fence, pImageIndex );
    }

    inline VkResult queuePresent(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
        return fpQueuePresentKHR(queue, pPresentInfo);
    }

    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR       {nullptr};
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR     {nullptr};
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR {nullptr};
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR     {nullptr};
    PFN_vkQueuePresentKHR fpQueuePresentKHR             {nullptr};

private:
    void initFunctions();
    VkDevice m_device       {nullptr};
    VkPhysicalDeviceMemoryProperties m_memory_properties    {};
};

#endif // QVKDEVICE_H
