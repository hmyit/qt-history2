#include "p4.h"
#include <qprocess.h>
#include <qmessagebox.h>
#include <qtextview.h>
#include <qlistview.h>
#include <qheader.h>
#include <qmultilineedit.h>
#include <qapplication.h>
#include <qregexp.h>
#include "diffdialog.h"
#include "submitdialog.h"

QDict<P4Info> *P4Info::_files = 0;
QString *P4Info::userName = 0;
QString *P4Info::clientName = 0;

QDict<P4Info> *P4Info::files()
{
    if ( !P4Info::_files )
	P4Info::_files = new QDict<P4Info>(53);

    return P4Info::_files;
}

P4Action::P4Action( const QString &filename )
    : QObject(), file( filename ), process( 0 )
{
}

P4Action::~P4Action()
{
    delete process;
}

bool P4Action::run( const QString &command )
{
    if ( !process ) {
	process = new QProcess( this );
	connect( process, SIGNAL( readyReadStdout() ),
		 this, SLOT( newData() ) );
	connect( process, SIGNAL( processExited() ),
		 this, SLOT( processExited() ) );
    }
    process->setArguments( QStringList::split( ' ', command ) );
    return process->start();
}

bool P4Action::run( const QString &command, const QString &in )
{
    if ( run( command ) ) {
	connect( process, SIGNAL(wroteToStdin()), this, SLOT(cmdProcessed()) );
	process->writeToStdin( in );
	return TRUE;
    }
    return FALSE;
}

void P4Action::cmdProcessed()
{
    process->closeStdin();
}

void P4Action::updateStats()
{
    emit showStatusBarMessage( data() );
    P4FStat* fstat = new P4FStat( fileName() );
    connect( fstat, SIGNAL(finished(const QString&,P4Info*)), this, SLOT(newStats(const QString&,P4Info*)) );
    fstat->execute();
}

void P4Action::newData()
{
    p4Data += QString( process->readStdout() );
}

void P4Action::newStats( const QString &s, P4Info *p4i )
{
    emit finished( s, p4i );

    delete this;
}

bool P4Action::success()
{
    return process && process->exitStatus() == 0;
}

P4Init::P4Init()
: P4Action( QString::null )
{
}

bool P4Init::execute()
{
    return run( "p4 info" );
}

void P4Init::processExited()
{
    QStringList entries = QStringList::split( '\n', data() );
    QStringList userEntry = entries.grep( "user name:", FALSE );
    P4Info::userName = new QString( QStringList::split( ' ', userEntry[0] )[2] );
    QStringList clientEntry = entries.grep( "client name:", FALSE );
    P4Info::clientName = new QString( QStringList::split( ' ', clientEntry[0] )[2] );
}

P4FStat::P4FStat( const QString& filename )
: P4Action( filename )
{
}

bool P4FStat::execute()
{
    return run( QString("p4 fstat \"%1\"").arg( fileName() ) );
}

