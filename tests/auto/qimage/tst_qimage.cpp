/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qimage.h>
#include <qimagereader.h>
#include <qlist.h>
#include <qmatrix.h>
#include <stdio.h>

//TESTED_CLASS=
//TESTED_FILES=gui/image/qimage.h gui/image/qimage.cpp

class tst_QImage : public QObject
{
    Q_OBJECT

public:
    tst_QImage();

private slots:
    void create();
    void convertBitOrder();
    void formatHadlersInput_data();
    void formatHadlersInput();

    void setAlphaChannel_data();
    void setAlphaChannel();

    void convertToFormat_data();
    void convertToFormat();

    void createAlphaMask_data();
    void createAlphaMask();

    void dotsPerMeterZero();

    void convertToFormatPreserveDotsPrMeter();

    void rotate90();

#if QT_VERSION >= 0x040102
    void copy();
#endif
    void setPixel_data();
    void setPixel();

    void setNumColors();
    void setColor();
};

tst_QImage::tst_QImage()

{
}

// Test if QImage (or any functions called from QImage) throws an
// exception when creating an extremely large image.
// QImage::create() should return "false" in this case.
void tst_QImage::create()
{
    bool cr = true;
#ifndef Q_WS_QWS
    try {
#endif
	QImage image(7000000, 7000000, 8, 256, QImage::IgnoreEndian);
        cr = !image.isNull();
#ifndef Q_WS_QWS
    } catch (...) {
    }
#endif
    QVERIFY( !cr );
}

void tst_QImage::convertBitOrder()
{
    QImage i(9,5,1,2,QImage::LittleEndian);
    i.setDotsPerMeterX(9);
    i.setDotsPerMeterY(5);
    QVERIFY(!i.isNull());

    QImage ni = i.convertBitOrder(QImage::BigEndian);
    QVERIFY(!ni.isNull());
    QVERIFY(ni.bitOrder() == QImage::BigEndian);

    // A bunch of verifies to make sure that nothing was lost
    QVERIFY(i.dotsPerMeterX() == ni.dotsPerMeterX());
    QVERIFY(i.dotsPerMeterY() == ni.dotsPerMeterY());
    QVERIFY(i.depth() == ni.depth());
    QVERIFY(i.size() == ni.size());
    QVERIFY(i.numColors() == ni.numColors());
}

void tst_QImage::formatHadlersInput_data()
{
    QTest::addColumn<QString>("testFormat");
    QTest::addColumn<QString>("testFile");

    // add a new line here when a file is added
    QTest::newRow("ICO") << "ICO" << "images/image.ico";
    QTest::newRow("PNG") << "PNG" << "images/image.png";
    QTest::newRow("GIF") << "GIF" << "images/image.gif";
    QTest::newRow("BMP") << "BMP" << "images/image.bmp";
    QTest::newRow("JPEG") << "JPEG" << "images/image.jpg";
    QTest::newRow("PBM") << "PBM" << "images/image.pbm";
    QTest::newRow("PGM") << "PGM" << "images/image.pgm";
    QTest::newRow("PPM") << "PPM" << "images/image.ppm";
    QTest::newRow("XBM") << "XBM" << "images/image.xbm";
    QTest::newRow("XPM") << "XPM" << "images/image.xpm";
}

void tst_QImage::formatHadlersInput()
{
    QFETCH(QString, testFormat);
    QFETCH(QString, testFile);

    QList<QByteArray> formats = QImageReader::supportedImageFormats();
   // qDebug("Image input formats : %s", formats.join(" | ").latin1());

    bool formatSupported = false;
    for (QList<QByteArray>::Iterator it = formats.begin(); it != formats.end(); ++it) {
        if (*it == testFormat.toLower()) {
	    formatSupported = true;
	    break;
	}
    }
    if (formatSupported) {
//	qDebug(QImage::imageFormat(testFile));
        qDebug("testFormat.toLower == %s", testFormat.toLower().toLatin1().constData());
        qDebug("imageFormat == %s", QImageReader::imageFormat(testFile).constData());
	QCOMPARE(testFormat.toLatin1().toLower(), QImageReader::imageFormat(testFile));
    } else {
	QString msg = "Format not supported : ";
	QSKIP(QString(msg + testFormat).toLatin1(), SkipSingle);
    }
}

