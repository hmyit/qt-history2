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

#include "qpaintdevice.h"
#include "qpainter.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qwsdisplay_qws.h"

QT_BEGIN_NAMESPACE

QPaintDevice::QPaintDevice()
{
    painters = 0;
}

extern void qt_painter_removePaintDevice(QPaintDevice *); //qpainter.cpp


QPaintDevice::~QPaintDevice()
{
    if (paintingActive())
        qWarning("QPaintDevice: Cannot destroy paint device that is being "
                  "painted");
    qt_painter_removePaintDevice(this);
}


int QPaintDevice::metric(PaintDeviceMetric m) const
{
    qWarning("QPaintDevice::metrics: Device has no metric information");
    if (m == PdmDpiX) {
        return 72;
    } else if (m == PdmDpiY) {
        return 72;
    } else if (m == PdmNumColors) {
        // FIXME: does this need to be a real value?
        return 256;
    } else {
        qDebug("Unrecognised metric %d!",m);
        return 0;
    }
}

/*!
    \internal
*/
QWSDisplay *QPaintDevice::qwsDisplay()
{
    return qt_fbdpy;
}

QT_END_NAMESPACE
