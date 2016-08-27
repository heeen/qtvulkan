#ifndef QVKINSTANCE_H
#define QVKINSTANCE_H
#define VK_USE_PLATFORM_XCB_KHR 1
#include "qvkutil.h"
#include "qvkphysicaldevice.h"
#include <vulkan/vulkan.h>

class QVkInstance {
public:
    QVkInstance();

    ~QVkInstance();

    QVkPhysicalDevice device(uint32_t index);

    inline VkResult getPhysicalDeviceSurfaceSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) {
        return fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
    }

    inline VkResult getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
        return fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
    }

    inline VkResult getPhysicalDeviceSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) {
        return fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
    }

    inline VkResult getPhysicalDeviceSurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) {
        return fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
    }

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR           {nullptr};
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR {nullptr};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR           {nullptr};
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR {nullptr};

    operator VkInstance() {
        return m_instance;
    }

    virtual VkBool32 debugMessage(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char *pLayerPrefix, const char *pMsg);

private:
    void initFunctions();
    VkInstance m_instance { nullptr };
    bool m_validate { true };
    QVulkanNames m_layerNames;
    QVulkanNames m_extensionNames;

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    s_dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
            uint64_t srcObject, size_t location, int32_t msgCode,
            const char *pLayerPrefix, const char *pMsg, void *pUserData);

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback  {nullptr};
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback  {nullptr};
    VkDebugReportCallbackEXT msg_callback           {nullptr};
    PFN_vkDebugReportMessageEXT DebugReportMessage  {nullptr};
};

#endif // QVKINSTACE_H
