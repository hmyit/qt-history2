#ifndef QPROPERTYEDITOR_DELEGATE_P_H
#define QPROPERTYEDITOR_DELEGATE_P_H

#include <qitemdelegate.h>

namespace QPropertyEditor
{

class Delegate : public QItemDelegate
{
    Q_OBJECT
public:
    Delegate(QObject *parent = 0);
    virtual ~Delegate();

    virtual bool eventFilter(QObject *object, QEvent *event);
    
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

//
// QItemDelegate Interface
//
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    virtual QWidget *editor(QWidget *parent,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index);

    virtual void setEditorData(QWidget *editor,
                               const QModelIndex &index) const;

    virtual void setModelData(QWidget *editor,
                              QAbstractItemModel *model,
                              const QModelIndex &index) const;

public slots:
    void sync();

protected:
    virtual void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option,
                                const QRect &rect, const QPixmap &pixmap) const;
                                
private:
    bool m_readOnly;
};

} // namespace QPropertyEditor

#endif // QPROPERTYEDITOR_DELEGATE_P_H

