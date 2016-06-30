#include "qvkutil.h"

VkFormat QtFormat2vkFormat(QImage::Format f) {
    switch (f) {
    case QImage::Format_Mono:                   return VK_FORMAT_UNDEFINED; //FIXME
    case QImage::Format_MonoLSB:                return VK_FORMAT_UNDEFINED;
    case QImage::Format_Indexed8:               return VK_FORMAT_UNDEFINED;
    case QImage::Format_RGB32:                  return VK_FORMAT_R8G8B8A8_UNORM;
    case QImage::Format_ARGB32:                 return VK_FORMAT_R8G8B8A8_UNORM;
    case QImage::Format_ARGB32_Premultiplied:   return VK_FORMAT_A8B8G8R8_UINT_PACK32;
    case QImage::Format_RGB16:                  return VK_FORMAT_R5G6B5_UNORM_PACK16;
    case QImage::Format_ARGB8565_Premultiplied: return VK_FORMAT_UNDEFINED;
    case QImage::Format_RGB666:                 return VK_FORMAT_UNDEFINED;
    case QImage::Format_ARGB6666_Premultiplied: return VK_FORMAT_UNDEFINED;
    case QImage::Format_RGB555:                 return VK_FORMAT_R5G5B5A1_UNORM_PACK16; //?
    case QImage::Format_ARGB8555_Premultiplied: return VK_FORMAT_UNDEFINED;
    case QImage::Format_RGB888:                 return VK_FORMAT_R8G8B8_UINT;
    case QImage::Format_RGB444:                 return VK_FORMAT_R4G4B4A4_UNORM_PACK16; // ??
    case QImage::Format_ARGB4444_Premultiplied: return VK_FORMAT_UNDEFINED; //??
    case QImage::Format_RGBX8888:               return VK_FORMAT_R8G8B8A8_UINT;
    case QImage::Format_RGBA8888:               return VK_FORMAT_R8G8B8A8_UINT;
    case QImage::Format_RGBA8888_Premultiplied: return VK_FORMAT_R8G8B8A8_UINT; // ??
    case QImage::Format_BGR30:                  return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case QImage::Format_A2BGR30_Premultiplied:  return VK_FORMAT_A2B10G10R10_UINT_PACK32; //??
    case QImage::Format_RGB30:                  return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case QImage::Format_A2RGB30_Premultiplied:  return VK_FORMAT_A2R10G10B10_UINT_PACK32; //??
    case QImage::Format_Alpha8:                 return VK_FORMAT_R8_UINT;
    case QImage::Format_Grayscale8:             return VK_FORMAT_R8_UINT;
    default:                                    return VK_FORMAT_UNDEFINED;
    }
}

QStringList layerNames(QVector<VkLayerProperties> layers) {
    QStringList names;
    for(const auto& l: layers) names << l.layerName;
    return names;
}
