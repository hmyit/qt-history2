/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of Qt Linguist.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*  TRANSLATOR PhraseBookBox

  Go to Phrase > Edit Phrase Book...  The dialog that pops up is a
  PhraseBookBox.
*/

#include "phrasebookbox.h"
#include "phraselv.h"

#include <qapplication.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

PhraseBookBox::PhraseBookBox( const QString& filename,
                              const PhraseBook& phraseBook, QWidget *parent,
                              const char *name, bool modal )
    : QDialog( parent, name, modal ), fn( filename ), pb( phraseBook )
{
    QGridLayout *gl = new QGridLayout( this, 4, 3, 11, 11,
                                       "phrase book outer layout" );
    QVBoxLayout *bl = new QVBoxLayout( 6, "phrase book button layout" );

    sourceLed = new QLineEdit( this, "source line edit" );
    QLabel *source = new QLabel( sourceLed, tr("S&ource phrase:"), this,
                                 "source label" );
    targetLed = new QLineEdit( this, "target line edit" );
    QLabel *target = new QLabel( targetLed, tr("&Translation:"), this,
                                 "target label" );
    definitionLed = new QLineEdit( this, "definition line edit" );
    QLabel *definition = new QLabel( definitionLed, tr("&Definition:"), this,
                                     "target label" );
    lv = new PhraseLV( this, "phrase book list view" );

    newBut = new QPushButton( tr("&New Phrase"), this );
    newBut->setDefault( TRUE );

    removeBut = new QPushButton( tr("&Remove Phrase"), this );
    removeBut->setEnabled( FALSE );
    QPushButton *saveBut = new QPushButton( tr("&Save"), this );
    QPushButton *closeBut = new QPushButton( tr("Close"), this );

    gl->addWidget( source, 0, 0 );
    gl->addWidget( sourceLed, 0, 1 );
    gl->addWidget( target, 1, 0 );
    gl->addWidget( targetLed, 1, 1 );
    gl->addWidget( definition, 2, 0 );
    gl->addWidget( definitionLed, 2, 1 );
    gl->addMultiCellWidget( lv, 3, 3, 0, 1 );
    gl->addMultiCell( bl, 0, 3, 2, 2 );

    bl->addWidget( newBut );
    bl->addWidget( removeBut );
    bl->addWidget( saveBut );
    bl->addWidget( closeBut );
    bl->addStretch( 1 );

    connect( sourceLed, SIGNAL(textChanged(const QString&)),
             this, SLOT(sourceChanged(const QString&)) );
    connect( targetLed, SIGNAL(textChanged(const QString&)),
             this, SLOT(targetChanged(const QString&)) );
    connect( definitionLed, SIGNAL(textChanged(const QString&)),
             this, SLOT(definitionChanged(const QString&)) );
    connect( lv, SIGNAL(selectionChanged(QListViewItem *)),
             this, SLOT(selectionChanged(QListViewItem *)) );
    connect( newBut, SIGNAL(clicked()), this, SLOT(newPhrase()) );
    connect( removeBut, SIGNAL(clicked()), this, SLOT(removePhrase()) );
    connect( saveBut, SIGNAL(clicked()), this, SLOT(save()) );
    connect( closeBut, SIGNAL(clicked()), this, SLOT(accept()) );

    PhraseBook::ConstIterator it;
    for ( it = phraseBook.begin(); it != phraseBook.end(); ++it )
        (void) new PhraseLVI( lv, (*it) );
    enableDisable();

    this->setWhatsThis(tr("This window allows you to add, modify, or delete"
                              " phrases in a phrase book.") );
    sourceLed->setWhatsThis(tr("This is the phrase in the source"
                                   " language.") );
    targetLed->setWhatsThis(tr("This is the phrase in the target language"
                                   " corresponding to the source phrase.") );
    definitionLed->setWhatsThis(tr("This is a definition for the source"
                                       " phrase.") );
    newBut->setWhatsThis(tr("Click here to add the phrase to the phrase"
                                " book.") );
    removeBut->setWhatsThis(tr("Click here to remove the phrase from the"
                                   " phrase book.") );
    saveBut->setWhatsThis(tr("Click here to save the changes made.") );
    closeBut->setWhatsThis(tr("Click here to close this window.") );
}

