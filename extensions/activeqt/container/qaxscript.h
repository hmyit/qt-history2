#ifndef QAXSCRIPT_H
#define QAXSCRIPT_H

#include <qaxobject.h>

class QAxBase;
class QAxScript;
class QAxScriptSite;
class QAxScriptEngine;
class QAxScriptManager;
class QAxScriptManagerPrivate;
struct IActiveScript;

class QAxScriptEngine : public QAxObject
{
public:
    enum State {
	Uninitialized = 0,
	Initialized = 5,
	Started = 1,
	Connected = 2,
	Disconnected = 3,
	Closed = 4
    };

    QAxScriptEngine(const QString &language, QAxScript *script);
    ~QAxScriptEngine();

    bool isValid() const;
    bool hasIntrospection() const;

    QString scriptLanguage() const;

    State state() const;
    void setState(State st);

    void addItem(const QString &name);

    long queryInterface( const QUuid &, void** ) const;

protected:
    bool initialize(IUnknown** ptr);

private:
    QAxScript *script_code;
    IActiveScript *engine;

    QString script_language;
};

class QAxScript : public QObject
{
    Q_OBJECT

public:
    enum FunctionFlags {
	FunctionNames = 0,
	FunctionSignatures	
    };

    QAxScript(const QString &name, QAxScriptManager *manager);
    ~QAxScript();

    bool load(const QString &code, const QString &language = QString::null);

    QStringList functions(FunctionFlags = FunctionNames) const;

    QString scriptCode() const;
    QString scriptName() const;
    QAxScriptEngine *scriptEngine() const;

    QVariant call(const QString &function, QValueList<QVariant> &arguments = QValueList<QVariant>());

signals:
    void entered();
    void finished();
    void finished(const QVariant &result);
    void finished(int code, const QString &source,const QString &description, const QString &help);
    void stateChanged(int state);
    void error(int code, const QString &description, int sourcePosition, const QString &sourceText);

private:
    friend class QAxScriptSite;
    friend class QAxScriptEngine;

    void updateObjects();
    QAxBase *findObject(const QString &name);

    QString script_name;
    QString script_code;
    QAxScriptManager *script_manager;
    QAxScriptEngine *script_engine;
    QAxScriptSite *script_site;
};

class QAxScriptManager : public QObject
{
    Q_OBJECT

public:
    QAxScriptManager( QObject *parent = 0, const char *name = 0 );
    ~QAxScriptManager();

    void addObject(QAxBase *object);
    void addObject(QObject *object);

    QStringList functions(QAxScript::FunctionFlags = QAxScript::FunctionNames) const;
    QStringList scriptNames() const;
    QAxScript *script(const QString &name) const;

    QAxScript* load(const QString &code, const QString &name, const QString &language);
    QAxScript* load(const QString &file, const QString &name);

    QVariant call(const QString &function, QValueList<QVariant> &arguments = QValueList<QVariant>());

    static bool registerEngine(const QString &name, const QString &extension, const QString &code = QString());
    static QString scriptFileFilter();

signals:
    void error(QAxScript *script, int code, const QString &description, int sourcePosition, const QString &sourceText);

private slots:
    void objectDestroyed(QObject *o);
    void scriptError(int code, const QString &description, int sourcePosition, const QString &sourceText);

private:
    friend class QAxScript;
    QAxScriptManagerPrivate *d;

    void updateScript(QAxScript*);
    QAxScript *scriptForFunction(const QString &function) const;
};


// QAxScript inlines

inline QString QAxScript::scriptCode() const
{
    return script_code;
}

inline QString QAxScript::scriptName() const
{
    return script_name;
}

inline QAxScriptEngine *QAxScript::scriptEngine() const
{
    return script_engine;
}

// QAxScriptEngine inlines

inline bool QAxScriptEngine::isValid() const
{
    return engine != 0;
}

inline QString QAxScriptEngine::scriptLanguage() const
{
    return script_language;
}

// QAxScriptManager inlines

inline void QAxScriptManager::addObject(QObject *object)
{
    extern QAxBase *qax_create_object_wrapper(QObject*);
    QAxBase *wrapper = qax_create_object_wrapper(object);
    addObject(wrapper);
}

#endif // QAXSCRIPT_H
