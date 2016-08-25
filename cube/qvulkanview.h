#ifndef QVULKANVIEW_H
#define QVULKANVIEW_H

#include <QWindow>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <vulkan/vulkan.h>
#include "qvkcmdbuf.h"
#include "qvkinstace.h"

#define DEMO_TEXTURE_COUNT 1

struct SwapchainBuffers {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
};

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

struct MeshData {
    QVector<QVector3D> pos;
    QVector<QVector2D> uv;
};

struct vktexcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    QVector4D position[12 * 3];
    QVector4D attr[12 * 3];
};


class QVulkanView : public QWindow {
public:
    QVulkanView();
    ~QVulkanView();
    void init_vk();
    void init_vk_swapchain();

    void resizeEvent(QResizeEvent *) override; // QWindow::resizeEvent
    void resize_vk();
    void prepare_pipeline();
    void prepare();
    void draw();
    void prepare_texture_image(const char *filename, texture_object *tex_obj, VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props);
    void prepare_textures();
    void prepare_depth();
    void destroy_texture_image(texture_object *tex_objs);
    void prepare_descriptor_layout();
    void prepare_render_pass();
    void flush_init_cmd();
    void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask);
    void prepare_buffers();
    void prepare_descriptor_pool();
    void prepare_framebuffers();

    VkShaderModule createShaderModule(QString filename);

    bool validationError() { return m_validationError; }
    inline QVkDevice& device() { return m_device; }
public slots:
    void redraw();

private:
protected:
/*    bool nativeEvent(const QByteArray &eventType, void *message, long *result)
    {
        qDebug()<<eventType<<message;
        Q_UNUSED(eventType);
        Q_UNUSED(message);
        Q_UNUSED(result);
        return true;
    }*/


    // we don't support copying
    QVulkanView(const QVulkanView&) = delete;
    QVulkanView& operator=(const QVulkanView&) = delete;

    virtual void prepareDescriptorSet() {}
    virtual void buildDrawCommand(VkCommandBuffer cmd_buf) {
        Q_UNUSED(cmd_buf);
    }

    QVector<const char*> m_extensionNames           {};
    QVector<const char*> m_deviceValidationLayers   {};
    VkPhysicalDeviceProperties m_gpu_props                  {};
    QVector<VkQueueFamilyProperties> m_queueProps           {};

    VkSurfaceKHR m_surface      { nullptr };
    bool m_prepared             { false };
    bool m_use_staging_buffer   { false };

    QVkInstance m_inst;
    QVkPhysicalDevice m_gpu;
    uint32_t m_graphics_queue_node_index                    {0};
    QVkDevice m_device;

    QVkQueue m_queue;


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

     // Buffer for initialization commands
    VkCommandBuffer m_cmd               {nullptr};
    VkPipelineLayout m_pipeline_layout  {nullptr};
    VkDescriptorSetLayout m_desc_layout {nullptr};
    VkPipelineCache m_pipelineCache     {nullptr};
    VkRenderPass m_render_pass          {nullptr};
    VkPipeline m_pipeline               {nullptr};
    uint32_t m_current_buffer           {0};

    VkDescriptorPool m_desc_pool  {nullptr};
    VkDescriptorSet m_desc_set  {nullptr};

    int32_t m_curFrame  {0};
    int32_t m_frameCount  {INT32_MAX};
    bool m_use_break { false };
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback  {nullptr};
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback  {nullptr};
    VkDebugReportCallbackEXT msg_callback           {nullptr};
    PFN_vkDebugReportMessageEXT DebugReportMessage  {nullptr};



    bool m_validationError { false };
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
            uint64_t srcObject, size_t location, int32_t msgCode,
            const char *pLayerPrefix, const char *pMsg, void *pUserData);
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



#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif




#endif // QVULKANVIEW_H
