#include <QApplication>
#include "qvulkanview.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("heeen");
    app.setApplicationName("Vulkan Test");
    QVulkanView view;
    view.resize(400, 400);
    view.show();
    return app.exec();
}
