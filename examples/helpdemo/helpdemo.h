#ifndef HELPDEMO_H
#define HELPDEMO_H

#include <qptrdict.h>

#include "helpdemobase.h"

class QAssistantClient;
class QPopupMenu;

class HelpDemo : public HelpDemoBase
{
    Q_OBJECT

public:
    HelpDemo( QWidget *parent = 0, const char *name = 0 );
    ~HelpDemo();

protected:
    void keyPressEvent( QKeyEvent *e );
    void contextMenuEvent( QContextMenuEvent *e );

private slots:
    void setAssistantArguments();
    void openAssistant();
    void closeAssistant();
    void displayPage();
    void showAssistantErrors( const QString &err );
    void installExampleDocs();
    void assistantOpened();
    void assistantClosed();

private:
    QWidget* lookForWidget();
    void showHelp( QWidget *w );

    QPtrDict<char> widgets;
    QAssistantClient *assistant;
    QPopupMenu *menu;

};

#endif
