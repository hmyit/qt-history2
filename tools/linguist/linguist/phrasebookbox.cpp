/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
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

#include <QtEvents>
#include <QLineEdit>
#include <QMessageBox>
#include <QHeaderView>

#define NewPhrase tr("(New Phrase)")

PhraseBookBox::PhraseBookBox(const QString& filename,
                             const PhraseBook& phraseBook, QWidget *parent)
    : QDialog(parent), blockListSignals(false), fn(filename), pb(phraseBook)
{
    setupUi(this);
    setModal(false);
    source->setBuddy(sourceLed);
    target->setBuddy(targetLed);
    definition->setBuddy(definitionLed);

    phrMdl = new PhraseModel(this);
    phraseList->setModel(phrMdl);
    phraseList->setSelectionBehavior(QAbstractItemView::SelectRows);
    phraseList->setSelectionMode(QAbstractItemView::SingleSelection);
    phraseList->setRootIsDecorated(false);
    phraseList->header()->setResizeMode(QHeaderView::Stretch);
    phraseList->header()->setClickable(true);

    connect(sourceLed, SIGNAL(textChanged(QString)),
        this, SLOT(sourceChanged(QString)));
    connect(targetLed, SIGNAL(textChanged(QString)),
        this, SLOT(targetChanged(QString)));
    connect(definitionLed, SIGNAL(textChanged(QString)),
        this, SLOT(definitionChanged(QString)));
    connect(phraseList->selectionModel(), SIGNAL(currentChanged(const QModelIndex &,
        const QModelIndex &)), this, SLOT(selectionChanged()));
    connect(newBut, SIGNAL(clicked()), this, SLOT(newPhrase()));
    connect(removeBut, SIGNAL(clicked()), this, SLOT(removePhrase()));
    connect(saveBut, SIGNAL(clicked()), this, SLOT(save()));
    connect(closeBut, SIGNAL(clicked()), this, SLOT(accept()));

    foreach(Phrase p, phraseBook) {
        phrMdl->addPhrase(p);
    }

    phrMdl->sort(0, Qt::AscendingOrder);

    enableDisable();
}

void PhraseBookBox::keyPressEvent(QKeyEvent *ev)
{
    // TODO:
    // does not work...
    /*if (ev->key() == Qt::Key_Down || ev->key() == Qt::Key_Up ||
        ev->key() == Qt::Key_Next || ev->key() == Qt::Key_Prior)
        QApplication::sendEvent(phraseList, new QKeyEvent(ev->type(),
        ev->key(), ev->state(), ev->text(), ev->isAutoRepeat(), ev->count()));
    else*/
        QDialog::keyPressEvent( ev );
}

void PhraseBookBox::newPhrase()
{
    Phrase ph;
    ph.setSource(NewPhrase);
    selectItem(phrMdl->addPhrase(ph));
}

void PhraseBookBox::removePhrase()
{
    phrMdl->removePhrase(phraseList->currentIndex());
}

void PhraseBookBox::save()
{
    pb.clear();

    QList<Phrase> pl = phrMdl->phraseList();
    Phrase p;

    for (int i=0; i<pl.count(); i++) {
        p = pl.at(i);
        if (!p.source().isEmpty() && p.source() != NewPhrase)
            pb.append(pl.at(i));
    }

    if (!pb.save(fn))
        QMessageBox::warning(this, tr("Qt Linguist"),tr("Cannot save phrase book '%1'.").arg(fn));
}

void PhraseBookBox::sourceChanged(const QString& source)
{
    QModelIndex index = phraseList->currentIndex();
    if (index.isValid()) {
        Phrase ph = phrMdl->phrase(index);
        ph.setSource(source);
        phrMdl->setPhrase(index, ph);
        sortAndSelectItem(index);
    }
}

void PhraseBookBox::targetChanged(const QString& target)
{
    QModelIndex index = phraseList->currentIndex();
    if (index.isValid()) {
        Phrase ph = phrMdl->phrase(index);
        ph.setTarget(target);
        phrMdl->setPhrase(index, ph);
        sortAndSelectItem(index);
    }
}

void PhraseBookBox::definitionChanged( const QString& definition )
{
    QModelIndex index = phraseList->currentIndex();
    if (index.isValid()) {
        Phrase ph = phrMdl->phrase(index);
        ph.setDefinition(definition);
        phrMdl->setPhrase(index, ph);
        sortAndSelectItem(index);
    }
}

void PhraseBookBox::sortAndSelectItem(const QModelIndex &index)
{
    Phrase curphr = phrMdl->phrase(index);
    phrMdl->resort();
    QModelIndex newIndex = phrMdl->index(curphr);

    // TODO
    // phraseList->blockSignals(bool) does not work (?)
    blockListSignals = true;
    selectItem(newIndex);
    blockListSignals = false;
}

void PhraseBookBox::selectionChanged()
{
    if (!blockListSignals)
        enableDisable();
}

void PhraseBookBox::selectItem(const QModelIndex &index)
{
    phraseList->scrollTo(index);
    phraseList->setCurrentIndex(index);
}

void PhraseBookBox::enableDisable()
{
    QModelIndex index = phraseList->currentIndex();

    sourceLed->blockSignals(true);
    targetLed->blockSignals(true);
    definitionLed->blockSignals(true);

    bool indexValid = index.isValid();

    if (indexValid) {
        Phrase p = phrMdl->phrase(index);
        sourceLed->setText(p.source().simplified());
        targetLed->setText(p.target().simplified());
        definitionLed->setText(p.definition());
    }
    else {
        sourceLed->setText(QString());
        targetLed->setText(QString());
        definitionLed->setText(QString());
    }

    sourceLed->setEnabled(indexValid);
    targetLed->setEnabled(indexValid);
    definitionLed->setEnabled(indexValid);
    removeBut->setEnabled(indexValid);

    sourceLed->blockSignals(false);
    targetLed->blockSignals(false);
    definitionLed->blockSignals(false);

    QLineEdit *led = (sourceLed->text() == NewPhrase ? sourceLed : targetLed);
    led->setFocus();
    led->selectAll();
}
