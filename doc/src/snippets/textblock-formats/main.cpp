#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextEdit *editor = new QTextEdit();

    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start); 

    QTextBlockFormat blockFormat = cursor.blockFormat();
    blockFormat.setTopMargin(4);
    blockFormat.setLeftMargin(4);
    blockFormat.setRightMargin(4);
    blockFormat.setBottomMargin(4);

    cursor.setBlockFormat(blockFormat);
    cursor.insertText(app.tr("This contains plain text inside a "
                             "text block with margins to keep it separate "
                             "from other parts of the document."));

    cursor.insertBlock();

    QTextBlockFormat backgroundFormat = blockFormat;
    backgroundFormat.setBackgroundColor(QColor("#dddddd"));

    cursor.setBlockFormat(backgroundFormat);
    cursor.insertText(app.tr("The background color of a text block can be "
                             "changed to highlight text."));

    cursor.insertBlock();

    QTextBlockFormat rightAlignedFormat = blockFormat;
    rightAlignedFormat.setAlignment(Qt::AlignRight);

    cursor.setBlockFormat(rightAlignedFormat);
    cursor.insertText(app.tr("The alignment of the text within a block is "
                             "controlled by the alignment properties of the "
                             "block itself. This text block is "
                             "right-aligned."));

    cursor.insertBlock();

    QTextBlockFormat paragraphFormat = blockFormat;
    paragraphFormat.setAlignment(Qt::AlignJustify);
    paragraphFormat.setFirstLineMargin(32);

    cursor.setBlockFormat(paragraphFormat);
    cursor.insertText(app.tr("Text can be formatted so that the first line "
                             "in a paragraph has its own margin. This makes "
                             "the text more readable."));

    cursor.insertBlock();

    QTextBlockFormat reverseFormat = blockFormat;
    reverseFormat.setAlignment(Qt::AlignJustify);
    reverseFormat.setFirstLineMargin(32);

    cursor.setBlockFormat(reverseFormat);
    cursor.insertText(app.tr("The direction of the text can be reversed. "
                             "This is useful for right-to-left languages."));

    editor->setWindowTitle(app.tr("Text Block Formats"));
    editor->resize(480, 480);
    editor->show();
    app.setMainWidget(editor);
    
    return app.exec();
}

