#include <QDebug>
#include "qvkdevice.h"

static PFN_vkGetDeviceProcAddr g_gdpa = nullptr;

#define GET_DEVICE_PROC_ADDR(instance, dev, entrypoint)                                  \
    {                                                                          \
        if (!g_gdpa)                                                           \
            g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(           \
                instance, "vkGetDeviceProcAddr");                            \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                 \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint,      \
                     "vkGetDeviceProcAddr Failure");                           \
        }                                                                      \
    }

QVkDevice::QVkDevice(QVkInstance instance,
                     QVkPhysicalDevice physicalDevice,
                     uint32_t graphicsQueueIndex,
                     QVulkanNames requestedLayers,
                     QVulkanNames requestedExtensions)
    :m_gpu(physicalDevice) {

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
    device_ci.enabledLayerCount = requestedLayers.count();
    device_ci.ppEnabledLayerNames =  requestedLayers.data();
    device_ci.enabledExtensionCount = requestedExtensions.count();
    device_ci.ppEnabledExtensionNames = requestedExtensions.data();
    device_ci.pEnabledFeatures = nullptr; // If specific features are required, pass them in here

    err = vkCreateDevice(physicalDevice, &device_ci, nullptr, &m_device);
    Q_ASSERT(!err);
    initFunctions(instance);

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memory_properties);

    // Look for validation layers
    if(/*m_validate*/ true) {
        auto getDevLayers = [this](uint32_t* c, VkLayerProperties* d) { return vkEnumerateDeviceLayerProperties(m_gpu, c, d); };
        auto foundLayers= getVk<VkLayerProperties>(getDevLayers);

        qDebug()<<"found device Layers:"<<getLayerNames(foundLayers);

        if(foundLayers.isEmpty()) {
            qFatal("validation requested, but no device validation layers found!");
        }
        if(!containsAllLayers(foundLayers, requestedLayers)) {
            qFatal("validation requested, but not all device validation layers found!");
        }
    }

    /* Look for device extensions */
    VkBool32 swapchainExtFound = 0;
    m_extensionNames.clear();

    auto getDevExt = [this](uint32_t* c, VkExtensionProperties* d) {
        return vkEnumerateDeviceExtensionProperties(m_gpu, nullptr/*layerName?*/, c, d);
    };
    auto foundExtensions = getVk<VkExtensionProperties>(getDevExt);

    if(foundExtensions.isEmpty()) {
        qFatal("no device extensions found!");
    }

    for (const auto& ext: foundExtensions) {
        qDebug()<<"device extension"<<ext.extensionName;
        if (!strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            swapchainExtFound = 1;
            m_extensionNames << VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }
    }

    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
                 "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
}

QVkDevice::~QVkDevice() {
    vkDestroyDevice(m_device, nullptr);
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

void QVkDevice::initFunctions(QVkInstance instance) {
    GET_DEVICE_PROC_ADDR(instance, m_device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(instance, m_device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(instance, m_device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(instance, m_device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(instance, m_device, QueuePresentKHR);
}

