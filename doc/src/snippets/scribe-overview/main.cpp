#include <QtGui>

void mergeFormat(QTextEdit *edit)
{
    QTextDocument *document = edit->document(); 
    QTextCursor cursor(document);

    cursor.movePosition(QTextCursor::Start); 
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);

    cursor.mergeCharFormat(format);
}

int main(int argc, char *argv[])
{
    QWidget *parent = 0;
    QString aStringContainingHTMLtext("<h1>Scribe Overview</h1>");

    QApplication app(argc, argv);

    QTextEdit *editor = new QTextEdit(parent);
    editor->setHtml(aStringContainingHTMLtext);
    editor->show();

    return app.exec();
}
