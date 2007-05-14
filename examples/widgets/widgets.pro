TEMPLATE      = subdirs
SUBDIRS       = analogclock \
                calculator \
                calendarwidget \
                charactermap \
                digitalclock \
                groupbox \
                icons \
                imageviewer \
                lineedits \
                movie \
                scribble \
                shapedclock \
                sliders \
                spinboxes \
                styles \
                stylesheet \
                tablet \
                tetrix \
                tooltips \
                wiggly \
                windowflags

# uic hangs in this example for some crosscompilers, task 158836
!cross_compile: SUBDIRS += validators

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS widgets.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets
INSTALLS += target sources
