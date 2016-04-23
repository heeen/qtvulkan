#ifndef QVULKANINSTANCE_H
#define QVULKANINSTANCE_H


#include "vulkan/vulkan.h"
#include <QObject>
#include <QVector>

class QCoreApplication;
class QVulkanPhysicalDevice;

class QVulkanInstance: public QObject {
    friend class QVulkanPhysicalDevice;
    friend class QVulkanDevice;
public:
    QVulkanInstance(bool validate);
    ~QVulkanInstance(); //TODO
    void init(QVector<const char*> enableLayers, QVector<const char*> enableExtensions);
    QStringList layerNames();
    QStringList extensionNames();

    const QVector<VkLayerProperties>& layers() const {
        return m_layers;
    }

    const QVector<VkExtensionProperties>& extensions() const {
        return m_extensions;
    }

    const QVector<VkPhysicalDevice>& devices() const {
        return m_devices;
    }

    VkInstance& data()  { return m_instance; }
private:
    VkInstance m_instance;
    QVector<VkLayerProperties> m_layers;
    QVector<VkExtensionProperties> m_extensions;
    QVector<VkPhysicalDevice> m_devices;

    void initLayers();
    void initExtensions();
    void initDevices();
    void initFunctionPointers();
    void initDebug();
    bool m_validate;

    static PFN_vkGetPhysicalDeviceSurfaceSupportKHR
        fpGetPhysicalDeviceSurfaceSupportKHR;
    static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
        fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
        fpGetPhysicalDeviceSurfaceFormatsKHR;
    static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
        fpGetPhysicalDeviceSurfacePresentModesKHR;
    static PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    static PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    static PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    static PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    static PFN_vkQueuePresentKHR fpQueuePresentKHR;
    static PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr;

    static PFN_vkCreateDebugReportCallbackEXT fpCreateDebugReportCallbackEXT;
    static PFN_vkDestroyDebugReportCallbackEXT fpDestroyDebugReportCallbackEXT;
    VkDebugReportCallbackEXT m_msgCallback;
    static PFN_vkDebugReportMessageEXT fpDebugReportMessageEXT;
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
            uint64_t srcObject, size_t location, int32_t msgCode,
            const char *pLayerPrefix, const char *pMsg, void *pUserData);
};

class QVulkanPhysicalDevice {
public:
    QVulkanPhysicalDevice(const VkPhysicalDevice& device);
    ~QVulkanPhysicalDevice(); //TODO
    const QVector<VkExtensionProperties>& extensions() const {
        return m_extensions;
    }
    QStringList extensionNames();
    QVector<VkQueueFamilyProperties> queueProperties() const {
        return m_queueProperties;
    }

    uint32_t hasGraphicsQueue() const;
    bool queueSupportsPresent(uint32_t index, VkSurfaceKHR surf);
    VkPhysicalDevice& device() { return m_device; }
    QVector<VkSurfaceFormatKHR> surfaceFormats(VkSurfaceKHR surface);
    QVector<VkPresentModeKHR> presentModes() { return m_presentModes; };

    void init(VkSurfaceKHR surface);
private:
    VkPhysicalDevice m_device;
    VkPhysicalDeviceFeatures m_features;
    VkPhysicalDeviceProperties m_properties;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;

    VkPhysicalDeviceLimits m_limits;
    VkPhysicalDeviceSparseProperties m_sparseProperties;

    QVector<VkExtensionProperties> m_extensions;
    QVector<VkQueueFamilyProperties> m_queueProperties;

    QVector<VkPresentModeKHR> m_presentModes;
    VkSurfaceCapabilitiesKHR m_surfaceCapabilities;
};

class QVulkanDevice {
public:
    QVulkanDevice(
            VkPhysicalDevice& phyDevice,
            uint32_t queue_index,
            const QVector<const char *> &layers,
            const QVector<const char *> &extensions);
    ~QVulkanDevice(); // TODO
    VkQueue getQueue(uint32_t index);
    VkCommandPool createCommandPool(int32_t gfxQueueIndex);
    VkSwapchainKHR createSwapChain();
private:
    VkDevice m_device;
    void initFunctionPointers();
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
};


class QVulkanCommandPool {
public:
    QVulkanCommandPool(VkCommandPool pool);
    ~QVulkanCommandPool();
    VkCommandPool m_pool;
};

class QVulkanQueue {
public:
    QVulkanQueue(VkQueue queue);
    VkQueue queue() { return m_queue; }
private:
    VkQueue m_queue;
};

class QVulkanTexture {
private:
    VkSampler m_sampler;

    VkImage m_image;
    VkImageLayout m_imageLayout;

    VkMemoryAllocateInfo m_memAllocInfo;
    VkDeviceMemory m_mem;
    VkImageView m_view;
    int32_t m_width, m_height;
};

class QVulkanSwapchainBuffers {
private:
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
};

#endif // QVULKANINSTANCE_H