void P4FStat::processExited()
{
    bool wasIgnore = FALSE;
    P4Info *old = P4Info::files()->find( fileName() );
    if ( old ) {
	wasIgnore = old->ignoreEdit;
	P4Info::files()->remove( fileName() );
	delete old;
    }

    P4Info* p4i = new P4Info;
    QStringList entries = QStringList::split( '\n', data() );

    p4i->controlled = FALSE;
    p4i->action = P4Info::None;
    p4i->ignoreEdit = wasIgnore;

    if ( data().find( "clientFile" ) != -1 ) {						    // The file is somehow known
	QStringList dfEntry = entries.grep( "depotFile" );
	if ( dfEntry.count() ) {
	    QStringList headActionEntry = entries.grep( "... headAction" );
	    if ( headActionEntry.count() ) {
		QString headAction = QStringList::split( ' ', headActionEntry[0] )[2];
		if ( headAction.stripWhiteSpace() != "delete" ) {			    // Maybe it's already deleted
		    p4i->controlled = TRUE;
		    p4i->action = P4Info::None;
		    p4i->depotFile = QStringList::split( ' ', dfEntry[0] )[2];
		    p4i->depotFile = p4i->depotFile.stripWhiteSpace();
		    QStringList actionEntry = entries.grep( "... action" );
		    if ( actionEntry.count() ) {
			QString act = QStringList::split( ' ', actionEntry[0] )[2];	    // Get current action
			act.contains( "edit", FALSE );
			if ( act.stripWhiteSpace() == "edit" )
			    p4i->action = P4Info::Edit;
			else if ( act.stripWhiteSpace() == "add" )
			    p4i->action = P4Info::Add;
			else if ( act.stripWhiteSpace() == "delete" )
			    p4i->action = P4Info::Delete;
		    }
		    QStringList headRevEntry = entries.grep( "... headRev" );		    // Compare revisions
		    if ( headRevEntry.count() ) {
			int headrev = 0;
			int haverev = 0;

			QString head = QStringList::split( ' ', headRevEntry[0] )[2];
			headrev = head.toInt();
			QStringList haveRevEntry = entries.grep( "... haveRev" );
			if ( haveRevEntry.count() ) {
			    QString have = QStringList::split( ' ', haveRevEntry[0] )[2];
			    haverev = have.toInt();
			}

			p4i->uptodate = !(haverev < headrev);
		    }
		}
	    }
	}
    }
    P4Info::files()->insert( fileName(), p4i );

    emit finished( fileName(), p4i );
    delete this;
}

P4Sync::P4Sync( const QString &filename )
: P4Action( filename )
{
}

bool P4Sync::execute()
{
    return run( QString("p4 sync %1").arg( fileName() ) );
}

void P4Sync::processExited()
{
    updateStats();
}

P4Edit::P4Edit( const QString &filename, bool s )
    : P4Action( filename ), silent( s )
{
}

bool P4Edit::execute()
{
    P4Info* p4i = P4Info::files()->find( fileName() );
    if ( !p4i ) {
	P4FStat* fstat = new P4FStat( fileName() );
	connect( fstat, SIGNAL(finished( const QString&, P4Info* )), this, SLOT(fStatResults(const QString&,P4Info*) ) );
	return fstat->execute();
    }  else {
	fStatResults( fileName(), p4i );
    }
    return TRUE;
}

void P4Edit::processExited()
{
    updateStats();
}

void P4Edit::fStatResults( const QString& filename, P4Info *p4i)
{
    P4Info::files()->remove( filename );
    P4Info::files()->insert( filename, p4i );
    if ( !p4i->controlled ) {
	if ( silent )
	    QMessageBox::information( 0, tr( "P4 Edit" ), tr( "Opening the file<pre>%1</pre>for edit failed!" ).arg( fileName() ) );
	return;
    } 
    if ( p4i->action == P4Info::None ) {
	if ( !silent ) {
	    if ( p4i->ignoreEdit )
		return;
	    if ( QMessageBox::information( 0, tr( "P4 Edit" ), tr( "The file<pre>%1</pre>is under Perforce Source Control "
								   "and not opened for edit.\n"
								   "Do you want to open it for edit?" ).
					   arg( fileName() ),
					   tr( "&Yes" ), tr( "&No" ) ) == 1 ) {
		p4i->ignoreEdit = TRUE;
		return;
	    }
	}
	run( QString("p4 edit \"%1\"").arg( fileName() ) );
    }
}

P4Submit::P4Submit( const QString &filename )
: P4Action( filename )
{
}

