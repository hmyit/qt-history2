#include "rangecontrols.h"

#include <qslider.h>
#include <qdial.h>
#include <qspinbox.h>
#include <qscrollbar.h>
#include <qslider.h>
#include <qstyle.h>

QString Q_GUI_EXPORT qacc_stripAmp(const QString &text);

/*!
  \class QAccessibleSpinBox qaccessiblewidget.h
  \brief The QAccessibleText class implements the QAccessibleInterface for spinbox widgets.
*/

/*!
  Constructs a QAccessibleSpinWidget object for \a w.
*/
QAccessibleSpinBox::QAccessibleSpinBox(QWidget *w)
: QAccessibleWidget(w, SpinBox)
{
    Q_ASSERT(spinBox());
    addControllingSignal("valueChanged(int)");
    addControllingSignal("valueChanged(QString)");
}

/*! \reimp */
QSpinBox *QAccessibleSpinBox::spinBox() const
{
    return qt_cast<QSpinBox*>(object());
}

/*! \reimp */
int QAccessibleSpinBox::childCount() const
{
    return ValueDown;
}

/*! \reimp */
QRect QAccessibleSpinBox::rect(int child) const
{
    QRect rect;
    switch(child) {
    case Editor:
        rect = widget()->rect();
        rect.setRight(spinBox()->upRect().left());
        break;
    case ValueUp:
        rect = spinBox()->upRect();
        break;
    case ValueDown:
        rect = spinBox()->downRect();
        break;
    default:
        rect = widget()->rect();
        break;
    }
    QPoint tl = widget()->mapToGlobal(QPoint(0, 0));
    return QRect(tl.x() + rect.x(), tl.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleSpinBox::navigate(Relation rel, int entry, QAccessibleInterface **target) const
{
    *target = 0;

    if (entry) switch (rel) {
    case Child:
        return entry <= childCount() ? entry : -1;
    case QAccessible::Left:
        return (entry == ValueUp || entry == ValueDown) ? Editor : -1;
    case QAccessible::Right:
        return entry == Editor ? ValueUp : -1;
    case QAccessible::Up:
        return entry == ValueDown ? ValueUp : -1;
    case QAccessible::Down:
        return entry == ValueUp ? ValueDown : -1;
    }
    return QAccessibleWidget::navigate(rel, entry, target);
}

/*! \reimp */
QString QAccessibleSpinBox::text(Text t, int child) const
{
    switch (t) {
    case Name:
        switch (child) {
        case ValueUp:
            return QSpinWidget::tr("More");
        case ValueDown:
            return QSpinWidget::tr("Less");
        }
        break;
    case Value:
        if (child == Editor || child == SpinBoxSelf)
            return spinBox()->text();
        break;
    }
    return QAccessibleWidget::text(t, 0);
}

/*! \reimp */
QAccessible::Role QAccessibleSpinBox::role(int child) const
{
    switch(child) {
    case Editor:
        return EditableText;
    case ValueUp:
    case ValueDown:
        return PushButton;
    default:
        break;
    }
    return QAccessibleWidget::role(child);
}

/*! \reimp */
int QAccessibleSpinBox::state(int child) const
{
    int state = QAccessibleWidget::state(child);
    switch(child) {
    case ValueUp:
        if (spinBox()->value() >= spinBox()->maxValue())
            state |= Unavailable;
        return state;
    case ValueDown:
        if (spinBox()->value() <= spinBox()->minValue())
            state |= Unavailable;
        return state;
    default:
        break;
    }
    return state;
}

/*! \reimp */
bool QAccessibleSpinBox::doAction(int action, int child)
{
    if (!widget()->isEnabled())
        return false;

/* // ### vohi - what's that code?
    if (action == Press) switch(child) {
    case ValueUp:
        if (spinBox()->value() >= spinBox()->maxValue())
            return false;
        spinBox()->stepUp();
        return true;
    case ValueDown:
        if (spinBox()->value() <= spinBox()->minValue())
            return false;
        spinBox()->stepDown();
        return true;
    default:
        break;
    }
    */
    return QAccessibleWidget::doAction(action, 0);
}

/*!
  \class QAccessibleScrollBar qaccessiblewidget.h
  \brief The QAccessibleScrollBar class implements the QAccessibleInterface for scroll bars.
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleScrollBar::QAccessibleScrollBar(QWidget *w, const QString &name)
: QAccessibleWidget(w, ScrollBar, name)
{
    Q_ASSERT(scrollBar());
    addControllingSignal("valueChanged(int)");
}

/*! Returns the scroll bar. */
QScrollBar *QAccessibleScrollBar::scrollBar() const
{
    return qt_cast<QScrollBar*>(object());
}

/*! \reimp */
QRect QAccessibleScrollBar::rect(int child) const
{
    QRect rect;
    QRect srect = scrollBar()->style().querySubControlMetrics(QStyle::CC_Slider,
                    scrollBar(), QStyle::SC_SliderHandle);
    int sz = scrollBar()->style().pixelMetric(QStyle::PM_ScrollBarExtent, scrollBar());
    switch (child) {
    case LineUp:
        rect = QRect(0, 0, sz, sz);
        break;
    case PageUp:
        if (scrollBar()->orientation() == Vertical)
            rect = QRect(0, sz, sz, srect.y() - sz);
        else
            rect = QRect(sz, 0, srect.x() - sz, sz);
        break;
    case Position:
        rect = srect;
        break;
    case PageDown:
        if (scrollBar()->orientation() == Vertical)
            rect = QRect(0, srect.bottom(), sz, scrollBar()->rect().height() - srect.bottom() - sz);
        else
            rect = QRect(srect.right(), 0, scrollBar()->rect().width() - srect.right() - sz, sz) ;
        break;
    case LineDown:
        if (scrollBar()->orientation() == Vertical)
            rect = QRect(0, scrollBar()->rect().height() - sz, sz, sz);
        else
            rect = QRect(scrollBar()->rect().width() - sz, 0, sz, sz);
        break;
    default:
        return QAccessibleWidget::rect(child);
    }

    QPoint tp = scrollBar()->mapToGlobal(QPoint(0,0));
    return QRect(tp.x() + rect.x(), tp.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleScrollBar::childCount() const
{
    return LineDown;
}

/*! \reimp */
QString        QAccessibleScrollBar::text(Text t, int child) const
{
    switch (t) {
    case Value:
        if (!child || child == Position)
            return QString::number(scrollBar()->value());
        return QString();
    case Name:
        switch (child) {
        case LineUp:
            return QScrollBar::tr("Line up");
        case PageUp:
            return QScrollBar::tr("Page up");
        case Position:
            return QScrollBar::tr("Position");
        case PageDown:
            return QScrollBar::tr("Page down");
        case LineDown:
            return QScrollBar::tr("Line down");
        }
        break;
    default:
        break;
    }
    return QAccessibleWidget::text(t, child);
}

/*! \reimp */
QAccessible::Role QAccessibleScrollBar::role(int child) const
{
    switch (child) {
    case LineUp:
    case PageUp:
    case PageDown:
    case LineDown:
        return PushButton;
    case Position:
        return Indicator;
    default:
        return ScrollBar;
    }
}

/*! \reimp */
bool QAccessibleScrollBar::doAction(int action, int child)
{
/*
    if (action == Press) switch (child) {
    case LineUp:
        scrollBar()->subtractLine();
        return true;
    case PageUp:
        scrollBar()->subtractPage();
        return true;
    case PageDown:
        scrollBar()->addPage();
        return true;
    case LineDown:
        scrollBar()->addLine();
        return true;
    }
*/
    return false;
}

/*!
  \class QAccessibleSlider qaccessiblewidget.h
  \brief The QAccessibleScrollBar class implements the QAccessibleInterface for sliders.
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleSlider::QAccessibleSlider(QWidget *w, const QString &name)
: QAccessibleWidget(w, Slider, name)
{
    Q_ASSERT(slider());
    addControllingSignal("valueChanged(int)");
}

/*! Returns the slider. */
QSlider *QAccessibleSlider::slider() const
{
    return qt_cast<QSlider*>(object());
}

/*! \reimp */
QRect QAccessibleSlider::rect(int child) const
{
    QRect rect;
    QRect srect = slider()->style().querySubControlMetrics(QStyle::CC_Slider,
                    slider(), QStyle::SC_SliderHandle);
    switch (child) {
    case PageLeft:
        if (slider()->orientation() == Vertical)
            rect = QRect(0, 0, slider()->width(), srect.y());
        else
            rect = QRect(0, 0, srect.x(), slider()->height());
        break;
    case Position:
        rect = srect;
        break;
    case PageRight:
        if (slider()->orientation() == Vertical)
            rect = QRect(0, srect.y() + srect.height(), slider()->width(), slider()->height()- srect.y() - srect.height());
        else
            rect = QRect(srect.x() + srect.width(), 0, slider()->width() - srect.x() - srect.width(), slider()->height());
        break;
    default:
        return QAccessibleWidget::rect(child);
    }

    QPoint tp = slider()->mapToGlobal(QPoint(0,0));
    return QRect(tp.x() + rect.x(), tp.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleSlider::childCount() const
{
    return PageRight;
}

/*! \reimp */
QString        QAccessibleSlider::text(Text t, int child) const
{
    switch (t) {
    case Value:
        if (!child || child == 2)
            return QString::number(slider()->value());
        return QString();
    case Name:
        switch (child) {
        case PageLeft:
            return slider()->orientation() == Horizontal ?
                QSlider::tr("Page left") : QSlider::tr("Page up");
        case Position:
            return QSlider::tr("Position");
        case PageRight:
            return slider()->orientation() == Horizontal ?
                QSlider::tr("Page right") : QSlider::tr("Page down");
        }
        break;
    default:
        break;
    }
    return QAccessibleWidget::text(t, child);
}

/*! \reimp */
QAccessible::Role QAccessibleSlider::role(int child) const
{
    switch (child) {
    case PageLeft:
    case PageRight:
        return PushButton;
    case Position:
        return Indicator;
    default:
        return Slider;
    }
}

/*! \reimp */
int QAccessibleSlider::defaultAction(int child) const
{
/*
    switch (child) {
    case SliderSelf:
        return SetFocus;
    case PageLeft:
        return Press;
    case PageRight:
        return Press;
    }
*/
    return 0;
}

/*! \reimp */
bool QAccessibleSlider::doAction(int action, int child)
{
/*
    switch(child) {
    case SliderSelf:
        if (action == SetFocus) {
            slider()->setFocus();
            return true;
        }
        break;
    case PageLeft:
        if (action == Press) {
            slider()->subtractPage();
            return true;
        }
        break;
    case Position:
        if (action == Increase) {
            slider()->addLine();
            return true;
        } else if (action == Decrease) {
            slider()->subtractLine();
            return true;
        }
        break;
    case PageRight:
        if (action == Press) {
            slider()->addPage();
            return true;
        }
        break;
    }
*/
    return false;
}
