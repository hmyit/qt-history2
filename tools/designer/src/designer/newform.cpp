/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "newform.h"
#include "qdesigner_workbench.h"
#include "qdesigner_actions.h"
#include "qdesigner_formwindow.h"
#include "qdesigner_settings.h"

#include <qdesigner_formbuilder_p.h>
#include <sheet_delegate_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/qdebug.h>
#include <QtGui/QHeaderView>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

enum NewForm_CustomRole
{
    TemplateNameRole = Qt::UserRole + 100
};

NewForm::NewForm(QDesignerWorkbench *workbench, QWidget *parentWidget, const QString &fileName)
    : QDialog(parentWidget,
#ifdef Q_WS_MAC
            Qt::Tool |
#endif
            Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      m_workbench(workbench),
      m_createButton(new QPushButton(QApplication::translate("NewForm", "C&reate", 0, QApplication::UnicodeUTF8))),
      m_recentButton(new QPushButton(QApplication::translate("NewForm", "Recent", 0,  QApplication::UnicodeUTF8))),
      m_fileName(fileName)
{
    ui.setupUi(this);
    ui.treeWidget->setItemDelegate(new qdesigner_internal::SheetDelegate(ui.treeWidget, this));
    ui.treeWidget->header()->hide();
    ui.treeWidget->header()->setStretchLastSection(true);
    ui.lblPreview->setBackgroundRole(QPalette::Base);
    ui.chkShowOnStartup->setChecked(QDesignerSettings().showNewFormOnStartup());
    ui.buttonBox->clear();
    ui.buttonBox->addButton(QApplication::translate("NewForm", "&Close", 0,
                                        QApplication::UnicodeUTF8), QDialogButtonBox::RejectRole);
    ui.buttonBox->addButton(m_createButton, QDialogButtonBox::AcceptRole);
    m_createButton->setEnabled(false);
    ui.buttonBox->addButton(QApplication::translate("NewForm", "&Open...", 0,
                                    QApplication::UnicodeUTF8), QDialogButtonBox::ActionRole);
    ui.buttonBox->addButton(m_recentButton, QDialogButtonBox::ActionRole);
    QDesignerActions *da = workbench->actionManager();
    QMenu *recentFilesMenu = new QMenu(tr("&Recent Forms"), m_recentButton);
    // Pop the "Recent Files" stuff in here.
    foreach(QAction *recentAction, da->recentFilesActions()->actions()) {
        recentFilesMenu->addAction(recentAction);
        connect(recentAction, SIGNAL(triggered()), this, SLOT(recentFileChosen()));
    }
    m_recentButton->setMenu(recentFilesMenu);

    loadFrom(QLatin1String(":/trolltech/designer/templates/forms"), true);

    QDesignerSettings settings;
    foreach(QString path, settings.formTemplatePaths())
        loadFrom(path, false);
}

NewForm::~NewForm()
{
    QDesignerSettings().setShowNewFormOnStartup(ui.chkShowOnStartup->isChecked());
}

void NewForm::recentFileChosen()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    if (action->objectName() == QLatin1String("__qt_action_clear_menu_"))
        return;
    close();
}

void NewForm::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    if (current && current->parent()) {
        const QIcon icon = formPreviewIcon(current->data(0, TemplateNameRole).toString());
        if (icon.isNull()) {
            m_createButton->setEnabled(false);
            ui.lblPreview->setText(tr("Error loading form"));
        } else {
            m_createButton->setEnabled(true);
            m_createButton->setDefault(true);
            ui.lblPreview->setPixmap(icon.pixmap(QSize(256, 256)));
        }
    } else {
        m_createButton->setEnabled(false);
        ui.lblPreview->setText(tr("Choose a template for a preview"));
    }
}

void NewForm::on_treeWidget_itemActivated(QTreeWidgetItem *item)
{
    if (item->data(0, TemplateNameRole).isValid())
        m_createButton->animateClick(0);
}

