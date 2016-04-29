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

struct vktexcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
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
    // we don't support copying
    Demo(const Demo&) = delete;
    Demo operator=(const Demo&) = delete;

    VkSurfaceKHR m_surface      { nullptr };
    bool m_prepared             { false };
    bool m_use_staging_buffer   { false };

    VkInstance m_inst       {nullptr};
    VkPhysicalDevice m_gpu  {nullptr};
    VkDevice m_device       {nullptr};
    VkQueue m_queue         {nullptr};
    uint32_t m_graphics_queue_node_index                    {0};
    VkPhysicalDeviceProperties m_gpu_props                  {};
    QVector<VkQueueFamilyProperties> m_queueProps           {};
    VkPhysicalDeviceMemoryProperties m_memory_properties    {};

    QVector<const char*> m_extensionNames           {};
    QVector<const char*> m_deviceValidationLayers   {};

    VkFormat m_format               {};
    VkColorSpaceKHR m_color_space   {};

    VkSwapchainKHR m_swapchain {nullptr};
    QVector<SwapchainBuffers> m_buffers     {};
    //FIXME these seem to have the same size should they be in the same vector?
    QVector<VkFramebuffer> m_framebuffers   {};

    VkCommandPool m_cmd_pool  {nullptr};

    struct {
        VkFormat format;
        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } m_depth {};

    struct texture_object m_textures[DEMO_TEXTURE_COUNT] {};

    struct {
        VkBuffer buf;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } m_uniform_data  {};

     // Buffer for initialization commands
    VkCommandBuffer m_cmd               {nullptr};
    VkPipelineLayout m_pipeline_layout  {nullptr};
    VkDescriptorSetLayout m_desc_layout {nullptr};
    VkPipelineCache m_pipelineCache     {nullptr};
    VkRenderPass m_render_pass          {nullptr};
    VkPipeline m_pipeline               {nullptr};
    uint32_t m_current_buffer           {0};

    QMatrix4x4 m_projection_matrix  {};
    QMatrix4x4 m_view_matrix        {};
    QMatrix4x4 m_model_matrix       {};

    float m_spin_angle  {0.1f};
    float m_spin_increment  {0.1f};
    bool m_pause {false};

    VkDescriptorPool m_desc_pool  {nullptr};
    VkDescriptorSet m_desc_set  {nullptr};


    bool m_quit { false };
    int32_t m_curFrame  {0};
    int32_t m_frameCount  {INT32_MAX};
    bool m_validate { true };
    bool m_use_break { false };
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback  {nullptr};
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback  {nullptr};
    VkDebugReportCallbackEXT msg_callback           {nullptr};
    PFN_vkDebugReportMessageEXT DebugReportMessage  {nullptr};


    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR           {nullptr};
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR {nullptr};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR           {nullptr};
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR {nullptr};
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR       {nullptr};
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR     {nullptr};
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR {nullptr};
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR     {nullptr};
    PFN_vkQueuePresentKHR fpQueuePresentKHR             {nullptr};

    QElapsedTimer m_fpsTimer {};
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

