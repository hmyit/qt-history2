#include "fileporter.h"
#include <iostream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include "lexer.h"
#include "replacetoken.h"
#include "logger.h"

using std::cout;
using std::endl;

QByteArray FilePorter::noPreprocess(const QString &filePath)
{
    QFile f(filePath);
    f.open(QIODevice::ReadOnly);
    return f.readAll();
}

FilePorter::FilePorter(QString rulesfilename)
:rulesFromXml(rulesfilename)
,tokenReplacementRules(rulesFromXml.getNoPreprocessPortingTokenRules())
,replaceToken(tokenReplacementRules)
{
    int dummy=0;
    foreach(QString headerName, rulesFromXml.getHeaderList(RulesFromXml::Qt4)) {
        qt4HeaderNames.insert(headerName, dummy);
    }
}

void FilePorter::port(QString inBasePath, QString inFilePath, QString outBasePath, QString outFilePath, FileType fileType )
{
    QString fullInFileName = inBasePath + inFilePath;
    QFileInfo infileInfo(fullInFileName);
    if(!infileInfo.exists()) {
        cout<<"Could not open file: " << fullInFileName.latin1() <<endl;
        return;
    }
   
    Lexer lexer;
    FileSymbol *sym = new FileSymbol();
    sym->contents = noPreprocess(fullInFileName);
    sym->tokenStream = lexer.tokenize(sym);

    Logger::instance()->setFileState(inFilePath);
/*
    TextReplacements textReplacements;
    textReplacements += getTokenTextReplacements(sym, tokenReplacementRules);
    QByteArray portedContents = textReplacements.apply(sym->contents);
*/
    QByteArray portedContents = replaceToken.getTokenTextReplacements(sym).apply(sym->contents);
    
    //This step needs to be done after the token replacements, since
    //we need to know which new class names that has been inserted in the source
    portedContents = includeAnalyse(portedContents, fileType);

    if(!outFilePath.isEmpty()) {
        QString fullOutfileName = outBasePath + outFilePath;
        FileWriter::instance()->writeFileVerbously(fullOutfileName, portedContents);
    }
    delete sym;
}

QByteArray FilePorter::includeAnalyse(QByteArray fileContents, FileType /*fileType*/)
{
    //Get list of used classes and included headers
    Lexer lexer;
    FileSymbol *sym = new FileSymbol();
    sym->contents = fileContents;
    TokenStream *inStream = lexer.tokenize(sym);

    QMap<QByteArray, int> classes;
    QMap<QByteArray, int> headers;

    inStream->rewind(0);
    while(!inStream->tokenAtEnd())
    {
        QByteArray tokenText=inStream->currentTokenText();
        if(tokenText[0] =='Q' && qt4HeaderNames.contains(tokenText))
            classes.insert(tokenText, 0);
        else if(tokenText.startsWith("#include")) {
            QString tokenString(tokenText);
            QStringList subTokenList = tokenString.split(QRegExp("<|>|\""), QString::SkipEmptyParts);
            foreach(QString subToken, subTokenList) {
                if(subToken[0] == 'Q' || subToken[0] == 'q')
                    headers.insert(subToken.latin1(), 0);
            }
        }
        inStream->nextToken();
    }
       
    //compare class and header names, find classes that lacks a
    //curresponding include directive
    QStringList neededHeaders = rulesFromXml.getNeededHeaderList();
    QStringList headersToInsert;
    QMapIterator<QByteArray, int> c(classes);
    //loop through classes
    while (c.hasNext()) {
        c.next();
        bool foundHeader=false;
        QMapIterator<QByteArray, int> h(headers);
        //loop through headers
        while (h.hasNext() && !foundHeader) {
            h.next();
            if(h.key().toLower().startsWith(c.key().toLower()))
                foundHeader=true; //we found a header for class c
        }
        if(!foundHeader){
            //we have a class without a coresponding header.
            bool needHeader=false;
            //loop through list of headers
            foreach(QString header, neededHeaders) {
                //compare class name with header
                //TODO: assumes that only new-style headers are specified in the 
                //rules. This is ok for now.
                if(header.latin1() == c.key()) {
                    needHeader=true;
                    break;
                }
            }
            if(needHeader) {
                headersToInsert.push_back(QString("#include <" + c.key() + ">"));
             //   printf("Must insert header file for class  %s \n ", c.key().constData());
            }
        }
    }
    
    //insert headers in files, at the end of the first block of
    //include files
    //TODO: make this more intelligent by not inserting inside #ifdefs
    inStream->rewind(0);
    bool includeEnd = false;
    bool includeStart = false;
    while(!inStream->tokenAtEnd() && !includeEnd)
    {
        QByteArray tokenText=inStream->currentTokenText();
        if(tokenText.trimmed().startsWith("#include"))
            includeStart=true;
        else if(includeStart &&(!tokenText.trimmed().isEmpty() || tokenText == "\n" ))
            includeEnd=true;
        inStream->nextToken();
    }
    /*
    //back up a bit to get just at the end of the last include
    while(inStream->currentTokenText().trimmed())  ) {
        inStream->rewind(inStream->cursor()-1);
    }
    */
    int insertPos=0;
    if(includeStart != false) 
        insertPos = inStream->token().position;
    
    int insertCount = headersToInsert.count();
    if(insertCount>0) {
        QByteArray insertText;
        QByteArray logText;
        
        insertText+="\n//Added by the Qt porting tool:\n";
        logText += "Added the following include directives: ";
        
        foreach(QString headerName, headersToInsert) {
            insertText = insertText + headerName.latin1() + "\n";
            logText = logText + headerName.latin1() + " ";
        }
        insertText+="\n";
        fileContents.insert(insertPos, insertText);        
        Logger::instance()->addEntry("AddHeader", logText, QString(), 0, 0); //TODO get line/column here
    }
    
    return fileContents;
}



