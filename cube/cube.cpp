#include "cube.h"
#include <QTimer>
#include <QApplication>
#include "cubemesh.h"

MeshData makeCube() {
    DEBUG_ENTRY;
    MeshData mesh;
    int numverts = 6*6;
    mesh.pos.reserve(numverts);
    mesh.uv.reserve(numverts);

    for(int i=0; i < numverts; i++) {
        mesh.pos<<QVector3D(
                g_vertex_buffer_data[i*3+0],
                g_vertex_buffer_data[i*3+1],
                g_vertex_buffer_data[i*3+2]);

        mesh.uv<<QVector2D(
                g_uv_buffer_data[i*2+0],
                g_uv_buffer_data[i*2+1]);
    }

    return mesh;
}

CubeDemo::CubeDemo()
    : m_uniformBuffer(device())
//    , m_vertexBuffer(device())
{
    DEBUG_ENTRY;
    QVector3D eye(0.0f, 3.0f, 5.0f);
    QVector3D origin(0, 0, 0);
    QVector3D up(0.0f, 1.0f, 0.0);
    m_cube = makeCube();

    m_projection_matrix.perspective(45.0f, 1.0f, 0.1f, 100.0f);
    m_view_matrix.lookAt(eye, origin, up);
    m_model_matrix = QMatrix();
    m_fpsTimer.start();

    prepareDescriptorSet();
    for (int i = 0; i < m_buffers.count(); i++) {
        qDebug()<<"build draw command for buffer"<<i;
        m_current_buffer = i;
        buildDrawCommand(m_buffers[i].cmd);
    }

    // FIXME: make cube mesh a proper vertex buffer
    // not this stupid uniform hack
    auto* data = m_uniformBuffer.map();
    for (int i = 0; i < m_cube.pos.size(); i++) {
        data->position[i] = QVector4D(m_cube.pos[i], 1.0f);
        data->attr[i] = QVector4D(m_cube.uv[i], 0.0f, 0.0f);
    }
    m_uniformBuffer.unmap();
}

CubeDemo::~CubeDemo()
{
    DEBUG_ENTRY;
}

void CubeDemo::prepareDescriptorSet()
{
    DEBUG_ENTRY;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = m_desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &m_desc_layout;

    VkResult U_ASSERT_ONLY err;
    err = vkAllocateDescriptorSets(m_device, &alloc_info, &m_desc_set);
    Q_ASSERT(!err);

    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT] = {};
    for (uint32_t i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = m_textures[i].sampler;
        tex_descs[i].imageView = m_textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    VkWriteDescriptorSet writes[2]={{},{}};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_desc_set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = m_uniformBuffer.descriptorInfo();

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);
}

void CubeDemo::buildDrawCommand(VkCommandBuffer cmd_buf)
{
    DEBUG_ENTRY;
    QVkCommandBufferRecorder br(cmd_buf);

    static int i=0;
    i+=20;
    QColor clear = QColor(40,40,i);

    br.beginRenderPass(m_render_pass,
                       m_framebuffers[m_current_buffer],
                       QVkRect(0, 0, width(), height()),
                       clear)
        .bindPipeline(m_pipeline)
        .bindDescriptorSet(m_pipeline_layout, &m_desc_set)
        .viewport(QVkViewport((float)width(), (float)height()))
        .scissor(QRect(0, 0, qMax(0, width()), qMax(0, height())))
        .draw(m_cube.pos.size())
        .endRenderPass();

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = nullptr;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    prePresentBarrier.image = m_buffers[m_current_buffer].image;

    br.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                       0, &prePresentBarrier);
}

void CubeDemo::redraw() {
    DEBUG_ENTRY;
    static uint32_t f = 0;
    f++;
    if(f == 100) {
        qDebug()<<(float)f / (float)m_fpsTimer.elapsed() * 1000.0f;
        f=0;
        m_fpsTimer.restart();
    }

    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(device());
    updateUniforms();
    draw();
}

void CubeDemo::updateUniforms() {

    DEBUG_ENTRY;
    QMatrix4x4 MVP, VP;
    int matrixSize = 16 * sizeof(float);

    VP = m_projection_matrix * m_view_matrix;

    // Rotate 22.5 degrees around the Y axis
    m_model_matrix.rotate(0.1f, QVector3D(0.0f, 1.0f, 0.0f));

    MVP = VP * m_model_matrix;

    auto* data = m_uniformBuffer.map();
    memcpy(&data->mvp, (const void *)MVP.data(), matrixSize);
    m_uniformBuffer.unmap();
}

int main(int argc, char **argv) {
    DEBUG_ENTRY;
    setvbuf(stdout, nullptr, _IONBF, 0);

    QGuiApplication app(argc, argv);
    CubeDemo demo;
    demo.resize(500,500);
    demo.show();
    QTimer t;
    t.setInterval(16);
    QObject::connect(&t, &QTimer::timeout, &demo, &CubeDemo::redraw );
    t.start();
    app.exec();
    return demo.validationError();
}
