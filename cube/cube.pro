CONFIG += c++11
QT += widgets gui gui-private
# FIXME paths...
INCLUDEPATH += $$PWD/../../Vulkan-LoaderAndValidationLayers/include
LIBS += -lvulkan -L$$PWD/../../Vulkan-LoaderAndValidationLayers/dbuild/loader -lxcb

SOURCES += \
    cube.cpp

HEADERS += \
    cube.h

