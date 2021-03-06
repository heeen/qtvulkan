#include <QDebug>
#include "qvkinstance.h"

QVkInstance::QVkInstance() {

    DEBUG_ENTRY;
    QVulkanNames validationLayers;
    const QVulkanNames standardValidation = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    const QVulkanNames defaultValidationLayers = {
        "VK_LAYER_GOOGLE_threading",     "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_device_limits", "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_image",         "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_swapchain",     "VK_LAYER_GOOGLE_unique_objects"
    };

    // Look for validation layers
    if (m_validate) {
        qDebug()<<"enabling validation";
        auto layers = getVk<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

        qDebug()<<"found instance layers:" << getLayerNames(layers);

        if (layers.isEmpty()) {
            qFatal("could not find any layers. check VK_LAYER_PATH (%s)", getenv("VK_LAYER_PATH"));
        }

        if(containsAllLayers(layers, standardValidation)) {
            qDebug()<<"using standard validation layer";
            validationLayers << standardValidation; // FIXME do we need different layers for instance and device?
            //m_deviceValidationLayers << standardValidation;
        } else if (containsAllLayers(layers, defaultValidationLayers)) {
            qDebug()<<"using default validation layers";
            validationLayers << defaultValidationLayers;
            //m_deviceValidationLayers << defaultValidationLayers;
        } else {
            qFatal("validation layers requested, but could not find validation layers");
        }
    }

    /* Look for instance extensions */
    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;

    auto getExt = [](uint32_t* c, VkExtensionProperties* d) { return vkEnumerateInstanceExtensionProperties(nullptr, c, d); };
    auto extensions = getVk<VkExtensionProperties>(getExt);

    if (extensions.isEmpty()) {
        qFatal("could not find any instance extensions");
    }

    for (const auto& ext: extensions) {
        qDebug()<< "instance extension:" << ext.extensionName;
        if (!strcmp(ext.extensionName, VK_KHR_SURFACE_EXTENSION_NAME)) {
            surfaceExtFound = 1;
            m_extensionNames << VK_KHR_SURFACE_EXTENSION_NAME;
        }
        if (!strcmp(ext.extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME)) {
            platformSurfaceExtFound = 1;
            m_extensionNames << VK_KHR_XCB_SURFACE_EXTENSION_NAME;
        }
        if (!strcmp(ext.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
            if (m_validate) {
                m_extensionNames << VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            }
        }
    }

    if (!surfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    if (!platformSurfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr;
    app.pApplicationName = "cube";
    app.applicationVersion = 0;
    app.pEngineName = "cubenegine";
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = validationLayers.count();
    inst_info.ppEnabledLayerNames = validationLayers.data();
    inst_info.enabledExtensionCount = m_extensionNames.count();
    inst_info.ppEnabledExtensionNames = m_extensionNames.data();

    /*
     * This is info for a temp callback to use during CreateInstance.
     * After the instance is created, we use the instance-based
     * function to register the final callback.
     */
    VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
    if (m_validate) {
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = nullptr;
        dbgCreateInfo.pfnCallback = s_dbgFunc;
        dbgCreateInfo.pUserData = this;
        dbgCreateInfo.flags =
                VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        // | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        inst_info.pNext = &dbgCreateInfo;
    }

    VkResult err = vkCreateInstance(&inst_info, nullptr, &m_instance);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT("Cannot find a specified extension library"
                 ".\nMake sure your layers path is set appropriately.\n",
                 "vkCreateInstance Failure");
    } else if (err) {
        ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                 "installable client driver (ICD) installed?\nPlease look at "
                 "the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    if (m_validate) {
        CreateDebugReportCallback =
                (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                    m_instance, "vkCreateDebugReportCallbackEXT");
        DestroyDebugReportCallback =
                (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                    m_instance, "vkDestroyDebugReportCallbackEXT");
        if (!CreateDebugReportCallback) {
            ERR_EXIT(
                        "GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
                        "vkGetProcAddr Failure");
        }
        if (!DestroyDebugReportCallback) {
            ERR_EXIT(
                        "GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
                        "vkGetProcAddr Failure");
        }
        DebugReportMessage =
                (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                    m_instance, "vkDebugReportMessageEXT");
        if (!DebugReportMessage) {
            ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
                     "vkGetProcAddr Failure");
        }

        dbgCreateInfo = {};
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = nullptr;
        dbgCreateInfo.pfnCallback = s_dbgFunc;
        dbgCreateInfo.pUserData = this;
        dbgCreateInfo.flags =
                VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        //| VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        err = CreateDebugReportCallback(m_instance, &dbgCreateInfo, nullptr, &msg_callback);
        switch (err) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
                     "CreateDebugReportCallback Failure");
            break;
        default:
            ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
                     "CreateDebugReportCallback Failure");
            break;
        }
    }
    initFunctions();
}


QVkInstance::~QVkInstance() {
    DEBUG_ENTRY;
    if(!m_instance) return;
    if (/*m_validate*/ true) {
        DestroyDebugReportCallback(m_instance, msg_callback, nullptr);
    }
    vkDestroyInstance(m_instance, nullptr);
}


VkBool32 QVkInstance::debugMessage(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg)
{
    Q_UNUSED(objType)
    Q_UNUSED(srcObject)
    Q_UNUSED(location)
    {
        QDebug q(qDebug());

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        q<<"ERROR";
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        q<<"WARNING";
    } else {
        q<<"INFO: ["<<pLayerPrefix<<"] Code"<<msgCode<<pMsg;
        return false;
    }

    q<<": ["<<pLayerPrefix<<"] Code"<<msgCode<<pMsg;
    }
    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    abort();
    return false;
}

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint,    \
                     "vkGetInstanceProcAddr Failure");                         \
        }                                                                      \
    }


void QVkInstance::initFunctions()
{
    GET_INSTANCE_PROC_ADDR(m_instance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(m_instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(m_instance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(m_instance, GetPhysicalDeviceSurfacePresentModesKHR);
// GET_INSTANCE_PROC_ADDR(m_instance, GetSwapchainImagesKHR);
}

QVkPhysicalDevice QVkInstance::device(uint32_t index) {
    DEBUG_ENTRY;
    qDebug()<<"get device"<<index<<m_instance;
    auto getDev = [this](uint32_t* c, VkPhysicalDevice* d) { return vkEnumeratePhysicalDevices(m_instance, c, d); };
    auto physicalDevices = getVk<VkPhysicalDevice>(getDev);

    if (physicalDevices.isEmpty()) {
        ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) "
                 "installed?\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkEnumeratePhysicalDevices Failure");
    }
    qDebug()<<physicalDevices;
    QVkPhysicalDevice dev(physicalDevices[index]);
    return dev;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
QVkInstance::s_dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char *pLayerPrefix, const char *pMsg, void *pUserData) {

    QVkInstance* inst = (QVkInstance*) pUserData;
    return inst->debugMessage(msgFlags, objType, srcObject, location, msgCode, pLayerPrefix, pMsg);
}
