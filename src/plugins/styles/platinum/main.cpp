#include <qplatinumstyle.h>
#include <qstyleplugin.h>

class PlatinumStyle : public QStylePlugin
{
public:
    PlatinumStyle();

    QStringList keys();
    QStyle *create(const QString&);
};

PlatinumStyle::PlatinumStyle()
: QStylePlugin()
{
}

QStringList PlatinumStyle::keys()
{
    QStringList list;
    list << "Platinum";
    return list;
}

QStyle* PlatinumStyle::create(const QString& s)
{
    if (s.toLower() == "platinum")
        return new QPlatinumStyle();

    return 0;
}


Q_EXPORT_PLUGIN(PlatinumStyle)
