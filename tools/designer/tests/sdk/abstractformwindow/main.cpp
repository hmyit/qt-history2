
#include "tst_abstractformwindow.h"

#include <ideapplication.h>

int main(int argc, char *argv[])
{
    tst_AbstractFormWindow tc(argc, argv);

    IDEApplication app(argc, argv);
    Q_UNUSED(app);

    return tc.exec();
}
