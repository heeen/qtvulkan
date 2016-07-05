#ifndef QVKUTIL_H
#define QVKUTIL_H

#include <QRectF>
#include <vulkan/vulkan.h>
#include <QImage>

struct QVkRect : public VkRect2D{
    QVkRect() {}
    QVkRect(int x, int y, int w, int h) {
        extent.width = w;
        extent.height = h;
        offset.x = x;
        offset.y = y;
    };

    QVkRect(QRect qr) {
        extent.width = qr.width();
        extent.height = qr.height();
        offset.x = qr.left();
        offset.y = qr.top();
    }

    QVkRect& operator=(QRect qr) {
        extent.width = qr.width();
        extent.height = qr.height();
        offset.x = qr.left();
        offset.y = qr.top();
        return *this;
    }
};

struct QVkViewport: public VkViewport {
    QVkViewport() { width = 300.0f; height = 300.0f; }
    QVkViewport(float aWidth, float aHeight)
        : QVkViewport(0.0f, 0.0f, aWidth, aHeight, 0.0f, 1.0f)
    { }
    QVkViewport(float aX, float aY, float aWidth, float aHeight, float aMinDepth=0.0f, float aMaxDepth=1.0f) {
        x = aX;
        y = aY;
        width = aWidth;
        height = aHeight;
        minDepth = aMinDepth;
        maxDepth = aMaxDepth;
    }
};

class QVkDeviceResource {
public:
    QVkDeviceResource(VkDevice dev): m_device(dev) {}
    VkDevice device() { return m_device; }
protected:
    VkDevice m_device;
};

VkFormat QtFormat2vkFormat(QImage::Format f);
QStringList layerNames(QVector<VkLayerProperties> layers);

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

class ScopeDebug {
public:
    ScopeDebug(const char* func) : f(func){
        operator ()(">>> %s", f);
        stack++;
    }
    ~ScopeDebug() {
        stack--;
        operator ()("<<< %s", f);
    }
    // 2 format is actually arg no. 2 because of this ptr?
    void operator() (const char *__format, ...) __attribute__ ((format (printf, 2, 3))) {
        va_list arglist;
        va_start(arglist,__format);
        for(uint32_t i=0; i< stack;i++) fputs("  ", stdout);
        vprintf(__format, arglist);
        va_end(arglist);
        fputs("\n",stdout);
    }
    static uint32_t stack;
    const char* f;
    ScopeDebug(const ScopeDebug&) = delete;
    ScopeDebug operator=(const ScopeDebug&) = delete;

};


#if 1
#define DEBUG_ENTRY ScopeDebug DBG(__PRETTY_FUNCTION__);
#else
#define DEBUG_ENTRY {}
#define DBG(...)
#endif


#endif // QVKUTIL_H

