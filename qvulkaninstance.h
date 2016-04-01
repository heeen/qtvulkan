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
private:
    VkPhysicalDevice m_device;
    VkPhysicalDeviceFeatures m_features;
    VkPhysicalDeviceProperties m_properties;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;

    VkPhysicalDeviceLimits m_limits;
    VkPhysicalDeviceSparseProperties m_sparseProperties;

    QVector<VkExtensionProperties> m_extensions;
    QVector<VkQueueFamilyProperties> m_queueProperties;
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
private:
    VkDevice m_device;
    void initFunctionPointers();
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
};

class QVulkanQueue {
public:
    QVulkanQueue(VkQueue queue);
    VkQueue queue() { return m_queue; }
private:
    VkQueue m_queue;
};

#endif // QVULKANINSTANCE_H
