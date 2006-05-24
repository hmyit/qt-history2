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

#include "meta.h"
#include "project.h"
#include "option.h"
#include <qdir.h>

QMap<QString, QMap<QString, QStringList> > QMakeMetaInfo::cache_vars;

QMakeMetaInfo::QMakeMetaInfo()
{

}


bool
QMakeMetaInfo::readLib(QString lib)
{
    clear();
    QString meta_file = findLib(lib);

    if(cache_vars.contains(meta_file)) {
        vars = cache_vars[meta_file];
        return true;
    }

    bool ret = false;
    if(!meta_file.isNull()) {
        if(meta_file.endsWith(Option::pkgcfg_ext)) {
            if((ret=readPkgCfgFile(meta_file)))
                meta_type = "pkgcfg";
        } else if(meta_file.endsWith(Option::libtool_ext)) {
            if((ret=readLibtoolFile(meta_file)))
                meta_type = "libtool";
        } else if(meta_file.endsWith(Option::prl_ext)) {
            QMakeProject proj;
            if(!proj.read(Option::fixPathToLocalOS(meta_file), QMakeProject::ReadProFile))
                return false;
            meta_type = "qmake";
            vars = proj.variables();
            ret = true;
        } else {
            warn_msg(WarnLogic, "QMakeMetaInfo: unknown file format for %s", meta_file.toLatin1().constData());
        }
    }
    if(ret)
        cache_vars.insert(meta_file, vars);
    return ret;
}


void
QMakeMetaInfo::clear()
{
    vars.clear();
}


QString
QMakeMetaInfo::findLib(QString lib)
{
    if((lib[0] == '\'' || lib[0] == '"') &&
       lib[lib.length()-1] == lib[0])
	lib = lib.mid(1, lib.length()-2);
    lib = Option::fixPathToLocalOS(lib);

    QString ret;
    QString extns[] = { Option::prl_ext, /*Option::pkgcfg_ext, Option::libtool_ext,*/ QString() };
    for(int extn = 0; !extns[extn].isNull(); extn++) {
        if(lib.endsWith(extns[extn]))
            ret = QFile::exists(lib) ? lib : QString();
    }
    if(ret.isNull()) {
        for(int extn = 0; !extns[extn].isNull(); extn++) {
            if(QFile::exists(lib + extns[extn])) {
                ret = lib + extns[extn];
                break;
            }
        }
    }
    if(ret.isNull())
        debug_msg(2, "QMakeMetaInfo: Cannot find info file for %s", lib.toLatin1().constData());
    else
        debug_msg(2, "QMakeMetaInfo: Found info file %s for %s", ret.toLatin1().constData(), lib.toLatin1().constData());
    return ret;
}


bool
QMakeMetaInfo::readLibtoolFile(const QString &f)
{
    /* I can just run the .la through the .pro parser since they are compatible.. */
    QMakeProject proj;
    if(!proj.read(Option::fixPathToLocalOS(f), QMakeProject::ReadProFile))
        return false;
    QString dirf = Option::fixPathToTargetOS(f).section(Option::dir_sep, 0, -2);
    if(dirf == f)
        dirf = "";
    else if(!dirf.isEmpty() && !dirf.endsWith(Option::output_dir))
        dirf += Option::dir_sep;
    QMap<QString, QStringList> &v = proj.variables();
    for(QMap<QString, QStringList>::Iterator it = v.begin(); it != v.end(); ++it) {
        QStringList lst = it.value();
        if(lst.count() == 1 && (lst.first().startsWith("'") || lst.first().startsWith("\"")) &&
           lst.first().endsWith(QString(lst.first()[0])))
            lst = QStringList(lst.first().mid(1, lst.first().length() - 2));
        if(!vars.contains("QMAKE_PRL_TARGET") &&
           (it.key() == "dlname" || it.key() == "library_names" || it.key() == "old_library")) {
            QString dir = v["libdir"].first();
            if((dir.startsWith("'") || dir.startsWith("\"")) && dir.endsWith(QString(dir[0])))
                dir = dir.mid(1, dir.length() - 2);
            dir = dir.trimmed();
            if(!dir.isEmpty() && !dir.endsWith(Option::dir_sep))
                dir += Option::dir_sep;
            if(lst.count() == 1)
                lst = lst.first().split(" ");
            for(QStringList::Iterator lst_it = lst.begin(); lst_it != lst.end(); ++lst_it) {
                bool found = false;
                QString dirs[] = { "", dir, dirf, dirf + ".libs" + QDir::separator(), "(term)" };
                for(int i = 0; !found && dirs[i] != "(term)"; i++) {
                    if(QFile::exists(dirs[i] + (*lst_it))) {
                        QString targ = dirs[i] + (*lst_it);
                        if(QDir::isRelativePath(targ))
                            targ.prepend(qmake_getpwd() + QDir::separator());
                        vars["QMAKE_PRL_TARGET"] << targ;
                        found = true;
                    }
                }
                if(found)
                    break;
            }
        } else if(it.key() == "dependency_libs") {
            if(lst.count() == 1) {
                QString dep = lst.first();
                if((dep.startsWith("'") || dep.startsWith("\"")) && dep.endsWith(QString(dep[0])))
                    dep = dep.mid(1, dep.length() - 2);
                lst = dep.trimmed().split(" ");
            }
            QMakeProject *conf = NULL;
            for(QStringList::Iterator lit = lst.begin(); lit != lst.end(); ++lit) {
                if((*lit).startsWith("-R")) {
                    if(!conf) {
                        conf = new QMakeProject;
                        conf->read(QMakeProject::ReadAll ^ QMakeProject::ReadProFile);
                    }
                    if(!conf->isEmpty("QMAKE_LFLAGS_RPATH"))
                        (*lit) = conf->first("QMAKE_LFLAGS_RPATH") + (*lit).mid(2);
                }
            }
            if(conf)
                delete conf;
            vars["QMAKE_PRL_LIBS"] += lst;
        }
    }
    return true;
}

bool
QMakeMetaInfo::readPkgCfgFile(const QString &f)
{
    fprintf(stderr, "Must implement reading in pkg-config files (%s)!!!\n", f.toLatin1().constData());
    return false;
}
