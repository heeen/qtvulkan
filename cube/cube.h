#ifndef CUBE_H
#define CUBE_H

#include "qvulkanview.h"
#include "qvulkanbuffer.h"

struct CubeUniforms {
    CubeUniforms() {
        memset(this, 0, sizeof(*this));
    }

    float mvp[4][4];
    QVector4D position[12 * 3];
    QVector4D attr[12 * 3];
};

class CubeDemo: public QVulkanView {
public:
    CubeDemo();
    ~CubeDemo();
    void init();
    virtual void prepareDescriptorSet() override;
    virtual void buildDrawCommand(VkCommandBuffer cmd_buf) override;
public slots:
    void redraw();

private:
    QVkUniformBuffer<CubeUniforms> m_uniformBuffer;
    MeshData m_cube;

    void updateUniforms();
    float m_spin_angle  {0.1f};
    float m_spin_increment  {0.1f};
    bool m_pause {false};
    bool m_quit { false };
    QElapsedTimer m_fpsTimer {};

    QMatrix4x4 m_projection_matrix  {};
    QMatrix4x4 m_view_matrix        {};
    QMatrix4x4 m_model_matrix       {};


    struct CubeVertexType {
        QVector4D position;
        QVector4D attr;
    };
//    QVkVertexBuffer<CubeVertexType> m_vertexBuffer;

};


#endif // CUBE_H

