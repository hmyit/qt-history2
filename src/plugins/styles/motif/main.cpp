#include <qstyleplugin.h>
#include <qmotifstyle.h>

class MotifStyle : public QStylePlugin
{
public:
    MotifStyle();

    QStringList keys();
    QStyle *create(const QString&);
};

MotifStyle::MotifStyle()
: QStylePlugin()
{
}

QStringList MotifStyle::keys()
{
    QStringList list;
    list << "Motif";
    return list;
}

QStyle* MotifStyle::create(const QString& s)
{
    if (s.toLower() == "motif")
        return new QMotifStyle();

    return 0;
}

Q_EXPORT_PLUGIN(MotifStyle)

