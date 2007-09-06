# This qmake file is included by all Patternist projects and contains common Qt defines,
# compiler warnings, and include paths.

DEFINES += QT_CLEAN_NAMESPACE       \
           QT_NO_CAST_FROM_ASCII    \
           QT_NO_CAST_TO_ASCII      \
           QT_NO_COMPAT

INCLUDEPATH += $$PWD/acceltree      \
               $$PWD/data           \
               $$PWD/api            \
               $$PWD/environment    \
               $$PWD/expr           \
               $$PWD/functions      \
               $$PWD/iterators      \
               $$PWD/janitors       \
               $$PWD/parser         \
               $$PWD/qobjectmodel   \
               $$PWD/type           \
               $$PWD/utils

DEPENDPATH += $$INCLUDEPATH

release {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

linux-g++ {
QMAKE_CXXFLAGS += -ansi                 \
                  -Wcast-align          \
                  -Wchar-subscripts     \
                  -Wconversion          \
                  -Wfloat-equal         \
                  -Wold-style-cast      \
                  -Woverloaded-virtual  \
                  -Wpointer-arith       \
                  -Wredundant-decls     \
                  -Wsign-compare        \
                  -Wundef               \
                  -Wunused              \
                  -Wunused-macros       \
                  -pedantic             \
                  -Wno-long-long
# -Wshadow              \ Qt sources can't handle this.
# FIXME           -Wlong-long           \
}