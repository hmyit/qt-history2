/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>


#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QSet>

typedef QMap<QString, QString> QStringMap;
typedef QList<int> QIntList;
Q_DECLARE_METATYPE(QImage)
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(QIntList)
Q_DECLARE_METATYPE(QImageWriter::ImageWriterError)
//Normally useless, but we'll see...
Q_DECLARE_METATYPE(QIODevice *)

//TESTED_FILES=gui/image/qimagewriter.h gui/image/qimagewriter.cpp

class tst_QImageWriter : public QObject
{
    Q_OBJECT

public:
    tst_QImageWriter();
    virtual ~tst_QImageWriter();

public slots:
    void init();
    void cleanup();

private slots:
    void getSetCheck();
    void writeImage_data();
    void writeImage();
    void writeImage2_data();
    void writeImage2();
    void supportedFormats();

    void setDescription_data();
    void setDescription();

    void writeToInvalidDevice();

    void supportsOption_data();
    void supportsOption();

    void saveWithNoFormat_data();
    void saveWithNoFormat();

    void saveToTemporaryFile();
};

static const QLatin1String prefix(SRCDIR "/images/");

static void initializePadding(QImage *image)
{
    int effectiveBytesPerLine = (image->width() * image->depth() + 7) / 8;
    int paddingBytes = image->bytesPerLine() - effectiveBytesPerLine;
    if (paddingBytes == 0)
        return;
    for (int y = 0; y < image->height(); ++y) {
        qMemSet(image->scanLine(y) + effectiveBytesPerLine, 0, paddingBytes);
    }
}

// Testing get/set functions
void tst_QImageWriter::getSetCheck()
{
    QImageWriter obj1;
    // QIODevice * QImageWriter::device()
    // void QImageWriter::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);

    QCOMPARE((QIODevice *) var1, obj1.device());
    // The class should possibly handle a 0-pointer as a device, since
    // there is a default contructor, so it's "handling" a 0 device by default.
    // For example: QMovie::setDevice(0) works just fine
    obj1.setDevice((QIODevice *)0);
    QCOMPARE((QIODevice *) 0, obj1.device());
    delete var1;

    // int QImageWriter::quality()
    // void QImageWriter::setQuality(int)
    obj1.setQuality(0);
    QCOMPARE(0, obj1.quality());
    obj1.setQuality(INT_MIN);
    QCOMPARE(INT_MIN, obj1.quality());
    obj1.setQuality(INT_MAX);
    QCOMPARE(INT_MAX, obj1.quality());

    // int QImageWriter::compression()
    // void QImageWriter::setCompression(int)
    obj1.setCompression(0);
    QCOMPARE(0, obj1.compression());
    obj1.setCompression(INT_MIN);
    QCOMPARE(INT_MIN, obj1.compression());
    obj1.setCompression(INT_MAX);
    QCOMPARE(INT_MAX, obj1.compression());

    // float QImageWriter::gamma()
    // void QImageWriter::setGamma(float)
    obj1.setGamma(0.0f);
    QCOMPARE(0.0f, obj1.gamma());
    obj1.setGamma(1.1f);
    QCOMPARE(1.1f, obj1.gamma());
}

tst_QImageWriter::tst_QImageWriter()
{
}

tst_QImageWriter::~tst_QImageWriter()
{

}

void tst_QImageWriter::init()
{
}

void tst_QImageWriter::cleanup()
{
}

void tst_QImageWriter::writeImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("lossy");
    QTest::addColumn<QByteArray>("format");

    QTest::newRow("BMP: colorful") << QString("colorful.bmp") << false << QByteArray("bmp");
    QTest::newRow("BMP: font") << QString("font.bmp") << false << QByteArray("bmp");
    QTest::newRow("XPM: marble") << QString("marble.xpm") << false << QByteArray("xpm");
    QTest::newRow("PNG: kollada") << QString("kollada.png") << false << QByteArray("png");
    QTest::newRow("PPM: teapot") << QString("teapot.ppm") << false << QByteArray("ppm");
    QTest::newRow("PBM: ship63") << QString("ship63.pbm") << true << QByteArray("pbm");
    QTest::newRow("XBM: gnus") << QString("gnus.xbm") << false << QByteArray("xbm");
    QTest::newRow("JPEG: beavis") << QString("beavis.jpg") << true << QByteArray("jpeg");
#if defined QTEST_HAVE_TIFF
    QTest::newRow("TIFF: teapot") << QString("teapot.tiff") << false << QByteArray("tiff");
#endif
}

void tst_QImageWriter::writeImage()
{
    QFETCH(QString, fileName);
    QFETCH(bool, lossy);
    QFETCH(QByteArray, format);

    QImage image;
    {
        QImageReader reader(prefix + fileName);
        image = reader.read();
        QVERIFY2(!image.isNull(), qPrintable(reader.errorString()));
    }
    {
        QImageWriter writer(prefix + "gen-" + fileName, format);
        QVERIFY(writer.write(image));
    }

    {
        // Shouldn't be able to write to read-only file
        QImageWriter writer(prefix + fileName, format);
        QVERIFY(!writer.write(image));
    }

    QImage image2;
    {
        QImageReader reader(prefix + "gen-" + fileName);
        image2 = reader.read();
        QVERIFY(!image2.isNull());
    }
    if (!lossy) {
        QCOMPARE(image, image2);
    } else {
        QCOMPARE(image.format(), image2.format());
        QCOMPARE(image.depth(), image2.depth());
    }
}

