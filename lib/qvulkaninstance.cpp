#include "qvulkaninstance.h"
#include <QDebug>
#include <QCoreApplication>

VkBool32 checkLayers(QVector<const char*>& check_names, QVector<VkLayerProperties> layers) {
    foreach(const char* name, check_names) {
        VkBool32 found = 0;
        foreach (const VkLayerProperties& layer, layers) {
            if (strcmp(name, layer.layerName) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            qDebug() << "Cannot find layer:" << name;
            return 0;
        }
    }
    return 1;
}

QVulkanInstance::QVulkanInstance(bool validate) {
    m_validate = validate;

    QVector<const char*> validationLayers;
    validationLayers
        << "VK_LAYER_GOOGLE_threading"
//      << "VK_LAYER_LUNARG_param_checker"
        << "VK_LAYER_LUNARG_parameter_validation"
        << "VK_LAYER_LUNARG_device_limits"
        << "VK_LAYER_LUNARG_object_tracker"
        << "VK_LAYER_LUNARG_image"
        << "VK_LAYER_LUNARG_core_validation"
        << "VK_LAYER_LUNARG_swapchain"
        << "VK_LAYER_GOOGLE_unique_objects";


    /* Look for validation layers */
    VkBool32 validation_found = 0;
    initLayers();
    initExtensions();

    if (m_validate) {
        validation_found = checkLayers(validationLayers, m_layers);
    }

    if (m_validate && !validation_found) {
        qFatal("vkEnumerateInstanceLayerProperties failed to find"
                 "required validation layer.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n"
                 "vkCreateInstance Failure");
    }
}

QVulkanInstance::~QVulkanInstance() {
    if (m_validate) {
        fpDestroyDebugReportCallbackEXT(m_instance, m_msgCallback, NULL);
    }
    vkDestroyInstance(m_instance, NULL);
}

void QVulkanInstance::init(QVector<const char*> enableLayers, QVector<const char*> enableExtension)
{
    QCoreApplication *qapp = QCoreApplication::instance();
    VkResult err;
    VkApplicationInfo app;
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr;
    app.pApplicationName = qapp->applicationName().toLocal8Bit();
    app.applicationVersion = qapp->applicationVersion().toUInt();
    app.pEngineName = qapp->applicationName().toLocal8Bit();
    app.engineVersion = qapp->applicationVersion().toUInt();
    app.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo inst_info;
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext= nullptr;
    inst_info.pApplicationInfo= &app;

    inst_info.enabledLayerCount = enableLayers.size();
    inst_info.ppEnabledLayerNames = enableLayers.constData();

    inst_info.enabledExtensionCount= enableExtension.size();
    inst_info.ppEnabledExtensionNames = enableExtension.constData();

    err = vkCreateInstance(&inst_info, nullptr, &m_instance);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        qDebug("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n"
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        qDebug("Cannot find a specified extension library"
                 ".\nMake sure your layers path is set appropriately.\n"
                 "vkCreateInstance Failure");
    } else if (err) {
        qDebug("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                 "installable client driver (ICD) installed?\nPlease look at "
                 "the Getting Started guide for additional information.\n"
                 "vkCreateInstance Failure");
    }

    initFunctionPointers();
    initDevices();
}

QStringList QVulkanInstance::layerNames()
{
    QStringList result;
    foreach (const VkLayerProperties& layer, m_layers) {
        result<<QString::fromLatin1(layer.layerName);
    }
    return result;
}

QStringList QVulkanInstance::extensionNames()
{
    QStringList result;
    foreach (const VkExtensionProperties& ext, m_extensions) {
        result<<QString::fromLatin1(ext.extensionName);
    }
    return result;
}

void QVulkanInstance::initLayers()
{
    uint32_t count = 0;
    VkResult err;
    err = vkEnumerateInstanceLayerProperties(&count, nullptr);
    Q_ASSERT(!err);

    if (count > 0) {
        m_layers.resize(count);
        err = vkEnumerateInstanceLayerProperties(&count, m_layers.data());
        Q_ASSERT(!err);
        Q_ASSERT(count == m_layers.size());

        foreach (const VkLayerProperties& layer, m_layers) {
            qDebug()<<"layer:"<<layer.layerName<<layer.description;
        }
    }
}

void QVulkanInstance::initExtensions()
{
    uint32_t count = 0;
    VkResult err;
    err = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    Q_ASSERT(!err);

    if (count > 0) {
        m_extensions.resize(count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &count, m_extensions.data());
        Q_ASSERT(!err);
        Q_ASSERT(count == m_extensions.size());

        foreach (const VkExtensionProperties& ext, m_extensions) {
            qDebug()<<"instance extension:"<<ext.extensionName<<ext.specVersion;
        }
    }
}

void QVulkanInstance::initDevices()
{
    uint32_t count = 0;
    VkResult err;
    err = vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    Q_ASSERT(!err);

    if (count > 0) {
        m_devices.resize(count);
        err = vkEnumeratePhysicalDevices(m_instance, &count, m_devices.data());
        Q_ASSERT(!err);
        Q_ASSERT(count == m_devices.size());
    }
    qDebug()<<"devices:"<<m_devices.count();
}


#define GET_INSTANCE_PROC_ADDR(entrypoint)                               \
    {                                                                          \
        fp##entrypoint =                                                       \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(m_instance, "vk" #entrypoint); \
        if (fp##entrypoint == nullptr) {                                    \
            qFatal("vkGetInstanceProcAddr failed to find vk" #entrypoint);     \
        }                                                                      \
    }

void QVulkanInstance::initFunctionPointers()
{
    //FIXME only load when extension exists in list
    GET_INSTANCE_PROC_ADDR(GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(GetPhysicalDeviceSurfacePresentModesKHR);
    GET_INSTANCE_PROC_ADDR(GetSwapchainImagesKHR);
    GET_INSTANCE_PROC_ADDR(GetDeviceProcAddr);
    GET_INSTANCE_PROC_ADDR(CreateDebugReportCallbackEXT);
    GET_INSTANCE_PROC_ADDR(DestroyDebugReportCallbackEXT);
    GET_INSTANCE_PROC_ADDR(DebugReportMessageEXT);
}

void QVulkanInstance::initDebug()
{
    VkResult err;
    VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
    dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    dbgCreateInfo.pNext = NULL;
    dbgCreateInfo.pfnCallback = dbgFunc;
    dbgCreateInfo.pUserData = NULL;
    dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    err = fpCreateDebugReportCallbackEXT(m_instance, &dbgCreateInfo, NULL,
                                         &m_msgCallback);
    switch (err) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        qFatal("CreateDebugReportCallback: out of host memory\n",
               "CreateDebugReportCallback Failure");
        break;
    default:
        qFatal("CreateDebugReportCallback: unknown failure\n",
               "CreateDebugReportCallback Failure");
        break;
    }
}

VkBool32 QVulkanInstance::dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        qDebug()<<"ERROR: ["<<pLayerPrefix<<"] Code"<<msgCode<<":"<<pMsg;
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        // We know that we're submitting queues without fences, ignore this
        // warning
    /*    if (strstr(pMsg,
                   "vkQueueSubmit parameter, VkFence fence, is null pointer")) {
            return false;
        }*/
        qDebug()<<"WARNING: ["<<pLayerPrefix<<"] Code"<<msgCode<<":"<<pMsg;
    } else {
        return false;
    }

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

QVulkanPhysicalDevice::QVulkanPhysicalDevice(const VkPhysicalDevice& device)
    :m_device(device)
{
    uint32_t count = 0;
    VkResult err;
    err = vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, nullptr);
    Q_ASSERT(!err);

    if (count > 0) {
        m_extensions.resize(count);
        err = vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, m_extensions.data());
        Q_ASSERT(!err);
        Q_ASSERT(count == m_extensions.size());

        foreach (const VkExtensionProperties& ext, m_extensions) {
            qDebug()<<"device extension:"<<ext.extensionName<<ext.specVersion;
        }
    }
    vkGetPhysicalDeviceFeatures(m_device, &m_features);
    vkGetPhysicalDeviceProperties(m_device, &m_properties);
    vkGetPhysicalDeviceMemoryProperties(m_device, &m_memoryProperties);

    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, nullptr);
    Q_ASSERT(count > 0);

    m_queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, m_queueProperties.data());
}

