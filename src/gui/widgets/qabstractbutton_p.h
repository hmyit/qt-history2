#ifndef QABSTRACBUTTON_P_H
#define QABSTRACBUTTON_P_H

#ifndef QT_H
#include <private/qwidget_p.h>
#include "qbasictimer.h"
#endif

class QAbstractButtonPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QAbstractButton);
public:
    QAbstractButtonPrivate()
        :checkable(false),checked(false),autoRepeat(false),autoExclusive(false),down(false),
         mlbDown(false), blockRefresh(false), group(0)
        {}

    QString text;
    QPixmap pixmap;
    QKeySequence mnemonic;
    uint checkable :1;
    uint checked :1;
    uint autoRepeat :1;
    uint autoExclusive :1;
    uint down :1;
    uint mlbDown :1;
    uint blockRefresh :1;

    Q4ButtonGroup* group;
    QBasicTimer repeatTimer;

    void init();
    void click();
    void refresh();

    QList<QAbstractButton *>queryButtonList() const;
    QAbstractButton *queryCheckedButton() const;
    void notifyChecked();
    void moveFocus(int key);
    void fixFocusPolicy();
};

#endif