void tst_QImageWriter::writeImage2_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QImage>("image");

    const QStringList formats = QStringList() << "bmp" << "xpm" << "png"
                                              << "ppm"; //<< "jpeg";
    QImage image0(70, 70, QImage::Format_ARGB32);
    image0.fill(QColor(Qt::red).rgb());

    QImage::Format imgFormat = QImage::Format_Mono;
    while (imgFormat != QImage::NImageFormats) {
        QImage image = image0.convertToFormat(imgFormat);
        initializePadding(&image);
        foreach (const QString format, formats) {
            const QString fileName = QString("solidcolor_%1.%2").arg(imgFormat)
                                     .arg(format);
            QTest::newRow(fileName.toLatin1()) << fileName
                                               << format.toLatin1()
                                               << image;
        }
        imgFormat = QImage::Format(int(imgFormat) + 1);
    }
}

/*
    Workaround for the equality operator for indexed formats
    (which fails if the colortables are different).

    Images must have the same format and size.
*/
static bool equalImageContents(const QImage &image1, const QImage &image2)
{
    switch (image1.format()) {
    case QImage::Format_Mono:
    case QImage::Format_Indexed8:
        for (int y = 0; y < image1.height(); ++y)
            for (int x = 0; x < image1.width(); ++x)
                if (image1.pixel(x, y) != image2.pixel(x, y))
                    return false;
        return true;
    default:
        return (image1 == image2);
    }
}

void tst_QImageWriter::writeImage2()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);
    QFETCH(QImage, image);

    //we reduce the scope of writer so that it closes the associated file
    // and QFile::remove can actually work
    {
        QImageWriter writer(fileName, format);
        QVERIFY(writer.write(image));
    }

    QImage written;

    //we reduce the scope of reader so that it closes the associated file
    // and QFile::remove can actually work
    {
        QImageReader reader(fileName, format);
        QVERIFY(reader.read(&written));
    }

    written = written.convertToFormat(image.format());
    if (!equalImageContents(written, image)) {
        qDebug() << "image" << image.format() << image.width()
                 << image.height() << image.depth()
                 << hex << image.pixel(0, 0);
        qDebug() << "written" << written.format() << written.width()
                 << written.height() << written.depth()
                 << hex << written.pixel(0, 0);
    }
    QVERIFY(equalImageContents(written, image));

    QVERIFY(QFile::remove(fileName));
}

void tst_QImageWriter::supportedFormats()
{
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QList<QByteArray> sortedFormats = formats;
    qSort(sortedFormats);

    // check that the list is sorted
    QCOMPARE(formats, sortedFormats);

    QSet<QByteArray> formatSet;
    foreach (QByteArray format, formats)
        formatSet << format;

    // check that the list does not contain duplicates
    QCOMPARE(formatSet.size(), formats.size());
}

void tst_QImageWriter::setDescription_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QStringMap>("description");

    QMap<QString, QString> willem;
    willem["Title"] = "PngSuite";
    willem["Author"] = "Willem A.J. van Schaik (willem@schaik.com)";
    willem["Copyright"] = "Copyright Willem van Schaik, Singapore 1995-96";
    willem["Description"] = "A compilation of a set of images created to test the "
                            "various color-types of the PNG format. Included are "
                            "black&white, color, paletted, with alpha channel, with "
                            "transparency formats. All bit-depths allowed according "
                            "to the spec are present.";
    willem["Software"] = "Created on a NeXTstation color using \"pnmtopng\".";
    willem["Disclaimer"] = "Freeware.";

    QTest::newRow("PNG") << prefix + QString("gen-pngwithtext.png") << willem;
}

void tst_QImageWriter::setDescription()
{
    QFETCH(QString, fileName);
    QFETCH(QStringMap, description);

    QImageWriter writer(fileName, "png");
    foreach (QString key, description.keys())
        writer.setText(key, description.value(key));
    QVERIFY(writer.write(QImage(prefix + "kollada.png")));

    QImageReader reader(fileName);
    foreach (QString key, description.keys())
        QCOMPARE(reader.text(key), description.value(key));
}

void tst_QImageWriter::writeToInvalidDevice()
{
    QLatin1String fileName("/these/directories/do/not/exist/001.png");
    {
        QImageWriter writer(fileName);
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        writer.setFormat("png");
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        QImage im(10, 10, QImage::Format_ARGB32);
        QVERIFY(!writer.write(im));
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        writer.setFormat("png");
        QImage im(10, 10, QImage::Format_ARGB32);
        QVERIFY(!writer.write(im));
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
}

void tst_QImageWriter::supportsOption_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QIntList>("options");

    QTest::newRow("png") << QString(prefix + "gen-black.png")
                         << (QIntList() << QImageIOHandler::Gamma
                              << QImageIOHandler::Description
                              << QImageIOHandler::Quality
                              << QImageIOHandler::Size);
#if defined QTEST_HAVE_TIFF
    QTest::newRow("tiff") << QString("images/gen-black.tiff")
                          << (QIntList() << QImageIOHandler::Size
                              << QImageIOHandler::CompressionRatio);
#endif
}

