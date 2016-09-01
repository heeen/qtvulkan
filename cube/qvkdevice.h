#ifndef QVKDEVICE_H
#define QVKDEVICE_H
#include <vulkan/vulkan.h>
#include "qvkutil.h"
#include "qvkinstance.h"
#include "qvkphysicaldevice.h"

class QVkDevice {
public:
    QVkDevice(QVkInstance& instance,
              QVkPhysicalDevice physicalDevice,
              uint32_t graphicsQueueIndex,
              QVulkanNames layers,
              QVulkanNames extensions);

    ~QVkDevice();
    Q_DISABLE_COPY(QVkDevice)

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
    void initFunctions(QVkInstance &instance);
    VkDevice m_device       {nullptr};
    QVkPhysicalDevice m_gpu;
    VkPhysicalDeviceMemoryProperties m_memory_properties    {};
    QVulkanNames m_layerNames;
    QVulkanNames m_extensionNames;
};

class QVkDeviceResource {
public:
    QVkDeviceResource(QSharedPointer<QVkDevice> dev)
        : m_device(dev)
    { }
    VkDevice device() { return *m_device; }
    QSharedPointer<QVkDevice> dev() { return m_device; }

    Q_DISABLE_COPY(QVkDeviceResource)
protected:
private:
    QSharedPointer<QVkDevice> m_device;
//    QVkDeviceResource& operator=(const QVkDeviceResource&) = delete;
};

#endif // QVKDEVICE_H
