#include <QFile>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QTextStream>

#include <QtDebug>

typedef QPair<QString, QString> QStringPair;

QString readSourceFile(const QString &sourceFile, bool fragmentProgram = false)
{
    QFile file(sourceFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Missing source file" << sourceFile;
        exit(0);
    }

    QString source;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (fragmentProgram && line[0] == '#' && !line.startsWith("#var"))
            continue;

        if (fragmentProgram)
            source.append("    \"");

        source.append(line);

        if (fragmentProgram)
            source.append("\\n\"");

        source.append('\n');
    }

    if (fragmentProgram)
        source.append("    ;\n");

    return source;
}

QList<QStringPair> readConf(const QString &confFile)
{
    QFile file(confFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Missing file" << confFile;
        exit(0);
    }

    QList<QStringPair> result;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith('#'))
            continue;

        QTextStream lineStream(&line);

        QString enumerator;
        QString sourceFile;

        lineStream >> enumerator;

        if (lineStream.atEnd()) {
            qDebug() << "Error in file" << confFile << "(" << enumerator << ")";
            exit(0);
        }

        lineStream >> sourceFile;

        result << QStringPair(enumerator, readSourceFile(sourceFile));
    }

    return result;
}

QString compileSource(const QString &source)
{
    {
        QFile tempSourceFile("__tmp__.glsl");
        if (!tempSourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed opening __tmp__.glsl";
            exit(0);
        }

        QTextStream out(&tempSourceFile);
        out << source;
    }

    if (std::system("cgc -quiet -oglsl -profile arbfp1 __tmp__.glsl >__tmp__.frag") == -1) {
        qDebug() << "Failed running cgc";
        exit(0);
    }

    return readSourceFile("__tmp__.frag", true);
}

QString getWord(QString line, int word)
{
    QTextStream in(&line);

    QString result;

    for (int i = 0; i < word; ++i)
        in >> result;

    return result;
}

static int toInt(const QByteArray &str)
{
    int value = 0;

    for (int i = 0; i < str.size(); ++i) {
        if (str[i] < '0' || str[i] > '9')
            break;

        value *= 10;
        value += (str[i] - '0');
    }

    return value;
}
QList<int> getLocations(const QSet<QString> &variables, QString source)
{
    QTextStream in(&source);

    QMap<QString, int> locations;

    foreach (QString variable, variables)
        locations[variable] = -1;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        line = line.right(line.size() - 1);

        if (line.startsWith("#var")) {
            QByteArray temp;
            QByteArray name;

            QTextStream lineStream(&line);

            lineStream >> temp >> temp >> name;

            int location = -1;

            while (!lineStream.atEnd()) {
                lineStream >> temp;

                if (temp.startsWith("c[")) {
                    location = toInt(temp.right(temp.size() - 2));
                    break;
                }

                if (temp == "texunit") {
                    lineStream >> temp;
                    location = toInt(temp);
                    break;
                }
            }

            locations[name] = location;
        }
    }

    QList<int> result;

    foreach (QString variable, variables)
        result << locations[variable];

    return result;
}

// remove #var statements
QString trimmed(QString source)
{
    QTextStream in(&source);

    QString result;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.trimmed().startsWith("\"#"))
            result += line + '\n';
    }

    return result;
}

