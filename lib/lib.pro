CONFIG += c++11
QT += widgets gui gui-private
# FIXME paths...
INCLUDEPATH += $$PWD/../../Vulkan-LoaderAndValidationLayers/include
LIBS += -lvulkan -L$$PWD/../../Vulkan-LoaderAndValidationLayers/dbuild/loader

HEADERS       =  qvulkanview.h \
    qvulkaninstance.h
SOURCES       = main.cpp \
    qvulkanview.cpp \
    qvulkaninstance.cpp