void tst_QImageWriter::supportsOption()
{
    QFETCH(QString, fileName);
    QFETCH(QIntList, options);

    QSet<QImageIOHandler::ImageOption> allOptions;
    allOptions << QImageIOHandler::Size
               << QImageIOHandler::ClipRect
               << QImageIOHandler::Description
               << QImageIOHandler::ScaledClipRect
               << QImageIOHandler::ScaledSize
               << QImageIOHandler::CompressionRatio
               << QImageIOHandler::Gamma
               << QImageIOHandler::Quality
               << QImageIOHandler::Name
               << QImageIOHandler::SubType
               << QImageIOHandler::IncrementalReading
               << QImageIOHandler::Endianness
               << QImageIOHandler::Animation
               << QImageIOHandler::BackgroundColor;

    QImageWriter writer(fileName);
    for (int i = 0; i < options.size(); ++i) {
        QVERIFY(writer.supportsOption(QImageIOHandler::ImageOption(options.at(i))));
        allOptions.remove(QImageIOHandler::ImageOption(options.at(i)));
    }

    foreach (QImageIOHandler::ImageOption option, allOptions)
        QVERIFY(!writer.supportsOption(option));
}

void tst_QImageWriter::saveWithNoFormat_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QImageWriter::ImageWriterError>("error");

    QTest::newRow("garble") << prefix + QString("gen-out.garble") << QByteArray("jpeg") << QImageWriter::UnsupportedFormatError;
    QTest::newRow("bmp") << prefix + QString("gen-out.bmp") << QByteArray("bmp") << QImageWriter::ImageWriterError(0);
    QTest::newRow("xbm") << prefix + QString("gen-out.xbm") << QByteArray("xbm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("xpm") << prefix + QString("gen-out.xpm") << QByteArray("xpm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("png") << prefix + QString("gen-out.png") << QByteArray("png") << QImageWriter::ImageWriterError(0);
    QTest::newRow("ppm") << prefix + QString("gen-out.ppm") << QByteArray("ppm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("pbm") << prefix + QString("gen-out.pbm") << QByteArray("pbm") << QImageWriter::ImageWriterError(0);
#if defined QTEST_HAVE_TIFF
    QTest::newRow("tiff") << prefix + QString("gen-out.tiff") << QByteArray("tiff") << QImageWriter::ImageWriterError(0);
#endif
}

void tst_QImageWriter::saveWithNoFormat()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);
    QFETCH(QImageWriter::ImageWriterError, error);

    QImage niceImage(64, 64, QImage::Format_ARGB32);
    qMemSet(niceImage.bits(), 0, niceImage.numBytes());

    QImageWriter writer(fileName /* , 0 - no format! */);
    if (error != 0) {
        QVERIFY(!writer.write(niceImage));
        QCOMPARE(writer.error(), error);
        return;
    }

    QVERIFY2(writer.write(niceImage), qPrintable(writer.errorString()));

    QImageReader reader(fileName);
    QCOMPARE(reader.format(), format);

    QVERIFY(reader.canRead());

    QImage outImage = reader.read();
    QVERIFY2(!outImage.isNull(), qPrintable(reader.errorString()));
}

void tst_QImageWriter::saveToTemporaryFile()
{
    QImage image(prefix + "kollada.png");
    QVERIFY(!image.isNull());

    {
        // 1) Via QImageWriter's API, with a standard temp file name
        QTemporaryFile file;
        QVERIFY(file.open());
        QImageWriter writer(&file, "PNG");
        if (writer.canWrite())
            QVERIFY(writer.write(image));
        else
            qWarning() << file.errorString();
        QCOMPARE(QImage(writer.fileName()), image);
    }
    {
        // 2) Via QImage's API, with a standard temp file name
        QTemporaryFile file;
        QVERIFY(file.open());
        QVERIFY(image.save(&file, "PNG"));
        file.reset();
        QImage tmp;
        QVERIFY(tmp.load(&file, "PNG"));
        QCOMPARE(tmp, image);
    }
    {
        // 3) Via QImageWriter's API, with a named temp file
        QTemporaryFile file("tempXXXXXX");
        QVERIFY(file.open());
        QImageWriter writer(&file, "PNG");
        QVERIFY(writer.write(image));
        QCOMPARE(QImage(writer.fileName()), image);
    }
    {
        // 4) Via QImage's API, with a named temp file
        QTemporaryFile file("tempXXXXXX");
        QVERIFY(file.open());
        QVERIFY(image.save(&file, "PNG"));
        file.reset();
        QImage tmp;
        QVERIFY(tmp.load(&file, "PNG"));
        QCOMPARE(tmp, image);
    }
}

QTEST_MAIN(tst_QImageWriter)
#include "tst_qimagewriter.moc"
