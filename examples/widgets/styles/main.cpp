#include <QApplication>

#include "widgetgallery.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    WidgetGallery gallery;
    app.setMainWidget(&gallery);
    gallery.show();
    return app.exec();
}