void PhraseBookBox::keyPressEvent( QKeyEvent *ev )
{
    if ( ev->key() == Key_Down || ev->key() == Key_Up ||
         ev->key() == Key_Next || ev->key() == Key_Prior )
        QApplication::sendEvent( lv,
                new QKeyEvent(ev->type(), ev->key(), ev->state(), ev->text(), ev->isAutoRepeat(), ev->count()) );
    else
        QDialog::keyPressEvent( ev );
}

void PhraseBookBox::newPhrase()
{
    Phrase ph;
    ph.setSource( NewPhrase );
    QListViewItem *item = new PhraseLVI( lv, ph );
    selectItem( item );
}

void PhraseBookBox::removePhrase()
{
    QListViewItem *item = lv->currentItem();
    QListViewItem *next = item->itemBelow() != 0 ? item->itemBelow()
                          : item->itemAbove();
    delete item;
    if ( next != 0 )
        selectItem( next );
    enableDisable();
}

void PhraseBookBox::save()
{
    pb.clear();
    QListViewItem *item = lv->firstChild();
    while ( item != 0 ) {
        if ( !item->text(PhraseLVI::SourceTextShown).isEmpty() &&
             item->text(PhraseLVI::SourceTextShown) != NewPhrase )
            pb.append( Phrase(((PhraseLVI *) item)->phrase()) );
        item = item->nextSibling();
    }
    if ( !pb.save( fn ) )
        QMessageBox::warning( this, tr("Qt Linguist"),
                              tr("Cannot save phrase book '%1'.").arg(fn) );
}

void PhraseBookBox::sourceChanged( const QString& source )
{
    if ( lv->currentItem() != 0 ) {
        lv->currentItem()->setText( PhraseLVI::SourceTextShown,
                                    source.trimmed() );
        lv->currentItem()->setText( PhraseLVI::SourceTextOriginal, source );
        lv->sort();
        lv->ensureItemVisible( lv->currentItem() );
    }
}

void PhraseBookBox::targetChanged( const QString& target )
{
    if ( lv->currentItem() != 0 ) {
        lv->currentItem()->setText( PhraseLVI::TargetTextShown,
                                    target.trimmed() );
        lv->currentItem()->setText( PhraseLVI::TargetTextOriginal, target );
        lv->sort();
        lv->ensureItemVisible( lv->currentItem() );
    }
}

void PhraseBookBox::definitionChanged( const QString& definition )
{
    if ( lv->currentItem() != 0 ) {
        lv->currentItem()->setText( PhraseLVI::DefinitionText, definition );
        lv->sort();
        lv->ensureItemVisible( lv->currentItem() );
    }
}

void PhraseBookBox::selectionChanged( QListViewItem * /* item */ )
{
    enableDisable();
}

void PhraseBookBox::selectItem( QListViewItem *item )
{
    lv->setSelected( item, TRUE );
    lv->ensureItemVisible( item );
}

void PhraseBookBox::enableDisable()
{
    QListViewItem *item = lv->currentItem();

    sourceLed->blockSignals( TRUE );
    targetLed->blockSignals( TRUE );
    definitionLed->blockSignals( TRUE );

    if ( item == 0 ) {
        sourceLed->setText( QString::null );
        targetLed->setText( QString::null );
        definitionLed->setText( QString::null );
    } else {
        sourceLed->setText( item->text(0) );
        targetLed->setText( item->text(1) );
        definitionLed->setText( item->text(2) );
    }
    sourceLed->setEnabled( item != 0 );
    targetLed->setEnabled( item != 0 );
    definitionLed->setEnabled( item != 0 );
    removeBut->setEnabled( item != 0 );

    sourceLed->blockSignals( FALSE );
    targetLed->blockSignals( FALSE );
    definitionLed->blockSignals( FALSE );

    QLineEdit *led = ( sourceLed->text() == NewPhrase ? sourceLed : targetLed );
    led->setFocus();
    led->selectAll();
}