void writeIncludeFile(const QSet<QString> &variables,
                      const QList<QStringPair> &brushes,
                      const QList<QStringPair> &compositionModes,
                      const QList<QStringPair> &masks,
                      const QMap<QString, QMap<QString, QString> > &compiled)
{
    QFile includeFile("fragmentprograms_p.h");
    if (!includeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed opening fragmentprograms_p.h";
        exit(0);
    }

    QTextStream out(&includeFile);

    QLatin1String tab("    ");

    out << "#ifndef FRAGMENTPROGRAMS_H\n"
        << "#define FRAGMENTPROGRAMS_H\n\n";

    out << "enum FragmentVariable {\n";

    foreach (QString str, variables)
        out << tab << "VAR_" << str.toUpper() << ",\n";

    out << "};\n\n";

    out << "enum FragmentBrushType {\n";

    foreach (QStringPair brush, brushes)
        out << tab << brush.first << ",\n";

    out << "};\n\n";

    out << "enum FragmentCompositionModeType {\n";

    foreach (QStringPair mode, compositionModes)
        out << tab << mode.first << ",\n";

    out << "};\n\n";

    out << "enum FragmentMaskType {\n";

    foreach (QStringPair mask, masks)
        out << tab << mask.first << ",\n";

    out << "};\n\n";

    out << "static const unsigned int num_fragment_variables = " << variables.size() << ";\n\n";
    out << "static const unsigned int num_fragment_brushes = " << brushes.size() << ";\n";
    out << "static const unsigned int num_fragment_composition_modes = " << compositionModes.size() << ";\n";
    out << "static const unsigned int num_fragment_masks = " << masks.size() << ";\n\n";

    foreach (QStringPair mask, masks) {
        const QString compiledSource = compiled[mask.first]["dummy"];

        out << "static const char *FragmentProgram_" << mask.first << " =\n"
            << trimmed(compiledSource)
            << '\n';
    }

    foreach (QStringPair brush, brushes) {
        foreach (QStringPair mode, compositionModes) {
            const QString compiledSource = compiled[brush.first][mode.first];

            out << "static const char *FragmentProgram_" << brush.first << '_' << mode.first << " =\n"
                << trimmed(compiledSource)
                << '\n';
        }
    }

    out << "static const char *mask_fragment_program_sources[num_fragment_masks] = {\n";

    foreach (QStringPair mask, masks)
        out << tab << "FragmentProgram_" << mask.first << ",\n";

    out << "};\n\n";

    out << "static const char *painter_fragment_program_sources[num_fragment_brushes][num_fragment_composition_modes] = {\n";

    foreach (QStringPair brush, brushes) {
        out << tab << "{\n";

        foreach (QStringPair mode, compositionModes)
            out << tab << tab << "FragmentProgram_" << brush.first << '_' << mode.first << ",\n";

        out << tab << "},\n";
    }

    out << "};\n\n";

    out << "static int painter_variable_locations[num_fragment_brushes][num_fragment_composition_modes][num_fragment_variables] = {\n";

    foreach (QStringPair brush, brushes) {
        out << tab << "{\n";

        foreach (QStringPair mode, compositionModes) {
            out << tab << tab << "{ ";

            QList<int> locations = getLocations(variables, compiled[brush.first][mode.first]);

            foreach (int location, locations)
                out << location << ", ";

            out << "},\n";
        }

        out << tab << "},\n";
    }

    out << "};\n\n";

    out << "#endif\n";
}

QList<QString> getVariables(QString program)
{
    QList<QString> result;

    QTextStream in(&program);
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith("uniform")) {
            QString word = getWord(line, 3);
            result << word.left(word.size() - 1);
        }
    }

    return result;
}

int main()
{
    QList<QStringPair> brushes = readConf(QLatin1String("brushes.conf"));
    QList<QStringPair> compositionModes = readConf(QLatin1String("composition_modes.conf"));
    QList<QStringPair> masks = readConf(QLatin1String("masks.conf"));

    QString painterSource = readSourceFile("painter.glsl");
    QString fastPainterSource = readSourceFile("fast_painter.glsl");

    QSet<QString> variables;

    QList<QStringPair> programs[3] = { brushes, compositionModes, masks };

    for (int i = 0; i < 3; ++i)
        foreach (QStringPair value, programs[i])
            variables += QSet<QString>::fromList(getVariables(value.second));

    variables += QSet<QString>::fromList(getVariables(painterSource));
    variables += QSet<QString>::fromList(getVariables(fastPainterSource));

    QMap<QString, QMap<QString, QString> > compiled;

    foreach (QStringPair brush, brushes) {
        foreach (QStringPair mode, compositionModes) {
            QString combinedSource = brush.second + mode.second + painterSource;

            compiled[brush.first][mode.first] = compileSource(combinedSource);
        }

        QString fastSource = brush.second + fastPainterSource;

        compiled[brush.first]["COMPOSITION_MODE_BLEND_MODE"] = compileSource(fastSource);
    }

    compositionModes << QStringPair("COMPOSITION_MODE_BLEND_MODE", "");

    foreach (QStringPair mask, masks)
        compiled[mask.first]["dummy"] = compileSource(mask.second);

    writeIncludeFile(variables, brushes, compositionModes, masks, compiled);

    return 0;
}
