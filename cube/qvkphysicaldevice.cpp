#include "qvkphysicaldevice.h"
#include "qvkutil.h"

VkPhysicalDeviceProperties QVkPhysicalDevice::properties() {
    DEBUG_ENTRY;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_device, &props);
    return props;
}

VkPhysicalDeviceFeatures QVkPhysicalDevice::features() {
    DEBUG_ENTRY;
    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(m_device, &physDevFeatures);
    return physDevFeatures;
}

QVector<VkQueueFamilyProperties> QVkPhysicalDevice::queueProperties() const
{
    DEBUG_ENTRY;
    return m_queueProperties;
}

int QVkPhysicalDevice::graphicsQueueIndex() {
    DEBUG_ENTRY;
    // Find a queue that supports gfx
    int gfx_queue_idx = 0;
    for (gfx_queue_idx = 0; gfx_queue_idx < m_queueProperties.count();
         gfx_queue_idx++) {
        if (m_queueProperties[gfx_queue_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return gfx_queue_idx;
    }
    return -1;
}

QVkPhysicalDevice::QVkPhysicalDevice(VkPhysicalDevice device): m_device(device) {
    DEBUG_ENTRY;
    DBG("phys dev %p\n", (void*)m_device);
    auto getPhysDevQueFamProp = [this](uint32_t* c, VkQueueFamilyProperties* d) {
        vkGetPhysicalDeviceQueueFamilyProperties(m_device, c, d); return VK_SUCCESS;
    };
    m_queueProperties = getVk<VkQueueFamilyProperties>(getPhysDevQueFamProp);
}
