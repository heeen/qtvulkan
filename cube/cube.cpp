#include "cube.h"
#include <QTimer>
#include <QApplication>

CubeDemo::CubeDemo()
{

}

CubeDemo::~CubeDemo()
{

}


int main(int argc, char **argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);

    QGuiApplication app(argc, argv);
    QVulkanView demo;
    demo.show();

    demo.resize(500,500);
    demo.init_vk();
    demo.init_vk_swapchain();
    demo.prepare();
    QTimer t;
    t.setInterval(16);
    QObject::connect(&t, &QTimer::timeout, &demo, &QVulkanView::redraw );
    t.start();
    app.exec();
    return demo.validationError();
}