QVulkanPhysicalDevice::~QVulkanPhysicalDevice()
{

}

QStringList QVulkanPhysicalDevice::extensionNames()
{
    QStringList result;
    foreach (const VkExtensionProperties& ext, m_extensions) {
        result<<QString::fromLatin1(ext.extensionName);
    }
    return result;
}

uint32_t QVulkanPhysicalDevice::hasGraphicsQueue() const
{
    uint32_t count = 0;
    foreach (const VkQueueFamilyProperties& props, m_queueProperties) {
        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            count ++;
        }
    }
    return count;
}

bool QVulkanPhysicalDevice::queueSupportsPresent(uint32_t index, VkSurfaceKHR surf)
{
    VkBool32 supported;
    QVulkanInstance::fpGetPhysicalDeviceSurfaceSupportKHR(m_device, index, surf, &supported);
    return supported;
}

#define GET_DEVICE_PROC_ADDR(entrypoint)                                       \
    {                                                                          \
        fp##entrypoint =                                                       \
            (PFN_vk##entrypoint)                                               \
            QVulkanInstance::fpGetDeviceProcAddr(m_device, "vk" #entrypoint);  \
        if (fp##entrypoint == nullptr) {                                       \
            qFatal("vkGetDeviceProcAddr failed to find vk" #entrypoint);       \
        }                                                                      \
    }

QVulkanDevice::QVulkanDevice(
        VkPhysicalDevice& phyDevice,
        uint32_t queue_index,
        const QVector<const char*>& layers,
        const QVector<const char*>& extensions)
{
    VkResult err;
    float queue_priorities[1] = {0.0};

    VkDeviceQueueCreateInfo queue;
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.pNext = NULL;
    queue.queueFamilyIndex = queue_index;
    queue.queueCount = 1;
    queue.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo deviceInfo;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = NULL;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queue;
    deviceInfo.enabledLayerCount = layers.size();
    deviceInfo.ppEnabledLayerNames = layers.data();
    deviceInfo.enabledExtensionCount = extensions.size();
    deviceInfo.ppEnabledExtensionNames = extensions.data();
    deviceInfo.pEnabledFeatures = NULL; // If specific features are required, pass them in here

    err = vkCreateDevice(phyDevice, &deviceInfo, NULL, &m_device);
    Q_ASSERT(!err);
    initFunctionPointers();
}

QVulkanDevice::~QVulkanDevice()
{

}

VkQueue QVulkanDevice::getQueue(uint32_t index)
{
    VkQueue queue;
    vkGetDeviceQueue(m_device, index, 0, &queue);
    return queue;
}

VkCommandPool QVulkanDevice::createCommandPool(int32_t gfxQueueIndex)
{
    VkResult err;
    VkCommandPool pool;
    VkCommandPoolCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = NULL;
    info.queueFamilyIndex = gfxQueueIndex;
    info.flags = 0;
    err = vkCreateCommandPool(m_device, &info, NULL, &pool);
    Q_ASSERT(!err);
    return pool;
}

VkSwapchainKHR QVulkanDevice::createSwapChain()
{

}

QVector<VkSurfaceFormatKHR> QVulkanPhysicalDevice::surfaceFormats(VkSurfaceKHR surface)
{
    VkResult err;
    QVector<VkSurfaceFormatKHR> formats;
    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    err = QVulkanInstance::fpGetPhysicalDeviceSurfaceFormatsKHR(m_device, surface, &formatCount, NULL);
    Q_ASSERT(!err);
    formats.resize(formatCount);
    err = QVulkanInstance::fpGetPhysicalDeviceSurfaceFormatsKHR(m_device, surface, &formatCount, formats.data());
    Q_ASSERT(!err);
    return formats;
}

void QVulkanPhysicalDevice::init(VkSurfaceKHR surface)
{
    uint32_t count = 0;
    VkResult err;
    // Check the surface capabilities and formats
    err = QVulkanInstance::fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_device, surface, &m_surfaceCapabilities);
    Q_ASSERT(!err);

    err = QVulkanInstance::fpGetPhysicalDeviceSurfacePresentModesKHR(m_device, surface, &count, NULL);
    Q_ASSERT(!err);
    m_presentModes.resize(count);
    err = QVulkanInstance::fpGetPhysicalDeviceSurfacePresentModesKHR(m_device, surface, &count, m_presentModes.data());
    Q_ASSERT(!err);
}

void QVulkanDevice::initFunctionPointers()
{
    GET_DEVICE_PROC_ADDR(CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(QueuePresentKHR);
}

PFN_vkGetPhysicalDeviceSurfaceSupportKHR QVulkanInstance::fpGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR QVulkanInstance::fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR QVulkanInstance::fpGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR QVulkanInstance::fpGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR QVulkanInstance::fpCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR QVulkanInstance::fpDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR QVulkanInstance::fpGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR QVulkanInstance::fpAcquireNextImageKHR;
PFN_vkQueuePresentKHR QVulkanInstance::fpQueuePresentKHR;
PFN_vkGetDeviceProcAddr QVulkanInstance::fpGetDeviceProcAddr;

PFN_vkCreateDebugReportCallbackEXT QVulkanInstance::fpCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT QVulkanInstance::fpDestroyDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT QVulkanInstance::fpDebugReportMessageEXT;

QVulkanQueue::QVulkanQueue(VkQueue queue)
    :m_queue(queue)
{

}

QVulkanCommandPool::QVulkanCommandPool(VkCommandPool pool)
    : m_pool(pool)
{

}
