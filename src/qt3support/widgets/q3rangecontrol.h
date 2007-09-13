/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef Q3RANGECONTROL_H
#define Q3RANGECONTROL_H

#include <QtCore/qglobal.h>
#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Qt3SupportLight)

#ifndef QT_NO_RANGECONTROL

class Q3RangeControlPrivate;

class Q_COMPAT_EXPORT Q3RangeControl
{
public:
    Q3RangeControl();
    Q3RangeControl(int minValue, int maxValue,
                   int lineStep, int pageStep, int value);
    virtual ~Q3RangeControl();

    int                value()                const;
    void        setValue(int);
    void        addPage();
    void        subtractPage();
    void        addLine();
    void        subtractLine();

    int                minValue()        const;
    int                maxValue()        const;
    void        setRange(int minValue, int maxValue);
    void        setMinValue(int minVal);
    void        setMaxValue(int minVal);

    int                lineStep()        const;
    int                pageStep()        const;
    void        setSteps(int line, int page);

    int                bound(int) const;

protected:
    int                positionFromValue(int val, int space) const;
    int                valueFromPosition(int pos, int space) const;
    void        directSetValue(int val);
    int                prevValue()        const;

    virtual void valueChange();
    virtual void rangeChange();
    virtual void stepChange();

private:
    int                minVal, maxVal;
    int                line, page;
    int                val, prevVal;

    Q3RangeControlPrivate * d;

private:
    Q_DISABLE_COPY(Q3RangeControl)
};


inline int Q3RangeControl::value() const
{ return val; }

inline int Q3RangeControl::prevValue() const
{ return prevVal; }

inline int Q3RangeControl::minValue() const
{ return minVal; }

inline int Q3RangeControl::maxValue() const
{ return maxVal; }

inline int Q3RangeControl::lineStep() const
{ return line; }

inline int Q3RangeControl::pageStep() const
{ return page; }


#endif // QT_NO_RANGECONTROL

#ifndef QT_NO_SPINWIDGET

class Q3SpinWidgetPrivate;
class Q_COMPAT_EXPORT Q3SpinWidget : public QWidget
{
    Q_OBJECT
public:
    Q3SpinWidget(QWidget* parent=0, const char* name=0);
    ~Q3SpinWidget();

    void         setEditWidget(QWidget * widget);
    QWidget *         editWidget();

    QRect upRect() const;
    QRect downRect() const;

    void setUpEnabled(bool on);
    void setDownEnabled(bool on);

    bool isUpEnabled() const;
    bool isDownEnabled() const;

    enum ButtonSymbols { UpDownArrows, PlusMinus };
    virtual void        setButtonSymbols(ButtonSymbols bs);
    ButtonSymbols        buttonSymbols() const;

    void arrange();

Q_SIGNALS:
    void stepUpPressed();
    void stepDownPressed();

public Q_SLOTS:
    void stepUp();
    void stepDown();

protected:
    void mousePressEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent* ev);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *);
#endif
    void changeEvent(QEvent *);
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void timerDone();
    void timerDoneEx();

private:
    Q3SpinWidgetPrivate * d;

    void updateDisplay();

private:
    Q_DISABLE_COPY(Q3SpinWidget)
};

#endif // QT_NO_RANGECONTROL

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q3RANGECONTROL_H