bool P4Submit::execute()
{
    SubmitDialog *dialog = new SubmitDialog( qApp->mainWidget(), 0, TRUE );

    QDictIterator<P4Info> it( *P4Info::files() );
    while ( it.current() ) {
	if ( it.current()->controlled && it.current()->action != P4Info::None ) {
	    QCheckListItem* item = new QCheckListItem( dialog->fileList, it.currentKey(), QCheckListItem::CheckBox );
	    item->setText( 1, it.current()->depotFile );
	    switch ( it.current()->action ) {
	    case P4Info::Edit:
		item->setText( 2, "Edit" );
		break;
	    case P4Info::Add:
		item->setText( 2, "Add" );
		break;
	    case P4Info::Delete:
		item->setText( 2, "Delete" );
		break;
	    default:
		break;
	    }
	}	
	++it;
    }
    dialog->description->setText( "<enter description here>" );

    if ( dialog->exec() != QDialog::Accepted )
	return FALSE;

    QString description = dialog->description->text().replace( QRegExp("\\n"), "\n\t" );

    QString buffer = "Change:\tnew\n\n";
    buffer += "Client:\t" + *P4Info::clientName + "\n\n";
    buffer += "User:\t" + *P4Info::userName + "\n\n";
    buffer += "Status:\tnew\n\n";
    buffer += "Description:\n\t";
    buffer += description + "\n\n";
    buffer += "Files:\n";

    bool haveFile = FALSE;
    QListViewItemIterator lvit( dialog->fileList );
    while ( lvit.current() ) {
	QCheckListItem* item = (QCheckListItem*)lvit.current();
	++lvit;
	if ( !item->isOn() )
	    continue;
	P4Info* p4i = P4Info::files()->find( item->text( 0 ) );
	if ( !p4i )
	    continue;
	buffer += "\t" + p4i->depotFile + "\n";
	haveFile = TRUE;
    }

    if ( !haveFile )
	return FALSE;
    return run( QString("p4 submit -i"), buffer );
}

void P4Submit::processExited()
{
    updateStats();
}


P4Revert::P4Revert( const QString &filename )
: P4Action( filename )
{
}

bool P4Revert::execute()
{
    P4Info *p4i = P4Info::files()->find( fileName() );
    if ( p4i && p4i->action == P4Info::Edit ) {
	if ( QMessageBox::information( 0, tr( "P4 Revert" ), tr( "<p>Reverting will <b>overwrite</b> all changes to the local file<pre>%1</pre></p>"
						"<p>Proceed with revert?</p>" ).
						arg( fileName() ),
						tr( "&Yes" ), tr( "&No" ) ) == 1 )
	    return FALSE;
    }

    return run( QString("p4 revert %1").arg( fileName() ) );
}

void P4Revert::processExited()
{
    updateStats();
}

P4Add::P4Add( const QString &filename )
: P4Action( filename )
{
}

bool P4Add::execute()
{
    return run( QString("p4 add %1").arg( fileName() ) );
}

void P4Add::processExited()
{
    updateStats();
}

P4Delete::P4Delete( const QString &filename )
: P4Action( filename )
{
}

bool P4Delete::execute()
{
    P4Info *p4i = P4Info::files()->find( fileName() );
    if ( p4i ) {
	if ( QMessageBox::information( 0, tr( "P4 Delete" ), tr( "<p>This will delete the <b>local</b> file <pre>%1</pre></p>"
						"<p>The <b>depot</b> file<pre>%2</pre>will be deleted by the next sync.</p>"
						"<p>Proceed with delete?</p>" ).
						arg( fileName() ).arg( p4i->depotFile ),
						tr( "&Yes" ), tr( "&No" ) ) == 1 )
	return FALSE;
    }

    return run( QString("p4 delete %1").arg( fileName() ) );
}

void P4Delete::processExited()
{
    updateStats();
}

P4Diff::P4Diff( const QString &filename )
: P4Action( filename )
{
}

bool P4Diff::execute()
{
    return run( QString("p4 diff %1").arg( fileName() ) );
}

void P4Diff::processExited()
{
    if ( success() ) {
	int fstEndl = data().find( '\n' );
	QString caption = data().mid( 4, fstEndl-9 );
	caption = caption.replace( QRegExp("===="), "" );
	QString diff = data().mid( fstEndl+1 );
	if ( !!diff ) {
	    diff = diff.replace( QRegExp( "<" ), "&lt;" );
	    diff = diff.replace( QRegExp( ">"), "&gt;" );
	    diff = diff.replace( QRegExp( "\\n\\&lt;" ), "</pre><font color=\"red\"><pre>" );
	    diff = diff.replace( QRegExp( "\\n\\&gt;" ), "</pre><font color=\"blue\"><pre>" );
	    diff = "<font face=\"Courier\">" + diff + "</font>";
	    DiffDialog* dialog = new DiffDialog( qApp->mainWidget(), 0, TRUE );
	    dialog->setCaption( caption );
	    dialog->view->setText( diff );
	    dialog->exec();
	}
    }

    delete this;
}
