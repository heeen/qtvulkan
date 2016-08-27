CONFIG += c++11
QT += widgets gui gui-private
# FIXME paths...
LIBS += -lvulkan -lxcb
QMAKE_CXXFLAGS += -g -O -Wall \
    -pedantic-errors -Wextra  -Wcast-align \
    -Wcast-qual  -Wchar-subscripts -Wcomment \
    -Wdisabled-optimization \
    -Wformat  -Wformat=2 \
    -Wformat-nonliteral -Wformat-security  \
    -Wformat-y2k \
    -Wimport  -Winit-self   \
    -Winvalid-pch   \
    -Wmissing-braces \
    -Wmissing-field-initializers -Wmissing-format-attribute   \
    -Wmissing-noreturn \
    -Wpacked -Wparentheses  -Wpointer-arith \
    -Wredundant-decls -Wreturn-type \
    -Wsequence-point  -Wshadow -Wsign-compare  -Wstack-protector \
     -Wswitch  \
    -Wtrigraphs  -Wuninitialized \
    -Wunknown-pragmas  -Wunreachable-code -Wunused \
    -Wunused-function  -Wunused-label  -Wunused-parameter \
    -Wunused-value  -Wunused-variable  -Wvariadic-macros \
    -Wvolatile-register-var  -Wwrite-strings \
    -Wzero-as-null-pointer-constant \
         -Werror

# -Wswitch-enum
# warnings caused by vulkan.h: -Wpadded -Weffc++
# warnings caused by Qt: -Waggregate-return -Wconversion -Wfloat-equal -Wsuggest-override -Wswitch-default -Winline -Wunsafe-loop-optimizations -Wstrict-aliasing -Wstrict-aliasing=2 -Wsuggest-final-types
#-Wsuggest-final-methods

SOURCES += \
    cube.cpp \
    qvulkanview.cpp \
    qvkutil.cpp \
    qvulkanbuffer.cpp \
    qvkcmdbuf.cpp \
    qvkimage.cpp \
    qvkinstance.cpp \
    qvkdevice.cpp \
    qvkphysicaldevice.cpp

HEADERS += \
    cube.h \
    qvkcmdbuf.h \
    qvkutil.h \
    qvulkanview.h \
    qvulkanbuffer.h \
    qvkimage.h \
    qvkinstance.h \
    qvkdevice.h \
    qvkphysicaldevice.h