void tst_QImage::setAlphaChannel_data()
{
    QTest::addColumn<int>("red");
    QTest::addColumn<int>("green");
    QTest::addColumn<int>("blue");
    QTest::addColumn<int>("alpha");
    QTest::addColumn<bool>("gray");

    QTest::newRow("red at 0%, gray") << 255 << 0 << 0 << 0 << true;
    QTest::newRow("red at 25%, gray") << 255 << 0 << 0 << 63 << true;
    QTest::newRow("red at 50%, gray") << 255 << 0 << 0 << 127 << true;
    QTest::newRow("red at 100%, gray") << 255 << 0 << 0 << 191 << true;
    QTest::newRow("red at 0%, 32bit") << 255 << 0 << 0 << 0 << false;
    QTest::newRow("red at 25%, 32bit") << 255 << 0 << 0 << 63 << false;
    QTest::newRow("red at 50%, 32bit") << 255 << 0 << 0 << 127 << false;
    QTest::newRow("red at 100%, 32bit") << 255 << 0 << 0 << 191 << false;

    QTest::newRow("green at 0%, gray") << 0 << 255 << 0 << 0 << true;
    QTest::newRow("green at 25%, gray") << 0 << 255 << 0 << 63 << true;
    QTest::newRow("green at 50%, gray") << 0 << 255 << 0 << 127 << true;
    QTest::newRow("green at 100%, gray") << 0 << 255 << 0 << 191 << true;
    QTest::newRow("green at 0%, 32bit") << 0 << 255 << 0 << 0 << false;
    QTest::newRow("green at 25%, 32bit") << 0 << 255 << 0 << 63 << false;
    QTest::newRow("green at 50%, 32bit") << 0 << 255 << 0 << 127 << false;
    QTest::newRow("green at 100%, 32bit") << 0 << 255 << 0 << 191 << false;

    QTest::newRow("blue at 0%, gray") << 0 << 0 << 255 << 0 << true;
    QTest::newRow("blue at 25%, gray") << 0 << 0 << 255 << 63 << true;
    QTest::newRow("blue at 50%, gray") << 0 << 0 << 255 << 127 << true;
    QTest::newRow("blue at 100%, gray") << 0 << 0 << 255 << 191 << true;
    QTest::newRow("blue at 0%, 32bit") << 0 << 0 << 255 << 0 << false;
    QTest::newRow("blue at 25%, 32bit") << 0 << 0 << 255 << 63 << false;
    QTest::newRow("blue at 50%, 32bit") << 0 << 0 << 255 << 127 << false;
    QTest::newRow("blue at 100%, 32bit") << 0 << 0 << 255 << 191 << false;
}

void tst_QImage::setAlphaChannel()
{
    QFETCH(int, red);
    QFETCH(int, green);
    QFETCH(int, blue);
    QFETCH(int, alpha);
    QFETCH(bool, gray);

    int width = 100;
    int height = 100;

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(qRgb(red, green, blue));

    // Create the alpha channel
    QImage alphaChannel;
    if (gray) {
        alphaChannel = QImage(width, height, QImage::Format_Indexed8);
        alphaChannel.setNumColors(256);
        for (int i=0; i<256; ++i)
            alphaChannel.setColor(i, qRgb(i, i, i));
        alphaChannel.fill(alpha);
    } else {
        alphaChannel = QImage(width, height, QImage::Format_ARGB32);
        alphaChannel.fill(qRgb(alpha, alpha, alpha));
    }

    image.setAlphaChannel(alphaChannel);
    image = image.convertToFormat(QImage::Format_ARGB32);
    QVERIFY(image.format() == QImage::Format_ARGB32);

    // alpha of 0 becomes black at a=0 due to premultiplication
    QRgb pixel = alpha == 0 ? 0 : qRgba(red, green, blue, alpha);
    bool allPixelsOK = true;
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            allPixelsOK &= image.pixel(x, y) == pixel;
        }
    }
    QVERIFY(allPixelsOK);

    QImage outAlpha = image.alphaChannel();
    QCOMPARE(outAlpha.size(), image.size());

    bool allAlphaOk = true;
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            allAlphaOk &= outAlpha.pixelIndex(x, y) == alpha;
        }
    }
    QVERIFY(allAlphaOk);

}


