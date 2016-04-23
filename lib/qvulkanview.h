#ifndef QVULKANVIEW_H
#define QVULKANVIEW_H
#include "vulkan/vulkan.h"
#include <QObject>
#include <QWindow>

class QVulkanInstance;

class QVulkanView : public QWindow
{
    Q_OBJECT
public:
    explicit QVulkanView(QWindow *parent = 0);
    ~QVulkanView();
    void resizeEvent(QResizeEvent *) override;
signals:

public slots:
private:
    static QVulkanInstance *s_vulkanInstance;
    VkSurfaceKHR m_vkSurface;
};

#endif // QVULKANVIEW_H
