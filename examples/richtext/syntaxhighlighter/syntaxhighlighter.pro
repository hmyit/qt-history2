HEADERS         = highlighter.h \
                  mainwindow.h
RESOURCES       = syntaxhighlighter.qrc
SOURCES         = highlighter.cpp \
                  mainwindow.cpp \
                  main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/richtext/syntaxhighlighter
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS syntaxhighlighter.pro examples
sources.path = $$[QT_INSTALL_EXAMPLES]/richtext/syntaxhighlighter
INSTALLS += target sources