void NewForm::on_buttonBox_clicked(QAbstractButton *btn)
{
    switch (ui.buttonBox->buttonRole(btn)) {
    case QDialogButtonBox::RejectRole:
        close();
        break;
    case QDialogButtonBox::ActionRole:
        if (btn != m_recentButton) {
            hide();
            m_fileName.clear();
            if (m_workbench->actionManager()->openForm())
                close();
            else
                show();
        }
        break;
    case QDialogButtonBox::AcceptRole:
        if (const QTreeWidgetItem *item = ui.treeWidget->currentItem()) { 
            if (openTemplate(item->data(0, TemplateNameRole).toString())) {
                close();
            } else {
                show();
            }
        }
        break;
    default:
        break;
    }
}

QDesignerWorkbench *NewForm::workbench() const
{
    return m_workbench;
}

QIcon NewForm::formPreviewIcon(const QString &fileName)
{
    QIcon result;

    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        qdesigner_internal::QDesignerFormBuilder formBuilder(workbench()->core(), qdesigner_internal::QDesignerFormBuilder::UseContainerExtension);

        QWidget *fake = new QWidget(0);

        fake->createWinId();
        fake->setAttribute(Qt::WA_WState_Visible);

        if (QWidget *widget = formBuilder.load(&f, fake)) {
            widget->setParent(fake, 0);
            widget->show();
            f.close();

            const int margin = 7;
            const int shadow = 7;

            const QPixmap pix = QPixmap::grabWidget(widget);
            QImage image = pix.toImage();
            image = image.scaled(256 - margin * 2,
                                 256 - margin * 2,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);

            QImage dest(image.width() + margin * 2, image.height() + margin * 2, QImage::Format_ARGB32_Premultiplied);
            dest.fill(0);

            QPainter p(&dest);
            p.drawImage(margin, margin, image);

            p.setPen(QPen(palette().brush(QPalette::WindowText), 0));

            p.drawRect(margin-1, margin-1, image.width() + 1, image.height() + 1);

            const QColor dark(Qt::darkGray);
            const QColor light(Qt::transparent);

            // right shadow
            {
                const QRect rect(margin + image.width() + 1, margin + shadow, shadow, image.height() - shadow + 1);
                QLinearGradient lg(rect.topLeft(), rect.topRight());
                lg.setColorAt(0, dark);
                lg.setColorAt(1, light);
                p.fillRect(rect, lg);
            }

            // bottom shadow
            {
                const QRect rect(margin + shadow, margin + image.height() + 1, image.width() - shadow + 1, shadow);
                QLinearGradient lg(rect.topLeft(), rect.bottomLeft());
                lg.setColorAt(0, dark);
                lg.setColorAt(1, light);
                p.fillRect(rect, lg);
            }

            // bottom/right corner shadow
            {
                const QRect rect(margin + image.width() + 1, margin + image.height() + 1, shadow, shadow);
                QRadialGradient g(rect.topLeft(), shadow);
                g.setColorAt(0, dark);
                g.setColorAt(1, light);
                p.fillRect(rect, g);
            }

            // top/right corner
            {
                const QRect rect(margin + image.width() + 1, margin, shadow, shadow);
                QRadialGradient g(rect.bottomLeft(), shadow);
                g.setColorAt(0, dark);
                g.setColorAt(1, light);
                p.fillRect(rect, g);
            }

            // bottom/left corner
            {
                const QRect rect(margin, margin + image.height() + 1, shadow, shadow);
                QRadialGradient g(rect.topRight(), shadow);
                g.setColorAt(0, dark);
                g.setColorAt(1, light);
                p.fillRect(rect, g);
            }

            p.end();

            result = QPixmap::fromImage(dest);
        }

        fake->deleteLater();
    }

    return result;
}

