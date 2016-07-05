CONFIG += c++11
QT += widgets gui gui-private
# FIXME paths...
LIBS += -lvulkan -lxcb
QMAKE_CXXFLAGS += -g -O -Wall  -pedantic  \
    -pedantic-errors -Wextra  -Wcast-align \
    -Wcast-qual  -Wchar-subscripts -Wcomment -Wconversion \
    -Wdisabled-optimization \
    -Wfloat-equal  -Wformat  -Wformat=2 \
    -Wformat-nonliteral -Wformat-security  \
    -Wformat-y2k \
    -Wimport  -Winit-self  -Winline \
    -Winvalid-pch   \
    -Wunsafe-loop-optimizations  -Wlong-long -Wmissing-braces \
    -Wmissing-field-initializers -Wmissing-format-attribute   \
    -Wmissing-noreturn \
    -Wpacked -Wparentheses  -Wpointer-arith \
    -Wredundant-decls -Wreturn-type \
    -Wsequence-point  -Wshadow -Wsign-compare  -Wstack-protector \
    -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch  -Wswitch-default \
    -Wtrigraphs  -Wuninitialized \
    -Wunknown-pragmas  -Wunreachable-code -Wunused \
    -Wunused-function  -Wunused-label  -Wunused-parameter \
    -Wunused-value  -Wunused-variable  -Wvariadic-macros \
    -Wvolatile-register-var  -Wwrite-strings \
    -Wzero-as-null-pointer-constant \
    -Wsuggest-final-types  -Wsuggest-final-methods -Wsuggest-override \
    -Werror

# -Wswitch-enum
# warnings caused by vulkan.h: -Wpadded -Weffc++
# warnings caused by Qt: -Waggregate-return
SOURCES += \
    cube.cpp \
    qvulkanview.cpp \
    qvkutil.cpp \
    qvulkanbuffer.cpp

HEADERS += \
    cube.h \
    qvkcmdbuf.h \
    qvkutil.h \
    qvulkanview.h \
    qvulkanbuffer.h

