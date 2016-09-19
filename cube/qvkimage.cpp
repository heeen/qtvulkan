#include "qvkimage.h"



QVkImage::~QVkImage() {
    DEBUG_ENTRY;
    vkDestroyImage(vkDevice(), m_image, nullptr);
    m_image = nullptr;
}


