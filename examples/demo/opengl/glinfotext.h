#include <qlayout.h>
#include <qtextview.h>
#include <qfont.h>
#include "glinfo.h"

class GLInfoText : public QWidget
{
public:
    GLInfoText(QWidget *parent) 
	: QWidget(parent)
    {
	view = new QTextView(this);
	view->setFont(QFont("courier"));
	QHBoxLayout *l = new QHBoxLayout(this);
	l->addWidget(view);
	GLInfo info;
	view->setText(info.extensions());
	view->append(info.configs());
    }
    
private:
    QTextView *view;
};
