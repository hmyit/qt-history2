#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QTcpServer;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);

private slots:
    void sendFortune();

private:
    QLabel *statusLabel;
    QPushButton *quitButton;
    QTcpServer *fortuneServer;
};

#endif
