/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "projectsettingsimpl.h"
#include "project.h"
#include "formwindow.h"
#include "metadatabase.h"
#include "mainwindow.h"
#include "asciivalidator.h"
#include "mainwindow.h"
#include "sourcefile.h"
#include "workspace.h"

#include <qlineedit.h>
#include <qtextedit.h>
#include <qcheckbox.h>
#include <qfiledialog.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qpushbutton.h>
#include <qlabel.h>

/*
 *  Constructs a ProjectSettings which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
ProjectSettings::ProjectSettings( Project *pro, QWidget* parent,  const char* name, bool modal, WFlags fl )
    : ProjectSettingsBase( parent, name, modal, fl ), project( pro )
{
    connect( buttonHelp, SIGNAL( clicked() ), MainWindow::self, SLOT( showDialogHelp() ) );

    editProjectFile->setFocus();

    if ( project->isDummy() ) {
	editProjectFile->setEnabled( false );
	editProjectFile->setText( project->projectName() );
    } else {
	if ( project->fileName().isEmpty() || project->fileName() == ".pro" ) {
	    editProjectFile->setText( tr( "unnamed.pro" ) );
	    editProjectFile->selectAll();
	} else {
	    editProjectFile->setText( project->fileName() );
	}
    }

    editDatabaseFile->setText( project->databaseDescription() );

    comboLanguage->insertStringList( MetaDataBase::languages() );
    for ( int j = 0; j < (int)comboLanguage->count(); ++j ) {
	if ( project->language() == comboLanguage->text( j ) ) {
	    comboLanguage->setCurrentItem( j );
	    break;
	}
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
ProjectSettings::~ProjectSettings()
{
}

void ProjectSettings::chooseDatabaseFile()
{
    QString fn = QFileDialog::getSaveFileName( QString::null, tr( "Database Files (*.db);;All Files (*)" ), this );
    if ( fn.isEmpty() )
	return;
    editDatabaseFile->setText( fn );
}

void ProjectSettings::chooseProjectFile()
{
    QString fn = QFileDialog::getSaveFileName( QString::null, tr( "Project Files (*.pro);;All Files (*)" ), this );
    if ( fn.isEmpty() )
	return;
    editProjectFile->setText( fn );
}

void ProjectSettings::helpClicked()
{
}

void ProjectSettings::okClicked()
{
    // ### check for validity
    project->setFileName( editProjectFile->text(), false );
    project->setDatabaseDescription( editDatabaseFile->text() );
    project->setLanguage( comboLanguage->text( comboLanguage->currentItem() ) );
    project->setModified( true );
    accept();
}

void ProjectSettings::languageChanged( const QString & )
{
}
