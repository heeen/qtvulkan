#include "qvkdevice.h"

static PFN_vkGetDeviceProcAddr g_gdpa = nullptr;

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
    {                                                                          \
        if (!g_gdpa)                                                           \
            g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(           \
                m_inst, "vkGetDeviceProcAddr");                            \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                 \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint,      \
                     "vkGetDeviceProcAddr Failure");                           \
        }                                                                      \
    }

QVkDevice::QVkDevice(QVkPhysicalDevice physicalDevice, uint32_t graphicsQueueIndex, QVector<const char *> layers, QVector<const char *> extensions) {

    VkResult err;
    Q_ASSERT((VkPhysicalDevice) physicalDevice) ;
    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queue = {};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.pNext = nullptr;
    queue.queueFamilyIndex = graphicsQueueIndex;
    queue.queueCount = 1;
    queue.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_ci = {};
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.pNext = nullptr;
    device_ci.queueCreateInfoCount = 1;
    device_ci.pQueueCreateInfos = &queue;
    device_ci.enabledLayerCount = layers.count();
    device_ci.ppEnabledLayerNames =  layers.data();
    device_ci.enabledExtensionCount = extensions.count();
    device_ci.ppEnabledExtensionNames = extensions.data();
    device_ci.pEnabledFeatures = nullptr; // If specific features are required, pass them in here

    err = vkCreateDevice(physicalDevice, &device_ci, nullptr, &m_device);
    Q_ASSERT(!err);
    initFunctions();

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memory_properties);
}

QVkDevice::~QVkDevice() {

}

int32_t QVkDevice::memoryType(uint32_t typeBits, VkFlags requirements) {

    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((m_memory_properties.memoryTypes[i].propertyFlags &
                 requirements) == requirements) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return -1;
}

void QVkDevice::initFunctions() {
    GET_DEVICE_PROC_ADDR(m_device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(m_device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(m_device, QueuePresentKHR);
}
