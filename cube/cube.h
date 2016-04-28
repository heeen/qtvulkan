#ifndef CUBE_H
#define CUBE_H

#include <QWindow>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QVector>

#include <vulkan/vulkan.h>

#define DEMO_TEXTURE_COUNT 1

typedef struct _SwapchainBuffers {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
} SwapchainBuffers;

/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    uint32_t tex_width, tex_height;
};

typedef QVector<const char*> QVulkanNames;

class Demo : public QWindow {
public:
    Demo();
    ~Demo();
    void init_vk();
    void init_vk_swapchain();
    void create_device();
    bool containsAllLayers(const QVector<VkLayerProperties> haystack, const QVulkanNames needles);

    void resizeEvent(QResizeEvent *) override; // QWindow::resizeEvent
    void resize_vk();
    void prepare_pipeline();
    void prepare();
    void draw();
    void draw_build_cmd(VkCommandBuffer cmd_buf);
    void prepare_texture_image(const char *filename, texture_object *tex_obj, VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props);
    void prepare_textures();
    void prepare_depth();
    void destroy_texture_image(texture_object *tex_objs);
    void prepare_cube_data_buffer();
    void prepare_descriptor_layout();
    void prepare_render_pass();
    void flush_init_cmd();
    void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask);
    void update_data_buffer();
    void prepare_buffers();
    bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
    void prepare_descriptor_pool();
    void prepare_descriptor_set();
    void prepare_framebuffers();

    VkShaderModule createShaderModule(QString filename);
public slots:
    void redraw();

private:
    VkSurfaceKHR m_surface;
    bool m_prepared;
    bool m_use_staging_buffer;

    VkInstance m_inst;
    VkPhysicalDevice m_gpu;
    VkDevice m_device;
    VkQueue m_queue;
    uint32_t m_graphics_queue_node_index;
    VkPhysicalDeviceProperties m_gpu_props;
    QVector<VkQueueFamilyProperties> m_queueProps;
    VkPhysicalDeviceMemoryProperties m_memory_properties;

    QVector<const char*> m_extensionNames;
    QVector<const char*> m_deviceValidationLayers;

    VkFormat m_format;
    VkColorSpaceKHR m_color_space;

    VkSwapchainKHR m_swapchain;
    QVector<SwapchainBuffers> m_buffers;
    QVector<VkFramebuffer> m_framebuffers;  //FIXME these seem to have the same size should they be in the same vector?

    VkCommandPool m_cmd_pool;

    struct {
        VkFormat format;
        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } m_depth;

    struct texture_object m_textures[DEMO_TEXTURE_COUNT];

    struct {
        VkBuffer buf;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } m_uniform_data;

    VkCommandBuffer m_cmd; // Buffer for initialization commands
    VkPipelineLayout m_pipeline_layout;
    VkDescriptorSetLayout m_desc_layout;
    VkPipelineCache m_pipelineCache;
    VkRenderPass m_render_pass;
    VkPipeline m_pipeline;

    QMatrix4x4 m_projection_matrix;
    QMatrix4x4 m_view_matrix;
    QMatrix4x4 m_model_matrix;

    float m_spin_angle;
    float m_spin_increment;
    bool m_pause;

    VkDescriptorPool m_desc_pool;
    VkDescriptorSet m_desc_set;


    bool m_quit;
    int32_t m_curFrame;
    int32_t m_frameCount;
    bool m_validate;
    bool m_use_break;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;

    uint32_t m_current_buffer;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;

    uint32_t frameCount, curFrame;
    QElapsedTimer m_fpsTimer;
};

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint,    \
                     "vkGetInstanceProcAddr Failure");                         \
        }                                                                      \
    }

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


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ERR_EXIT(err_msg, err_class) qFatal(err_msg)

QStringList layerNames(QVector<VkLayerProperties> layers) {
    QStringList names;
    foreach(const auto& l, layers) names << l.layerName;
    return names;
}

template<typename VKTYPE, typename VKFUNC>
QVector<VKTYPE> getVk(VKFUNC getter) {
    VkResult err;
    QVector<VKTYPE> ret;
    uint32_t count;
    err = getter(&count, nullptr);
    Q_ASSERT(!err);
    if (count > 0) {
        ret.resize(count);
        err = getter(&count, ret.data());
        Q_ASSERT(!err);
    }
    return ret;
}

#endif // CUBE_H

