#ifndef QVKPHYSICALDEVICE_H
#define QVKPHYSICALDEVICE_H

#include <QVector>
#include <vulkan/vulkan.h>

class QVkPhysicalDevice {
    friend class QVkInstance;
public:
    ~QVkPhysicalDevice() {

    }
    operator VkPhysicalDevice*() {
        return &m_device;
    }

    operator VkPhysicalDevice() {
        return m_device;
    }

    VkPhysicalDeviceProperties properties();

    VkPhysicalDeviceFeatures features();

    QVector<VkQueueFamilyProperties> queueProperties() const;

    int graphicsQueueIndex();
private:
    explicit QVkPhysicalDevice(VkPhysicalDevice device);
    VkPhysicalDevice m_device;
    QVector<VkQueueFamilyProperties> m_queueProperties           {};
};

#endif /* QVKPHYSICALDEVICE_H */