void tst_QImage::convertToFormat_data()
{
    QTest::addColumn<int>("inFormat");
    QTest::addColumn<uint>("inPixel");
    QTest::addColumn<int>("resFormat");
    QTest::addColumn<uint>("resPixel");

    QTest::newRow("red rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xffff0000
                                      << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xff00ff00
                                        << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xff0000ff
                                       << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("red rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xffff0000
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xff00ff00
                                           << int(QImage::Format_ARGB32_Premultiplied) <<0xff00ff00;
    QTest::newRow("blue rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xff0000ff
                                          << int(QImage::Format_ARGB32_Premultiplied) << 0xff0000ff;

    QTest::newRow("semired argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                       << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u;
    QTest::newRow("semigreen argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u;
    QTest::newRow("semiblue argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                        << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu;
    QTest::newRow("semiwhite argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7fffffffu
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu;
    QTest::newRow("semiblack argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f000000u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u;

    QTest::newRow("semired pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_ARGB32) << 0x7fff0000u;
    QTest::newRow("semigreen pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_ARGB32) << 0x7f00ff00u;
    QTest::newRow("semiblue pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_ARGB32) << 0x7f0000ffu;
    QTest::newRow("semiwhite pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_ARGB32) << 0x7fffffffu;
    QTest::newRow("semiblack pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_ARGB32) << 0x7f000000u;

    QTest::newRow("semired pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_RGB32) << 0xffff0000u;
    QTest::newRow("semigreen pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_RGB32) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_RGB32) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_RGB32) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("semired argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB32) << 0xffff0000u;
    QTest::newRow("semigreen argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB32) << 0xff00ff00u;
    QTest::newRow("semiblue argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB32) << 0xff0000ffu;
    QTest::newRow("semiwhite argb -> rgb32") << int(QImage::Format_ARGB32) << 0x7fffffffu
                                             << int(QImage::Format_RGB32) << 0xffffffffu;
    QTest::newRow("semiblack argb -> rgb32") << int(QImage::Format_ARGB32) << 0x7f000000u
                                             << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("black mono -> rgb32") << int(QImage::Format_Mono) << 0x00000000u
                                         << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("white mono -> rgb32") << int(QImage::Format_Mono) << 0x00000001u
                                         << int(QImage::Format_RGB32) << 0xffffffffu;

#ifdef Q_WS_QWS
    QTest::newRow("red rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xffff0000
                                         << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xff00ff00
                                           << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xff0000ff
                                          << int(QImage::Format_ARGB32) << 0xff0000ff;
    QTest::newRow("red rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xffff0000
                                         << int(QImage::Format_RGB16) << 0xffff0000;
    QTest::newRow("green rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xff00ff00
                                           << int(QImage::Format_RGB16) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xff0000ff
                                          << int(QImage::Format_RGB16) << 0xff0000ff;
    QTest::newRow("semired argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB16) << 0xffff0000;
    QTest::newRow("semigreen argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB16) << 0xff00ff00;
    QTest::newRow("semiblue argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB16) << 0xff0000ff;
    QTest::newRow("semired pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_RGB16) << 0xffff0000u;

    QTest::newRow("semigreen pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_RGB16) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_RGB16) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_RGB16) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_RGB16) << 0xff000000u;
#endif // Q_WS_QWS
}


void tst_QImage::convertToFormat()
{
    QFETCH(int, inFormat);
    QFETCH(uint, inPixel);
    QFETCH(int, resFormat);
    QFETCH(uint, resPixel);

    QImage src(32, 32, QImage::Format(inFormat));

    if (inFormat == QImage::Format_Mono) {
        src.setColor(0, qRgba(0,0,0,0xff));
        src.setColor(1, qRgba(255,255,255,0xff));
    }

    for (int y=0; y<src.height(); ++y)
        for (int x=0; x<src.width(); ++x)
            src.setPixel(x, y, inPixel);

    QImage result = src.convertToFormat(QImage::Format(resFormat));

    QCOMPARE(src.width(), result.width());
    QCOMPARE(src.height(), result.height());

    bool same = true;
    for (int y=0; y<result.height(); ++y) {
        for (int x=0; x<result.width(); ++x) {
            QRgb pixel = result.pixel(x, y);
            same &= (pixel == resPixel);
            if (!same) {
                printf("expect=%08x, result=%08x\n", resPixel, pixel);
                y = 100000;
                break;
            }

        }
    }

    QVERIFY(same);
}

void tst_QImage::createAlphaMask_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("alpha1");
    QTest::addColumn<int>("alpha2");

    int alphas[] = { 0, 127, 255 };

    for (int a1 = 0; a1 < sizeof(alphas) / sizeof(int); ++a1) {
        for (int a2 = 0; a2 < sizeof(alphas) / sizeof(int); ++a2) {
            if (a1 == a2)
                continue;
            for (int x=10; x<18; x+=3) {
                for (int y=100; y<108; y+=3) {
                    QTest::newRow(QString("x=%1, y=%2, a1=%3, a2=%4").arg(x).arg(y).arg(alphas[a1]).arg(alphas[a2]))
                        << x << y << alphas[a1] << alphas[a2];
                }
            }
        }
    }
}

void tst_QImage::createAlphaMask()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, alpha1);
    QFETCH(int, alpha2);

    QSize size(255, 255);
    int pixelsInLines = size.width() + size.height() - 1;
    int pixelsOutofLines = size.width() * size.height() - pixelsInLines;

    // Generate an white image with two lines, horizontal at y and vertical at x.
    // Lines have alpha of alpha2, rest has alpha of alpha1
    QImage image(size, QImage::Format_ARGB32);
    for (int cy=0; cy<image.height(); ++cy) {
        for (int cx=0; cx<image.width(); ++cx) {
            int alpha = (y == cy || x == cx) ? alpha2 : alpha1;
            image.setPixel(cx, cy, qRgba(255, 255, 255, alpha));
        }
    }

    QImage mask = image.createAlphaMask(Qt::OrderedAlphaDither);

    // Sanity check...
    QCOMPARE(mask.width(), image.width());
    QCOMPARE(mask.height(), image.height());

    // Sum up the number of pixels set for both lines and other area
    int sumAlpha1 = 0;
    int sumAlpha2 = 0;
    for (int cy=0; cy<image.height(); ++cy) {
        for (int cx=0; cx<image.width(); ++cx) {
            int *alpha = (y == cy || x == cx) ? &sumAlpha2 : &sumAlpha1;
            *alpha += mask.pixelIndex(cx, cy);
        }
    }

    // Compare the set bits to whats expected for that alpha.
    const int threshold = 5;
    QVERIFY(qAbs(sumAlpha1 * 255 / pixelsOutofLines - alpha1) < threshold);
    QVERIFY(qAbs(sumAlpha2 * 255 / pixelsInLines - alpha2) < threshold);
}

void tst_QImage::dotsPerMeterZero()
{
    QImage img(100, 100, QImage::Format_RGB32);
    QVERIFY(!img.isNull());

    int defaultDpmX = img.dotsPerMeterX();
    int defaultDpmY = img.dotsPerMeterY();
    QVERIFY(defaultDpmX != 0);
    QVERIFY(defaultDpmY != 0);

    img.setDotsPerMeterX(0);
    img.setDotsPerMeterY(0);

    QCOMPARE(img.dotsPerMeterX(), defaultDpmX);
    QCOMPARE(img.dotsPerMeterY(), defaultDpmY);
}

void tst_QImage::rotate90()
{
    // test if rotate90 is lossless
    int w = 54;
    int h = 13;
    QImage original(w,h, QImage::Format_RGB32);
    original.fill(qRgb(255,255,255));

    for (int x = 0; x < w; ++x) {
        original.setPixel(x,0, qRgb(x,0,128));
        original.setPixel(x,h - 1, qRgb(0,255 - x,128));
    }
    for (int y = 0; y < h; ++y) {
        original.setPixel(0, y, qRgb(y,0,255));
        original.setPixel(w - 1, y, qRgb(0,255 - y,255));
    }

    // original.save("rotated90_original.png", "png");

    // Initialize the matrix manually (do not use rotate) to avoid rounding errors
    QMatrix matRotate90 = QMatrix(0, 1, -1, 0, 0, 0);
    QImage dest = original;
    // And rotate it 4 times, then the image should be identical to the original
    for (int i = 0; i < 4 ; ++i) {
        dest = dest.transformed(matRotate90);
    }

    // Make sure they are similar in format before we compare them.
    dest = dest.convertToFormat(QImage::Format_RGB32);

    // dest.save("rotated90_result.png","png");
    QCOMPARE(original, dest);

    // Test with QMatrix::rotate 90 also, since we trust that now
    matRotate90.rotate(90);
    dest = original;
    // And rotate it 4 times, then the image should be identical to the original
    for (int i = 0; i < 4 ; ++i) {
        dest = dest.transformed(matRotate90);
    }

    // Make sure they are similar in format before we compare them.
    dest = dest.convertToFormat(QImage::Format_RGB32);

    QCOMPARE(original, dest);


}

#if QT_VERSION >= 0x040102
void tst_QImage::copy()
{
    // Task 99250
    {
        QImage img(16,16,QImage::Format_ARGB32);
        img.copy(QRect(1000,1,1,1));
    }
}
#endif

void tst_QImage::setPixel_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<uint>("value");
    QTest::addColumn<uint>("expected");

    QTest::newRow("ARGB32 red") << int(QImage::Format_ARGB32)
                                << 0xffff0000 << 0xffff0000;
    QTest::newRow("ARGB32 green") << int(QImage::Format_ARGB32)
                                  << 0xff00ff00 << 0xff00ff00;
    QTest::newRow("ARGB32 blue") << int(QImage::Format_ARGB32)
                                 << 0xff0000ff << 0xff0000ff;
#ifdef Q_WS_QWS
    QTest::newRow("RGB16 red") << int(QImage::Format_RGB16)
                               << 0xffff0000 << 0xf800u;
    QTest::newRow("RGB16 green") << int(QImage::Format_RGB16)
                                 << 0xff00ff00 << 0x07e0u;
    QTest::newRow("RGB16 blue") << int(QImage::Format_RGB16)
                                << 0xff0000ff << 0x001fu;
#endif // Q_WS_QWS
}

void tst_QImage::setPixel()
{
    QFETCH(int, format);
    QFETCH(uint, value);
    QFETCH(uint, expected);

    const int w = 13;
    const int h = 15;

    QImage img(w, h, QImage::Format(format));

    // fill image
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, value);

    // check pixel values
    switch (format) {
    case int(QImage::Format_RGB32):
    case int(QImage::Format_ARGB32):
    case int(QImage::Format_ARGB32_Premultiplied):
    {
        for (int y = 0; y < h; ++y) {
            const quint32 *row = (const quint32*)(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                quint32 result = row[x];
                if (result != expected)
                    printf("[x,y]: %d,%d, expected=%08x, result=%08x\n",
                           x, y, expected, result);
                QCOMPARE(uint(result), expected);
            }
        }
        break;
    }
#ifdef Q_WS_QWS
    case int(QImage::Format_RGB16):
    {
        for (int y = 0; y < h; ++y) {
            const quint16 *row = (const quint16*)(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                quint16 result = row[x];
                if (result != expected)
                    printf("[x,y]: %d,%d, expected=%04x, result=%04x\n",
                           x, y, expected, result);
                QCOMPARE(uint(result), expected);
            }
        }
        break;
    }
#endif
    default:
        qFatal("Test not implemented for format %d", format);
    }
}

void tst_QImage::convertToFormatPreserveDotsPrMeter()
{
    int w = 512, h = 512;
    QImage img(100, 100, QImage::Format_ARGB32_Premultiplied);

    int dpmx = 123;
    int dpmy = 234;
    img.setDotsPerMeterX(dpmx);
    img.setDotsPerMeterY(dpmy);

    img = img.convertToFormat(QImage::Format_RGB32);

    QCOMPARE(img.dotsPerMeterX(), dpmx);
    QCOMPARE(img.dotsPerMeterY(), dpmy);
}

void tst_QImage::setNumColors()
{
    QImage img(0, 0, QImage::Format_Indexed8);
    QTest::ignoreMessage(QtWarningMsg, "QImage::setNumColors: null image");
    img.setNumColors(256);
    QCOMPARE(img.numColors(), 0);
}

void tst_QImage::setColor()
{
    QImage img(0, 0, QImage::Format_Indexed8);
    QTest::ignoreMessage(QtWarningMsg, "QImage::setColor: Index out of bound 0");
    img.setColor(0, qRgba(18, 219, 108, 128));
}

QTEST_MAIN(tst_QImage)
#include "tst_qimage.moc"
