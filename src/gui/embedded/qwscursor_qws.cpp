/****************************************************************************
**
** Implementation of QCursor for Qt/Embedded.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qcursor.h"
#include "qbitmap.h"
#include "qgfx_qws.h"
#include "qapplication.h"
#include "qwindowsystem_qws.h"
#include "qwscursor_qws.h"

#ifndef QT_NO_QWS_CURSOR
static QWSCursor *systemCursorTable[Qt::LastCursor+1];
static bool systemCursorTableInit = false;

// 16 x 16
static const uchar cur_arrow_bits[] = {
   0x07, 0x00, 0x39, 0x00, 0xc1, 0x01, 0x02, 0x0e, 0x02, 0x10, 0x02, 0x08,
   0x04, 0x04, 0x04, 0x02, 0x04, 0x04, 0x88, 0x08, 0x48, 0x11, 0x28, 0x22,
   0x10, 0x44, 0x00, 0x28, 0x00, 0x10, 0x00, 0x00 };
static const uchar mcur_arrow_bits[] = {
   0x07, 0x00, 0x3f, 0x00, 0xff, 0x01, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x0f,
   0xfc, 0x07, 0xfc, 0x03, 0xfc, 0x07, 0xf8, 0x0f, 0x78, 0x1f, 0x38, 0x3e,
   0x10, 0x7c, 0x00, 0x38, 0x00, 0x10, 0x00, 0x00 };

static const unsigned char cur_up_arrow_bits[] = {
   0x80, 0x00, 0x40, 0x01, 0x40, 0x01, 0x20, 0x02, 0x20, 0x02, 0x10, 0x04,
   0x10, 0x04, 0x08, 0x08, 0x78, 0x0f, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01,
   0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0xc0, 0x01};
static const unsigned char mcur_up_arrow_bits[] = {
   0x80, 0x00, 0xc0, 0x01, 0xc0, 0x01, 0xe0, 0x03, 0xe0, 0x03, 0xf0, 0x07,
   0xf0, 0x07, 0xf8, 0x0f, 0xf8, 0x0f, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01};

static const unsigned char cur_cross_bits[] = {
   0xc0, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01,
   0x7f, 0x7f, 0x01, 0x40, 0x7f, 0x7f, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01,
   0x40, 0x01, 0x40, 0x01, 0xc0, 0x01, 0x00, 0x00};
static const unsigned char mcur_cross_bits[] = {
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00};

static const uchar cur_ibeam_bits[] = {
   0x00, 0x00, 0xe0, 0x03, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00 };
static const uchar mcur_ibeam_bits[] = {
   0xf0, 0x07, 0xf0, 0x07, 0xf0, 0x07, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xf0, 0x07, 0xf0, 0x07, 0xf0, 0x07, 0x00, 0x00 };

static const uchar cur_ver_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0xf0, 0x0f,
    0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xf0, 0x0f,
    0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00 };
static const uchar mcur_ver_bits[] = {
    0x00, 0x00, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f,
    0xfc, 0x7f, 0xc0, 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xfc, 0x7f, 0xf8, 0x3f,
    0xf0, 0x1f, 0xe0, 0x0f, 0xc0, 0x07, 0x80, 0x03 };

static const uchar cur_hor_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0x30, 0x18,
    0x38, 0x38, 0xfc, 0x7f, 0xfc, 0x7f, 0x38, 0x38, 0x30, 0x18, 0x20, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar mcur_hor_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x60, 0x0c, 0x70, 0x1c, 0x78, 0x3c,
    0xfc, 0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0x7f, 0x78, 0x3c,
    0x70, 0x1c, 0x60, 0x0c, 0x40, 0x04, 0x00, 0x00 };
static const uchar cur_bdiag_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3c, 0x00, 0x3e,
    0x00, 0x37, 0x88, 0x23, 0xd8, 0x01, 0xf8, 0x00, 0x78, 0x00, 0xf8, 0x00,
    0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar mcur_bdiag_bits[] = {
    0x00, 0x00, 0xc0, 0x7f, 0x80, 0x7f, 0x00, 0x7f, 0x00, 0x7e, 0x04, 0x7f,
    0x8c, 0x7f, 0xdc, 0x77, 0xfc, 0x63, 0xfc, 0x41, 0xfc, 0x00, 0xfc, 0x01,
    0xfc, 0x03, 0xfc, 0x07, 0x00, 0x00, 0x00, 0x00 };
static const uchar cur_fdiag_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0xf8, 0x00, 0x78, 0x00,
    0xf8, 0x00, 0xd8, 0x01, 0x88, 0x23, 0x00, 0x37, 0x00, 0x3e, 0x00, 0x3c,
    0x00, 0x3e, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00 };
static const uchar mcur_fdiag_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0xfc, 0x07, 0xfc, 0x03, 0xfc, 0x01, 0xfc, 0x00,
    0xfc, 0x41, 0xfc, 0x63, 0xdc, 0x77, 0x8c, 0x7f, 0x04, 0x7f, 0x00, 0x7e,
    0x00, 0x7f, 0x80, 0x7f, 0xc0, 0x7f, 0x00, 0x00 };
static const uchar cur_blank_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// 20 x 20
static const uchar forbidden_bits[] = {
    0x00,0x00,0x00,0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xf0,0x00,0x38,0xc0,0x01,
    0x7c,0x80,0x03,0xec,0x00,0x03,0xce,0x01,0x07,0x86,0x03,0x06,0x06,0x07,0x06,
    0x06,0x0e,0x06,0x06,0x1c,0x06,0x0e,0x38,0x07,0x0c,0x70,0x03,0x1c,0xe0,0x03,
    0x38,0xc0,0x01,0xf0,0xe0,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00,0x00,0x00,0x00 };

static const uchar forbiddenm_bits[] = {
    0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xff,0x00,0xf8,0xff,0x01,0xfc,0xf0,0x03,
    0xfe,0xc0,0x07,0xfe,0x81,0x07,0xff,0x83,0x0f,0xcf,0x07,0x0f,0x8f,0x0f,0x0f,
    0x0f,0x1f,0x0f,0x0f,0x3e,0x0f,0x1f,0xfc,0x0f,0x1e,0xf8,0x07,0x3e,0xf0,0x07,
    0xfc,0xe0,0x03,0xf8,0xff,0x01,0xf0,0xff,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00};

// 32 x 32
static const uchar wait_data_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0x04, 0x40, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0x08, 0x20, 0x00,
   0x00, 0x08, 0x20, 0x00, 0x00, 0x08, 0x20, 0x00, 0x00, 0x08, 0x20, 0x00,
   0x00, 0x50, 0x15, 0x00, 0x00, 0xa0, 0x0a, 0x00, 0x00, 0x40, 0x05, 0x00,
   0x00, 0x80, 0x02, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x20, 0x08, 0x00,
   0x00, 0x10, 0x10, 0x00, 0x00, 0x08, 0x21, 0x00, 0x00, 0x88, 0x22, 0x00,
   0x00, 0x48, 0x25, 0x00, 0x00, 0xa8, 0x2a, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0x04, 0x40, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar wait_mask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf0, 0x1f, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00, 0xc0, 0x07, 0x00,
   0x00, 0x80, 0x03, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xe0, 0x0f, 0x00,
   0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const uchar hsplit_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
    0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar hsplitm_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
    0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
    0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
    0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar vsplit_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
    0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar vsplitm_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
    0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
    0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
    0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
    0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
    0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
    0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar phand_bits[] = {
    0x00, 0x00, 0x00, 0x00,	0xfe, 0x01, 0x00, 0x00,	0x01, 0x02, 0x00, 0x00,
    0x7e, 0x04, 0x00, 0x00,	0x08, 0x08, 0x00, 0x00,	0x70, 0x08, 0x00, 0x00,
    0x08, 0x08, 0x00, 0x00,	0x70, 0x14, 0x00, 0x00,	0x08, 0x22, 0x00, 0x00,
    0x30, 0x41, 0x00, 0x00,	0xc0, 0x20, 0x00, 0x00,	0x40, 0x12, 0x00, 0x00,
    0x80, 0x08, 0x00, 0x00,	0x00, 0x05, 0x00, 0x00,	0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00 };
static const uchar phandm_bits[] = {
    0xfe, 0x01, 0x00, 0x00,	0xff, 0x03, 0x00, 0x00,	0xff, 0x07, 0x00, 0x00,
    0xff, 0x0f, 0x00, 0x00,	0xfe, 0x1f, 0x00, 0x00,	0xf8, 0x1f, 0x00, 0x00,
    0xfc, 0x1f, 0x00, 0x00,	0xf8, 0x3f, 0x00, 0x00,	0xfc, 0x7f, 0x00, 0x00,
    0xf8, 0xff, 0x00, 0x00,	0xf0, 0x7f, 0x00, 0x00,	0xe0, 0x3f, 0x00, 0x00,
    0xc0, 0x1f, 0x00, 0x00,	0x80, 0x0f, 0x00, 0x00,	0x00, 0x07, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00 };

static const uchar size_all_data_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0x81, 0x40, 0x00, 0x80, 0x81, 0xc0, 0x00,
   0xc0, 0xff, 0xff, 0x01, 0x80, 0x81, 0xc0, 0x00, 0x00, 0x81, 0x40, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uchar size_all_mask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc2, 0x21, 0x00,
   0x00, 0xc3, 0x61, 0x00, 0x80, 0xc3, 0xe1, 0x00, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01, 0x80, 0xc3, 0xe1, 0x00,
   0x00, 0xc3, 0x61, 0x00, 0x00, 0xc2, 0x21, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const uchar whatsthis_bits[] = {
   0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0xf0, 0x07, 0x00,
   0x09, 0x18, 0x0e, 0x00, 0x11, 0x1c, 0x0e, 0x00, 0x21, 0x1c, 0x0e, 0x00,
   0x41, 0x1c, 0x0e, 0x00, 0x81, 0x1c, 0x0e, 0x00, 0x01, 0x01, 0x07, 0x00,
   0x01, 0x82, 0x03, 0x00, 0xc1, 0xc7, 0x01, 0x00, 0x49, 0xc0, 0x01, 0x00,
   0x95, 0xc0, 0x01, 0x00, 0x93, 0xc0, 0x01, 0x00, 0x21, 0x01, 0x00, 0x00,
   0x20, 0xc1, 0x01, 0x00, 0x40, 0xc2, 0x01, 0x00, 0x40, 0x02, 0x00, 0x00,
   0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
static const uchar whatsthism_bits[] = {
   0x01, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x07, 0x00, 0x07, 0xf8, 0x0f, 0x00,
   0x0f, 0xfc, 0x1f, 0x00, 0x1f, 0x3e, 0x1f, 0x00, 0x3f, 0x3e, 0x1f, 0x00,
   0x7f, 0x3e, 0x1f, 0x00, 0xff, 0x3e, 0x1f, 0x00, 0xff, 0x9d, 0x0f, 0x00,
   0xff, 0xc3, 0x07, 0x00, 0xff, 0xe7, 0x03, 0x00, 0x7f, 0xe0, 0x03, 0x00,
   0xf7, 0xe0, 0x03, 0x00, 0xf3, 0xe0, 0x03, 0x00, 0xe1, 0xe1, 0x03, 0x00,
   0xe0, 0xe1, 0x03, 0x00, 0xc0, 0xe3, 0x03, 0x00, 0xc0, 0xe3, 0x03, 0x00,
   0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

static const uchar busy_bits[] = {
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
    0x41, 0xe0, 0xff, 0x00, 0x81, 0x20, 0x80, 0x00, 0x01, 0xe1, 0xff, 0x00,
    0x01, 0x42, 0x40, 0x00, 0xc1, 0x47, 0x40, 0x00, 0x49, 0x40, 0x55, 0x00,
    0x95, 0x80, 0x2a, 0x00, 0x93, 0x00, 0x15, 0x00, 0x21, 0x01, 0x0a, 0x00,
    0x20, 0x01, 0x11, 0x00, 0x40, 0x82, 0x20, 0x00, 0x40, 0x42, 0x44, 0x00,
    0x80, 0x41, 0x4a, 0x00, 0x00, 0x40, 0x55, 0x00, 0x00, 0xe0, 0xff, 0x00,
    0x00, 0x20, 0x80, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uchar busym_bits[] = {
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
    0x7f, 0xe0, 0xff, 0x00, 0xff, 0xe0, 0xff, 0x00, 0xff, 0xe1, 0xff, 0x00,
    0xff, 0xc3, 0x7f, 0x00, 0xff, 0xc7, 0x7f, 0x00, 0x7f, 0xc0, 0x7f, 0x00,
    0xf7, 0x80, 0x3f, 0x00, 0xf3, 0x00, 0x1f, 0x00, 0xe1, 0x01, 0x0e, 0x00,
    0xe0, 0x01, 0x1f, 0x00, 0xc0, 0x83, 0x3f, 0x00, 0xc0, 0xc3, 0x7f, 0x00,
    0x80, 0xc1, 0x7f, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0xe0, 0xff, 0x00,
    0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

void QWSServer::initializeCursor()
{
    // setup system cursors
#ifndef QT_NO_QWS_CURSOR
//    qt_screen->initCursor(sharedram + ramlen,TRUE);

    // default cursor
    cursor = 0;
    setCursor(QWSCursor::systemCursor(ArrowCursor));
#endif
    sendMouseEvent( QPoint(swidth/2, sheight/2), 0 );
}

void QWSServer::setCursor(QWSCursor *curs)
{
#ifndef QT_NO_QWS_CURSOR
    if (cursor == curs)
    	return;

    cursor = curs;

    if ( !haveviscurs || !curs )
	curs = QWSCursor::systemCursor(BlankCursor);

    qt_screencursor->hide();
    qt_screencursor->set(curs->image(),
			 curs->hotSpot().x(),
			 curs->hotSpot().y());
    qt_screencursor->show();
#endif
}

#ifndef QT_NO_QWS_CURSOR
static void cleanupSystemCursorTable()
{
    for ( int i = 0; i <= Qt::LastCursor; i++ )
	if ( systemCursorTable[i] ) {
	    delete systemCursorTable[i];
	    systemCursorTable[i] = 0;
	}
}
#endif

void QWSCursor::createSystemCursor( int id )
{
#ifndef QT_NO_QWS_CURSOR
    if ( !systemCursorTableInit ) {
	for ( int i = 0; i <= LastCursor; i++ )
	    systemCursorTable[i] = 0;
	qAddPostRoutine( cleanupSystemCursorTable );
	systemCursorTableInit = true;
    }
    switch ( id ) {
	// 16x16 cursors
	case ArrowCursor:
	    systemCursorTable[ArrowCursor] =
		new QWSCursor(cur_arrow_bits, mcur_arrow_bits, 16, 16, 0, 0);
	    break;

	case UpArrowCursor:
	    systemCursorTable[UpArrowCursor] =
		new QWSCursor(cur_up_arrow_bits, mcur_up_arrow_bits, 16, 16, 7, 0);
	    break;

	case CrossCursor:
	    systemCursorTable[CrossCursor] =
		new QWSCursor(cur_cross_bits, mcur_cross_bits, 16, 16, 7, 7);
	    break;

	case IbeamCursor:
	    systemCursorTable[IbeamCursor] =
		new QWSCursor(cur_ibeam_bits, mcur_ibeam_bits, 16, 16, 7, 7);
	    break;

	case SizeVerCursor:
	    systemCursorTable[SizeVerCursor] =
		new QWSCursor(cur_ver_bits, mcur_ver_bits, 16, 16, 7, 7);
	    break;

	case SizeHorCursor:
	    systemCursorTable[SizeHorCursor] =
		new QWSCursor(cur_hor_bits, mcur_hor_bits, 16, 16, 7, 7);
	    break;

	case SizeBDiagCursor:
	    systemCursorTable[SizeBDiagCursor] =
		new QWSCursor(cur_bdiag_bits, mcur_bdiag_bits, 16, 16, 7, 7);
	    break;

	case SizeFDiagCursor:
	    systemCursorTable[SizeFDiagCursor] =
		new QWSCursor(cur_fdiag_bits, mcur_fdiag_bits, 16, 16, 7, 7);
	    break;

	case BlankCursor:
	    systemCursorTable[BlankCursor] =
		new QWSCursor(0, 0, 0, 0, 0, 0);
	    break;

	// 20x20 cursors
	case ForbiddenCursor:
	    systemCursorTable[ForbiddenCursor] =
		new QWSCursor(forbidden_bits, forbiddenm_bits, 20, 20, 10, 10);
	    break;

	// 32x32 cursors
	case WaitCursor:
	    systemCursorTable[WaitCursor] =
		new QWSCursor(wait_data_bits, wait_mask_bits, 32, 32, 15, 15);
	    break;

	case SplitVCursor:
	    systemCursorTable[SplitVCursor] =
		new QWSCursor(vsplit_bits, vsplitm_bits, 32, 32, 15, 15);
	    break;

	case SplitHCursor:
	    systemCursorTable[SplitHCursor] =
		new QWSCursor(hsplit_bits, hsplitm_bits, 32, 32, 15, 15);
	    break;

	case SizeAllCursor:
	    systemCursorTable[SizeAllCursor] =
		new QWSCursor(size_all_data_bits, size_all_mask_bits, 32, 32, 15, 15);
	    break;

	case PointingHandCursor:
	    systemCursorTable[PointingHandCursor] =
		new QWSCursor(phand_bits, phandm_bits, 32, 32, 0, 0);
	    break;

	case WhatsThisCursor:
	    systemCursorTable[WhatsThisCursor] =
		new QWSCursor(whatsthis_bits, whatsthism_bits, 32, 32, 0, 0);
	    break;
	case BusyCursor:
	    systemCursorTable[BusyCursor] =
		new QWSCursor(busy_bits, busym_bits, 32, 32, 0, 0);
	    break;
	default:
	    qWarning( "Unknown system cursor %d", id );
    }
#endif
}

QWSCursor *QWSCursor::systemCursor(int id)
{
    QWSCursor *cursor = 0;
#ifndef QT_NO_QWS_CURSOR
    if (id >= 0 && id <= LastCursor) {
	if ( !systemCursorTable[id] )
	    createSystemCursor(id);
	cursor = systemCursorTable[id];
    }

    if (cursor == 0) {
	if ( !systemCursorTable[ArrowCursor] )
	    createSystemCursor(ArrowCursor);
	cursor = systemCursorTable[ArrowCursor];
    }
#endif
    return cursor;
}

void QWSCursor::set(const uchar *data, const uchar *mask,
		    int width, int height, int hx, int hy)
{
#ifndef QT_NO_QWS_CURSOR
    hot.setX(hx);
    hot.setY(hy);

    cursor.create(width, height, 8, 3);

    if ( !width || !height || !data || !mask )
	return;

    cursor.setColor(0, 0xff000000);
    cursor.setColor(1, 0xffffffff);
    cursor.setColor(2, 0x00000000);

    int bytesPerLine = (width + 7) / 8;
    int p = 0;
    int d, m;

    int x = -1, w = 0;

    for (int i = 0; i < height; i++)
    {
	for (int j = 0; j < bytesPerLine; j++, data++, mask++)
	{
	    for (int b = 0; b < 8 && j*8+b < width; b++)
	    {
		d = *data & (1 << b);
		m = *mask & (1 << b);
		if (d && m) p = 0;
		else if (!d && m) p = 1;
		else p = 2;
		cursor.setPixel(j*8+b, i, p);

		// calc region
		if (x < 0 && m)
		    x = j*8+b;
		else if (x >= 0 && !m) {
		    rgn = rgn.unite(QRect(x, i, w, 1));
		    x = -1;
		    w = 0;
		}
		if (m)
		    w++;
	    }
	}
	if (x >= 0) {
	    rgn = rgn.unite(QRect(x, i, w, 1));
	    x = -1;
	    w = 0;
	}
    }

    if ( qt_screencursor) {
	if ( qt_screencursor->supportsAlphaCursor() ) {
	    createDropShadow(5, 2);
	}
    }
#endif
}

// ### now we're really silly
void QWSCursor::createDropShadow(int dropx, int dropy)
{
#if !defined(QT_NO_QWS_CURSOR) && !defined(QT_NO_QWS_ALPHA_CURSOR)
    if (cursor.width() + dropx > 64 || cursor.height() + dropy > 64)
	return;

    if ( !cursor.hasAlphaBuffer() ) {
	cursor.setAlphaBuffer(true);

	const int nblur=4;
	const int darkness=140;

	QImage drop(cursor.width()+dropx+nblur, cursor.height()+dropy+nblur, 8, 18);
	drop.setColor(0, 0xff000000); // bg (black)
	drop.setColor(1, 0xffffffff); // fg (white)
	for (int i=0; i<16; i++) {
	    drop.setColor(2+i, (darkness*i/16)<<24);
	}
	drop.fill(2); // all trans
	QImage drop2 = drop.copy();

	int cp;

	// made solid shadow
	for (int row = 0; row < cursor.height(); row++) {
	    for (int col = 0; col < cursor.width(); col++) {
		cp = cursor.pixelIndex(col, row);
		if (cp != 2)
		    drop.setPixel(col+dropx, row+dropy, 17);
	    }
	}

	// blur shadow
	for (int blur=0; blur<nblur; blur++) {
	    QImage& to((blur&1)?drop:drop2);
	    QImage& from((blur&1)?drop2:drop);
	    for (int row = 1; row < drop.height()-1; row++) {
		for (int col = 1; col < drop.width()-1; col++) {
		    int t=0;
		    for (int dx=-1; dx<=1; dx++) {
			for (int dy=-1; dy<=1; dy++) {
			    t += from.pixelIndex(col+dx,row+dy)-2;
			}
		    }
		    to.setPixel(col,row,2+t/9);
		}
	    }
	}

	// copy cursor
	for (int row = 0; row < cursor.height(); row++) {
	    for (int col = 0; col < cursor.width(); col++) {
		cp = cursor.pixelIndex(col, row);
		if (cp != 2)
		    drop.setPixel(col, row, cp);
	    }
	}

	cursor = drop;
    }
#endif
}