void NewForm::loadFrom(const QString &path, bool resourceFile)
{
    const QDir dir(path);

    if (!dir.exists())
        return;

    // Iterate through the directory and add the templates
    const QFileInfoList list = dir.entryInfoList(QStringList(QLatin1String("*.ui")), QDir::Files);

    if (list.isEmpty())
        return;

    const QChar separator = resourceFile ? QChar(QLatin1Char('/'))
                                         : QDir::separator();
    QTreeWidgetItem *root = new QTreeWidgetItem(ui.treeWidget);
    // Try to get something that is easy to read.
    QString visiblePath = path;
    int index = visiblePath.lastIndexOf(separator);
    if (index != -1) {
        // try to find a second slash, just to be a bit better.
        const int index2 = visiblePath.lastIndexOf(separator, index - 1);
        if (index2 != -1)
            index = index2;
        visiblePath = visiblePath.mid(index + 1);
        visiblePath = QDir::toNativeSeparators(visiblePath);
    }

    root->setText(0, visiblePath.replace(QLatin1String("_"), QLatin1String(" ")));
    root->setToolTip(0, path);

    foreach(QFileInfo fi, list) {
        if (!fi.isFile())
            continue;

        QTreeWidgetItem *item = new QTreeWidgetItem(root);
        item->setText(0, fi.baseName().replace(QLatin1String("_"), QLatin1String(" ")));
        item->setData(0, TemplateNameRole, fi.absoluteFilePath());

        QTreeWidgetItem *i = ui.treeWidget->currentItem();
        if (i == 0) {
            ui.treeWidget->setCurrentItem(item);
            ui.treeWidget->setItemSelected(item, true);
        }
    }
    ui.treeWidget->setItemExpanded(root, true);
}

void NewForm::on_treeWidget_itemPressed(QTreeWidgetItem *item)
{
    if (item && !item->parent())
        ui.treeWidget->setItemExpanded(item, !ui.treeWidget->isItemExpanded(item));
}

// Figure out a title for a new file.
QString NewForm::newUntitledTitle() const
{    
    int maxUntitled = 0;
    const int totalWindows = m_workbench->formWindowCount();
    // This will cause some problems with i18n, but for now I need the string to be "static"
    QRegExp rx(QLatin1String("untitled( (\\d+))?"));
    for (int i = 0; i < totalWindows; ++i) {
        QString title = m_workbench->formWindow(i)->windowTitle();
        title.replace(QLatin1String("[*]"), QLatin1String(""));
        if (rx.indexIn(title) != 1) {
            if (maxUntitled == 0)
                ++maxUntitled;
            if (rx.numCaptures() > 1)
                maxUntitled = qMax(rx.cap(2).toInt(), maxUntitled);
        }
    }

    QString newTitle = QLatin1String("untitled");

    if (maxUntitled) {
        newTitle += QLatin1Char(' ');
        newTitle += QString::number(maxUntitled + 1);
    }

    newTitle.append(QLatin1String("[*]"));
    return newTitle;
}


// Figure out a title for a new file.
QString NewForm::newFileTitle(QString &fileName) const
{    
    int maxUntitled = 0;
    const int totalWindows = m_workbench->formWindowCount();

    for (int i = 0; i < totalWindows; ++i) {
        if (fileName == m_workbench->formWindow(i)->editor()->fileName())
            ++maxUntitled;
    }
     
    const QString baseName  = QFileInfo(fileName).fileName();
    QString newTitle = baseName;    

    if (maxUntitled) {
        newTitle += QLatin1Char(' ');
        newTitle += QString::number(maxUntitled + 1);
        fileName.replace(baseName, newTitle);
    }
    
    newTitle.append(QLatin1String("[*]"));
    return newTitle;
}

bool NewForm::openTemplate(const QString &templateFileName)
{
    
    const QString  newTitle = m_fileName.isEmpty() ? newUntitledTitle() : newFileTitle(m_fileName);
    QString errorMessage;
    if (!workbench()->openTemplate(templateFileName,
                                   m_fileName,
                                   newTitle,
                                   &errorMessage)) {        
        QMessageBox::warning(this, tr("Read error"), errorMessage);        
        return false;
    }
    return true;
}
