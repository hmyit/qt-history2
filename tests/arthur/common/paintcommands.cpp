#include "paintcommands.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpainter.h>
#include <q3painter.h>
#include <qbitmap.h>
#include <qtextstream.h>
#include <qtextlayout.h>
#include <qdebug.h>

#ifndef QT_NO_OPENGL
#include <qglpixelbuffer.h>
#endif

/*********************************************************************************
** everything to populate static tables
**********************************************************************************/
const char *PaintCommands::brushStyleTable[] = {
    "NoBrush",
    "SolidPattern",
    "Dense1Pattern",
    "Dense2Pattern",
    "Dense3Pattern",
    "Dense4Pattern",
    "Dense5Pattern",
    "Dense6Pattern",
    "Dense7Pattern",
    "HorPattern",
    "VerPattern",
    "CrossPattern",
    "BDiagPattern",
    "FDiagPattern",
    "DiagCrossPattern",
    "LinearGradientPattern"
};

const char *PaintCommands::penStyleTable[] = {
    "NoPen",
    "SolidLine",
    "DashLine",
    "DotLine",
    "DashDotLine",
    "DashDotDotLine"
};

const char *PaintCommands::fontWeightTable[] = {
    "Light",
    "Normal",
    "DemiBold",
    "Bold",
    "Black"
};

const char *PaintCommands::clipOperationTable[] = {
    "NoClip",
    "ReplaceClip",
    "IntersectClip",
    "UniteClip"
};

const char *PaintCommands::spreadMethodTable[] = {
    "PadSpread",
    "ReflectSpread",
    "RepeatSpread"
};

const char *PaintCommands::compositionModeTable[] = {
    "SourceOver",
    "DestinationOver",
    "Clear",
    "Source",
    "Destination",
    "SourceIn",
    "DestinationIn",
    "SourceOut",
    "DestinationOut",
    "SourceAtop",
    "DestinationAtop",
    "Xor"
};

const char *PaintCommands::imageFormatTable[] = {
    "Invalid",
    "Mono",
    "MonoLSB",
    "Indexed8",
    "RGB32",
    "ARGB32",
    "ARGB32_Premultiplied"
};

int PaintCommands::translateEnum(const char *table[], const QString &pattern, int limit)
{
    for (int i=0; i<limit; ++i)
        if (pattern.toLower() == QString(table[i]).toLower())
            return i;
    return -1;
}

QList<PaintCommands::PaintCommandInfos> PaintCommands::s_commandInfoTable = QList<PaintCommands::PaintCommandInfos>();

#define DECL_PAINTCOMMAND(identifier, method, regexp, syntax, sample) \
    s_commandInfoTable << PaintCommandInfos(QLatin1String(identifier), &PaintCommands::method, QRegExp(regexp), \
                                            QLatin1String(syntax), QLatin1String(sample) );

#define DECL_PAINTCOMMANDSECTION(title) \
    s_commandInfoTable << PaintCommandInfos(QLatin1String(title));

void PaintCommands::staticInit()
{
    if (!s_commandInfoTable.isEmpty()) return;

    DECL_PAINTCOMMANDSECTION("misc");
    DECL_PAINTCOMMAND("comment", command_comment,
                      "^\\s*#",
                      "# this is some comments",
                      "# place your comments here");
    DECL_PAINTCOMMAND("import", command_import,
                      "^import\\s+\"(.*)\"$",
                      "import <qrcFilename>",
                      "import \"myfile.qrc\"");
    DECL_PAINTCOMMAND("begin_block", command_begin_block,
                      "^begin_block\\s+(\\w*)$",
                      "begin_block <blockName>",
                      "begin_block blockName");
    DECL_PAINTCOMMAND("end_block", command_end_block,
                      "^end_block$",
                      "end_block",
                      "end_block");
    DECL_PAINTCOMMAND("repeat_block", command_repeat_block,
                      "^repeat_block\\s+(\\w*)$",
                      "repeat_block <blockName>",
                      "repeat_block blockName");
    DECL_PAINTCOMMAND("textlayout_draw", command_textlayout_draw,
                      "^textlayout_draw\\s+\"(.*)\"\\s+([0-9.]*)$",
                      "textlayout_draw <text> <width>",
                      "textlayout_draw \"your text\" 1.0");
    DECL_PAINTCOMMAND("abort", command_abort,
                      "^abort$",
                      "abort",
                      "abort");
    DECL_PAINTCOMMAND("noop", command_noop,
                      "^$",
                      "-",
                      "\n");

    DECL_PAINTCOMMANDSECTION("setters");
    DECL_PAINTCOMMAND("setBackgroundMode", command_setBgMode,
                      "^(setBackgroundMode|setBgMode)\\s+(\\w*)$",
                      "setBackgroundMode <OpaqueMode|TransparentMode>",
                      "setBackgroundMode TransparentMode");
    DECL_PAINTCOMMAND("setBackground", command_setBackground,
                      "^setBackground\\s+#?(\\w*)\\s*(\\w*)?$",
                      "setBackground <color> [pattern]",
                      "setBackground black SolidPattern");
    DECL_PAINTCOMMAND("setOpacity", command_setOpacity,
                      "^setOpacity\\s+(-?\\d*\\.?\\d*)$",
                      "setOpacity <opacity>\n  - opacity is in [0,1]",
                      "setOpacity 1.0");
    DECL_PAINTCOMMAND("path_setFillRule", command_path_setFillRule,
                      "^path_setFillRule\\s+(\\w*)\\s+(\\w*)$",
                      "path_setFillRule <pathName> [Winding|OddEven]",
                      "path_setFillRule pathName Winding");
    DECL_PAINTCOMMAND("setBrush", command_setBrush,
                      "^setBrush\\s+(#?[\\w.:\\/]*)\\s*(\\w*)?$",
                      "setBrush <pixmapFileName>\nsetBrush noBrush\nsetBrush <color> <pattern>",
                      "setBrush white SolidPattern");
    DECL_PAINTCOMMAND("setBrushOrigin", command_setBrushOrigin,
                      "^setBrushOrigin\\s*(-?\\w*)\\s+(-?\\w*)$",
                      "setBrushOrigin <dx> <dy>",
                      "setBrushOrigin 0 0");
    DECL_PAINTCOMMAND("brushTranslate", command_brushTranslate,
                      "^brushTranslate\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "brushTranslate <tx> <ty>",
                      "brushTranslate 0.0 0.0");
    DECL_PAINTCOMMAND("brushScale", command_brushScale,
                      "^brushScale\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "brushScale <kx> <ky>",
                      "brushScale 0.0 0.0");
    DECL_PAINTCOMMAND("brushRotate", command_brushRotate,
                      "^brushRotate\\s+(-?[\\w.]*)$",
                      "brushRotate <angle>\n - angle in degrees",
                      "brushRotate 0.0");
    DECL_PAINTCOMMAND("brushShear", command_brushShear,
                      "^brushShear\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "brushShear <sx> <sy>",
                      "brushShear 0.0 0.0");
    DECL_PAINTCOMMAND("setCompositionMode", command_setCompositionMode,
                      "^setCompositionMode\\s+([\\w_0-9]*)$",
                      "setCompositionMode <compositionMode>",
                      "setCompositionMode SourceOver");
    DECL_PAINTCOMMAND("setFont", command_setFont,
                      "^setFont\\s+\"([\\w\\s]*)\"\\s*(\\w*)\\s*(\\w*)\\s*(\\w*)$",
                      "setFont <fontFace> [size] [normal|bold] [italic]",
                      "setFont \"times\" normal");
    DECL_PAINTCOMMAND("setPen", command_setPen,
                      "^setPen\\s+#?(\\w*)$",
                      "setPen <color>\nsetPen <penStyle>\nsetPen brush\nsetPen NoPen",
                      "setPen black");
    DECL_PAINTCOMMAND("setPen", command_setPen2,
                      "^setPen\\s+(#?\\w*)\\s+([\\w.]+)\\s*(\\w*)\\s*(\\w*)\\s*(\\w*)$",
                      "setPen brush|<color> [width] [penStyle] [capStyle] [joinStyle]",
                      "setPen black 1 FlatCap MiterJoin");
    DECL_PAINTCOMMAND("pen_setDashPattern", command_pen_setDashPattern,
                      "^pen_setDashPattern\\s+\\[([\\w\\s.]*)\\]$",
                      "pen_setDashPattern <[ dash_1 space_1 ... dash_n space_n]>",
                      "pen_setDashPattern [ 2 1 4 1 3 3 ]");
    DECL_PAINTCOMMAND("pen_setCosmetic", command_pen_setCosmetic,
                      "^pen_setCosmetic\\s+(\\w*)$",
                      "pen_setCosmetic <true | false>",
                      "pen_setCosmetic true");
    DECL_PAINTCOMMAND("setRenderHint", command_setRenderHint,
                      "^setRenderHint\\s+([\\w_0-9]*)\\s*(\\w*)$",
                      "setRenderHint <Antialiasing|SmoothPixmapTransform> <true|false>",
                      "setRenderHint Antialiasing true");
    DECL_PAINTCOMMAND("clearRenderHint", command_clearRenderHint,
                      "^clearRenderHint$",
                      "clearRenderHint",
                      "clearRenderHint");

    DECL_PAINTCOMMANDSECTION("gradients");
    DECL_PAINTCOMMAND("gradient_appendStop", command_gradient_appendStop,
                      "^gradient_appendStop\\s+([\\w.]*)\\s+#?(\\w*)$",
                      "gradient_appendStop <pos> <color>",
                      "gradient_appendStop 1.0 red");
    DECL_PAINTCOMMAND("gradient_clearStops", command_gradient_clearStops,
                      "^gradient_clearStops$",
                      "gradient_clearStops",
                      "gradient_clearStops");
    DECL_PAINTCOMMAND("gradient_setConical", command_gradient_setConical,
                      "^gradient_setConical\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)$",
                      "gradient_setConical <cx> <cy> <angle>\n  - angle in degrees",
                      "gradient_setConical 5.0 5.0 45.0");
    DECL_PAINTCOMMAND("gradient_setLinear", command_gradient_setLinear,
                      "^gradient_setLinear\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)$",
                      "gradient_setLinear <x1> <y1> <x2> <y2>",
                      "gradient_setLinear 1.0 1.0 2.0 2.0");
    DECL_PAINTCOMMAND("gradient_setRadial", command_gradient_setRadial,
                      "^gradient_setRadial\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)\\s?([\\w.]*)\\s?([\\w.]*)$",
                      "gradient_setRadial <cx> <cy> <rad> <fx> <fy>\n  - C is the center\n  - rad in the angle in degrees\n  - F is the focal point",
                      "gradient_setRadial 1.0 1.0 45.0 2.0 2.0");
    DECL_PAINTCOMMAND("gradient_setSpread", command_gradient_setSpread,
                      "^gradient_setSpread\\s+(\\w*)$",
                      "gradient_setSpread <PadSpread|ReflectSpread|RepeatSpread>",
                      "gradient_setSpread PadSpread");

    DECL_PAINTCOMMANDSECTION("qt3 drawing ops");
    DECL_PAINTCOMMAND("qt3_drawArc", command_qt3_drawArc,
                      "^qt3_drawArc\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("qt3_drawChord", command_qt3_drawChord,
                      "^qt3_drawChord\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("qt3_drawEllipse", command_qt3_drawEllipse,
                      "^qt3_drawEllipse\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("qt3_drawPie", command_qt3_drawPie,
                      "^qt3_drawPie\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("qt3_drawRect", command_qt3_drawRect,
                      "^qt3_drawRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("qt3_drawRoundRect", command_qt3_drawRoundRect,
                      "^qt3_drawRoundRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w)?\\s*(-?\\w)?$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("drawing ops");
    DECL_PAINTCOMMAND("drawPoint", command_drawPoint,
                      "^drawPoint\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawLine", command_drawLine,
                      "^drawLine\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawRect", command_drawRect,
                      "^drawRect\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawRoundRect", command_drawRoundRect,
                      "^drawRoundRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawArc", command_drawArc,
                      "^drawArc\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawChord", command_drawChord,
                      "^drawChord\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawEllipse", command_drawEllipse,
                      "^drawEllipse\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawPath", command_drawPath,
                      "^drawPath\\s+(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawPie", command_drawPie,
                      "^drawPie\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawPixmap", command_drawPixmap,
                      "^drawPixmap\\s+([\\w.:\\-/]*)"                                             
                      "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?"    // target rect                                             
                      "\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?$", // source rect
                      "",
                      "");
    DECL_PAINTCOMMAND("drawImage", command_drawImage,
                      "^drawImage\\s+([\\w.:\\/]*)"
                      "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?"    // target rect
                      "\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?$", // source rect
                      "",
                      "");
    DECL_PAINTCOMMAND("drawPolygon", command_drawPolygon,
                      "^drawPolygon\\s+\\[([\\w\\s-.]*)\\]\\s*(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawConvexPolygon", command_drawConvexPolygon,
                      "^drawConvexPolygon\\s+\\[([\\w\\s-.]*)\\]$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawPolyline", command_drawPolyline,
                      "^drawPolyline\\s+\\[([\\w\\s]*)\\]$",
                      "",
                      "");
    DECL_PAINTCOMMAND("drawText", command_drawText,
                      "^drawText\\s+(-?\\w*)\\s+(-?\\w*)\\s+\"(.*)\"$",
                      "",
                      "");
    DECL_PAINTCOMMAND("", command_drawTiledPixmap,
                      "^drawTiledPixmap\\s+([\\w.:\\/]*)"
                      "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)\\s*(-?\\w*)"
                      "\\s*(-?\\w*)\\s*(-?\\w*)$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("painterPaths");
    DECL_PAINTCOMMAND("path_moveTo", command_path_moveTo,
                      "^path_moveTo\\s+([.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_lineTo", command_path_lineTo,
                      "^path_lineTo\\s+([.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_addEllipse", command_path_addEllipse,
                      "^path_addEllipse\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_addPolygon", command_path_addPolygon,
                      "^path_addPolygon\\s+(\\w*)\\s+\\[([\\w\\s]*)\\]\\s*(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_addRect", command_path_addRect,
                      "^path_addRect\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_addText", command_path_addText,
                      "^path_addText\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+\"(.*)\"$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_arcTo", command_path_arcTo,
                      "^path_arcTo\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_cubicTo", command_path_cubicTo,
                      "^path_cubicTo\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_closeSubpath", command_path_closeSubpath,
                      "^path_closeSubpath\\s+(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_createOutline", command_path_createOutline,
                      "^path_createOutline\\s+(\\w*)\\s+(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_debugPrint", command_path_debugPrint,
                      "^path_debugPrint\\s+(\\w*)$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("regions");
    DECL_PAINTCOMMAND("region_addRect", command_region_addRect,
                      "^region_addRect\\s+(\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("region_addEllipse", command_region_addEllipse,
                      "^region_addEllipse\\s+(\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("clipping");
    DECL_PAINTCOMMAND("region_getClipRegion", command_region_getClipRegion,
                      "^region_getClipRegion\\s+(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("path_getClipPath", command_path_getClipPath,
                      "^path_getClipPath\\s+([\\w0-9]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("setClipPath", command_setClipPath,
                      "^setClipPath\\s+(\\w*)\\s*(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("setClipRect", command_setClipRect,
                      "^setClipRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("setClipRegion", command_setClipRegion,
                      "^setClipRegion\\s+(\\w*)\\s*(\\w*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("setClipping", command_setClipping,
                      "^setClipping\\s+(\\w*)$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("surface");
    DECL_PAINTCOMMAND("surface_begin", command_surface_begin,
                      "^surface_begin\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("surface_end", command_surface_end,
                      "^surface_end$",
                      "surface_end",
                      "surface_end");

    DECL_PAINTCOMMANDSECTION("painter states");
    DECL_PAINTCOMMAND("restore", command_restore,
                      "^restore$",
                      "restore",
                      "restore");
    DECL_PAINTCOMMAND("save", command_save,
                      "^save$",
                      "save",
                      "save");

    DECL_PAINTCOMMANDSECTION("pixmaps'n'images");
    DECL_PAINTCOMMAND("pixmap_load", command_pixmap_load,
                      "^pixmap_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("pixmap_setMask", command_pixmap_setMask,
                      "^pixmap_setMask\\s+([\\w.:\\/]*)\\s+([\\w.:\\/]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("bitmap_load", command_bitmap_load,
                      "^bitmap_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("image_convertToFormat", command_image_convertToFormat,
                      "^image_convertToFormat\\s+([\\w.:\\/]*)\\s+([\\w.:\\/]+)\\s+([\\w0-9_]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("image_load", command_image_load,
                      "^image_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("image_setColor", command_image_setColor,
                      "^image_setColor\\s+([\\w.:\\/]*)\\s+([0-9]*)\\s+#([0-9]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("image_setNumColors", command_image_setNumColors,
                      "^image_setNumColors\\s+([\\w.:\\/]*)\\s+([0-9]*)$",
                      "",
                      "");

    DECL_PAINTCOMMANDSECTION("transformations");
    DECL_PAINTCOMMAND("resetMatrix", command_resetMatrix,
                      "^resetMatrix$",
                      "resetMatrix",
                      "resetMatrix");
    DECL_PAINTCOMMAND("setMatrix", command_setMatrix,
                      "^setMatrix\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("translate", command_translate,
                      "^translate\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("rotate", command_rotate,
                      "^rotate\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("scale", command_scale,
                      "^scale\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)$",
                      "",
                      "");
    DECL_PAINTCOMMAND("mapQuadToQuad", command_mapQuadToQuad,
                      "^mapQuadToQuad\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)$",
                      "",
                      "");
}

#undef DECL_PAINTCOMMAND
/*
                                                                typedef void (PaintCommands::*qPaintCommand) (QRegExp re);

                                                                struct PaintCommand {
                                                                    PaintCommand(QRegExp re, qPaintCommand cmd)
                                                                        : regExp(re), command(cmd)
                                                                    {
                                                                        Q_ASSERT(re.isValid());
                                                                    }
                                                                
                                                                    QRegExp regExp;
                                                                    qPaintCommand command;
                                                                };
*/
/*********************************************************************************
** utility
**********************************************************************************/
template <typename T> T PaintCommands::image_load(const QString &filepath)
{
    T t(filepath);

    if (t.isNull())
        t = T(":images/" + filepath);

    if (t.isNull())
        t = T("images/" + filepath);

    if (t.isNull()) {
        QFileInfo fi(filepath);
        QDir dir = fi.absoluteDir();
        dir.cdUp();
        dir.cd("images");
        QString fileName = QString("%1/%2").arg(dir.absolutePath()).arg(fi.fileName());
        t = T(fileName);
        if (t.isNull() && !fileName.endsWith(".png")) {
            fileName.append(".png");
            t = T(fileName);
        }
    }

    return t;
}

//                                                                                       static QList<PaintCommand> commandTable;
/*********************************************************************************
** setters
**********************************************************************************/
void PaintCommands::insertAt(int commandIndex, const QStringList &newCommands)
{
    int index = 0;
    int left = newCommands.size();
    while (left--)
        m_commands.insert(++commandIndex, newCommands.at(index++));
}

/*********************************************************************************
** run
**********************************************************************************/
void PaintCommands::runCommand(const QString &scriptLine)
{
    staticInit();
    foreach (PaintCommandInfos command, s_commandInfoTable)
        if (!command.isSectionHeader() && command.regExp.indexIn(scriptLine) >= 0) {
            (this->*(command.paintMethod))(command.regExp);
            break;
        }
}

void PaintCommands::runCommands()
{
    staticInit();
    /*if (commandTable.isEmpty()) {
        static QRegExp comment("^\\s*#");
        static QRegExp import("import\\s+\"(.*)\"");

        static QRegExp begin_block("begin_block\\s+(\\w*)");
        static QRegExp end_block("end_block");
        static QRegExp repeat_block("repeat_block\\s+(\\w*)");

        static QRegExp drawEllipse("drawEllipse\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp drawRect("drawRect\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp drawRoundRect("drawRoundRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?");
        static QRegExp drawPie("drawPie\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp drawChord("drawChord\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp drawArc("drawArc\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp qt3_drawEllipse("qt3_drawEllipse\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp qt3_drawRect("qt3_drawRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp qt3_drawRoundRect("qt3_drawRoundRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w)?\\s*(-?\\w)?");
        static QRegExp qt3_drawPie("qt3_drawPie\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp qt3_drawChord("qt3_drawChord\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp qt3_drawArc("qt3_drawArc\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp drawLine("drawLine\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp drawPath("drawPath\\s+(\\w*)");
        static QRegExp drawPixmap("drawPixmap\\s+([\\w.:\\-/]*)"
                                  "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?"    // target rect
                                  "\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?");  // source rect
        static QRegExp drawImage("drawImage\\s+([\\w.:\\/]*)"
                                 "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)?\\s*(-?\\w*)?"    // target rect
                                 "\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?\\s*(-?\\w*)?");  // source rect
        static QRegExp drawPoint("drawPoint\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp drawPolygon("drawPolygon\\s+\\[([\\w\\s-.]*)\\]\\s*(\\w*)");
        static QRegExp drawConvexPolygon("drawConvexPolygon\\s+\\[([\\w\\s-.]*)\\]");
        static QRegExp drawPolyline("drawPolyline\\s+\\[([\\w\\s]*)\\]");
        static QRegExp drawText("drawText\\s+(-?\\w*)\\s+(-?\\w*)\\s+\"(.*)\"");
        static QRegExp drawTiledPixmap("drawTiledPixmap\\s+([\\w.:\\/]*)"
                                       "\\s+(-?\\w*)\\s+(-?\\w*)\\s*(-?\\w*)\\s*(-?\\w*)"
                                       "\\s*(-?\\w*)\\s*(-?\\w*)");
        static QRegExp empty(".*");
        static QRegExp path_addEllipse("path_addEllipse\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_addRect("path_addRect\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_addPolygon("path_addPolygon\\s+(\\w*)\\s+\\[([\\w\\s]*)\\]\\s*(\\w*)");
        static QRegExp path_addText("path_addText\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+\"(.*)\"");
        static QRegExp path_arcTo("path_arcTo\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_cubicTo("path_cubicTo\\s+(\\w*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_lineTo("path_lineTo\\s+([.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_moveTo("path_moveTo\\s+([.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp path_createOutline("path_createOutline\\s+(\\w*)\\s+(\\w*)");
        static QRegExp path_getClipPath("path_getClipPath\\s+([\\w0-9]*)");
        static QRegExp path_closeSubpath("path_closeSubpath\\s+(\\w*)");
        static QRegExp path_setFillRule("path_setFillRule\\s+(\\w*)\\s+(\\w*)");
        static QRegExp path_debugPrint("path_debugPrint\\s+(\\w*)");

        static QRegExp region_addRect("region_addRect\\s+(\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp region_addEllipse("region_addEllipse\\s+(\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)");
        static QRegExp region_getClipRegion("region_getClipRegion\\s+(\\w*)");
        static QRegExp resetMatrix("resetMatrix");
        static QRegExp restore("restore");
        static QRegExp rotate("rotate\\s+(-?[\\w.]*)");
        static QRegExp save("save");
        static QRegExp scale("scale\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp mapQuadToQuad("mapQuadToQuad\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp setMatrix("setMatrix\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)\\s+(-?[.\\w]*)");
        static QRegExp setBackground("setBackground\\s+#?(\\w*)\\s*(\\w*)?");
        static QRegExp setOpacity("setOpacity\\s+(-?[\\w.]*)");
        static QRegExp setBgMode("setBackgroundMode\\s+(\\w*)");
        static QRegExp setBrush("setBrush\\s+(#?[\\w.:\\/]*)\\s*(\\w*)?");
        static QRegExp setBrushOrigin("setBrushOrigin\\s*(-?\\w*)\\s+(-?\\w*)");

        static QRegExp brushTranslate("brushTranslate\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp brushScale("brushScale\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp brushShear("brushShear\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp brushRotate("brushRotate\\s+(-?[\\w.]*)");

        static QRegExp setClipPath("setClipPath\\s+(\\w*)\\s*(\\w*)");
        static QRegExp setClipRect("setClipRect\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(\\w*)");
//        static QRegExp setClipRectangle("setClipRectangle\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s+(-?\\w*)\\s*(\\w*)");
        static QRegExp setClipRegion("setClipRegion\\s+(\\w*)\\s*(\\w*)");
        static QRegExp setClipping("setClipping\\s+(\\w*)");
        static QRegExp setFont("setFont\\s+\"([\\w\\s]*)\"\\s*(\\w*)\\s*(\\w*)\\s*(\\w*)");
        static QRegExp setPen("setPen\\s+#?(\\w*)");
        static QRegExp setPen2("setPen\\s+(#?\\w*)\\s+([\\w.]+)\\s*(\\w*)\\s*(\\w*)\\s*(\\w*)");
        static QRegExp pen_setCosmetic("pen_setCosmetic\\s+(\\w*)");
        static QRegExp pen_setDashPattern("pen_setDashPattern\\s+\\[([\\w\\s.]*)\\]");

        static QRegExp setRenderHint("setRenderHint\\s+([\\w_0-9]*)\\s*(\\w*)");
        static QRegExp setCompositionMode("setCompositionMode\\s+([\\w_0-9]*)");

        static QRegExp surface_begin("surface_begin\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp surface_end("surface_end");

        static QRegExp gradient_clearStops("gradient_clearStop");
        static QRegExp gradient_appendStop("gradient_appendStop\\s+([\\w.]*)\\s+#?(\\w*)");
        static QRegExp gradient_setLinear("gradient_setLinear\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)");
        static QRegExp gradient_setRadial("gradient_setRadial\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)\\s?([\\w.]*)\\s?([\\w.]*)");
        static QRegExp gradient_setConical("gradient_setConical\\s+([\\w.]*)\\s+([\\w.]*)\\s+([\\w.]*)");
        static QRegExp gradient_setSpread("gradient_setSpread\\s+(\\w*)");

        static QRegExp translate("translate\\s+(-?[\\w.]*)\\s+(-?[\\w.]*)");
        static QRegExp pixmap_load("pixmap_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)");
        static QRegExp bitmap_load("bitmap_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)");
        static QRegExp pixmap_setMask("pixmap_setMask\\s+([\\w.:\\/]*)\\s+([\\w.:\\/]*)");
        static QRegExp image_load("image_load\\s+([\\w.:\\/]*)\\s*([\\w.:\\/]*)");
        static QRegExp image_setNumColors("image_setNumColors\\s+([\\w.:\\/]*)\\s+([0-9]*)");
        static QRegExp image_setColor("image_setColor\\s+([\\w.:\\/]*)\\s+([0-9]*)\\s+#([0-9]*)");
        static QRegExp image_convertToFormat("image_convertToFormat\\s+([\\w.:\\/]*)\\s+([\\w.:\\/]+)\\s+([\\w0-9_]*)");

        static QRegExp textlayout_draw("textlayout_draw\\s+\"(.*)\"\\s+([0-9.]*)");

        static QRegExp abort("^abort");

        // Skip comments first of all
        commandTable.append(PaintCommand(comment,             &PaintCommands::command_comment));

        // Importer
        commandTable.append(PaintCommand(import,              &PaintCommands::command_import));

        // Block commands
        commandTable.append(PaintCommand(begin_block,         &PaintCommands::command_begin_block));
        commandTable.append(PaintCommand(end_block,           &PaintCommands::command_end_block));
        commandTable.append(PaintCommand(repeat_block,        &PaintCommands::command_repeat_block));

        // Setters
        commandTable.append(PaintCommand(setBgMode,           &PaintCommands::command_setBgMode));
        commandTable.append(PaintCommand(setBackground,       &PaintCommands::command_setBackground));
        commandTable.append(PaintCommand(setOpacity,          &PaintCommands::command_setOpacity));
        commandTable.append(PaintCommand(setBrush,            &PaintCommands::command_setBrush));
        commandTable.append(PaintCommand(setBrushOrigin,      &PaintCommands::command_setBrushOrigin));
        commandTable.append(PaintCommand(brushTranslate,      &PaintCommands::command_brushTranslate));
        commandTable.append(PaintCommand(brushScale,          &PaintCommands::command_brushScale));
        commandTable.append(PaintCommand(brushRotate,         &PaintCommands::command_brushRotate));
        commandTable.append(PaintCommand(brushShear,          &PaintCommands::command_brushShear));

        commandTable.append(PaintCommand(setClipPath,         &PaintCommands::command_setClipPath));
        commandTable.append(PaintCommand(setClipRect,         &PaintCommands::command_setClipRect));
        commandTable.append(PaintCommand(setClipRegion,       &PaintCommands::command_setClipRegion));
        commandTable.append(PaintCommand(setClipping,         &PaintCommands::command_setClipping));
        commandTable.append(PaintCommand(setFont,             &PaintCommands::command_setFont));
        commandTable.append(PaintCommand(setPen2,             &PaintCommands::command_setPen2));
        commandTable.append(PaintCommand(setPen,              &PaintCommands::command_setPen));
        commandTable.append(PaintCommand(pen_setDashPattern,  &PaintCommands::command_pen_setDashPattern));
        commandTable.append(PaintCommand(pen_setCosmetic,     &PaintCommands::command_pen_setCosmetic));
        commandTable.append(PaintCommand(setRenderHint,       &PaintCommands::command_setRenderHint));
        commandTable.append(PaintCommand(setCompositionMode,  &PaintCommands::command_setCompositionMode));
        commandTable.append(PaintCommand(gradient_clearStops, &PaintCommands::command_gradient_clearStops));
        commandTable.append(PaintCommand(gradient_appendStop, &PaintCommands::command_gradient_appendStop));
        commandTable.append(PaintCommand(gradient_setLinear,  &PaintCommands::command_gradient_setLinear));
        commandTable.append(PaintCommand(gradient_setRadial,  &PaintCommands::command_gradient_setRadial));
        commandTable.append(PaintCommand(gradient_setConical, &PaintCommands::command_gradient_setConical));
        commandTable.append(PaintCommand(gradient_setSpread,  &PaintCommands::command_gradient_setSpread));

        // Drawing ops
        commandTable.append(PaintCommand(qt3_drawEllipse,     &PaintCommands::command_qt3_drawEllipse));
        commandTable.append(PaintCommand(qt3_drawArc,         &PaintCommands::command_qt3_drawArc));
        commandTable.append(PaintCommand(qt3_drawChord,       &PaintCommands::command_qt3_drawChord));
        commandTable.append(PaintCommand(qt3_drawPie,         &PaintCommands::command_qt3_drawPie));
        commandTable.append(PaintCommand(qt3_drawRect,        &PaintCommands::command_qt3_drawRect));
        commandTable.append(PaintCommand(qt3_drawRoundRect,   &PaintCommands::command_qt3_drawRoundRect));
        commandTable.append(PaintCommand(drawEllipse,         &PaintCommands::command_drawEllipse));
        commandTable.append(PaintCommand(drawArc,             &PaintCommands::command_drawArc));
        commandTable.append(PaintCommand(drawChord,           &PaintCommands::command_drawChord));
        commandTable.append(PaintCommand(drawPie,             &PaintCommands::command_drawPie));
        commandTable.append(PaintCommand(drawRect,            &PaintCommands::command_drawRect));
        commandTable.append(PaintCommand(drawRoundRect,       &PaintCommands::command_drawRoundRect));
        commandTable.append(PaintCommand(drawLine,            &PaintCommands::command_drawLine));
        commandTable.append(PaintCommand(drawPath,            &PaintCommands::command_drawPath));
        commandTable.append(PaintCommand(drawPixmap,          &PaintCommands::command_drawPixmap));
        commandTable.append(PaintCommand(drawImage,           &PaintCommands::command_drawImage));
        commandTable.append(PaintCommand(drawPoint,           &PaintCommands::command_drawPoint));
        commandTable.append(PaintCommand(drawPolygon,         &PaintCommands::command_drawPolygon));
        commandTable.append(PaintCommand(drawConvexPolygon,   &PaintCommands::command_drawConvexPolygon));
        commandTable.append(PaintCommand(drawPolyline,        &PaintCommands::command_drawPolyline));

        commandTable.append(PaintCommand(drawText,            &PaintCommands::command_drawText));
        commandTable.append(PaintCommand(drawTiledPixmap,     &PaintCommands::command_drawTiledPixmap));
        commandTable.append(PaintCommand(path_addText,        &PaintCommands::command_path_addText));
        commandTable.append(PaintCommand(path_addEllipse,     &PaintCommands::command_path_addEllipse));
        commandTable.append(PaintCommand(path_addRect,        &PaintCommands::command_path_addRect));
        commandTable.append(PaintCommand(path_addPolygon,     &PaintCommands::command_path_addPolygon));
        commandTable.append(PaintCommand(path_arcTo,          &PaintCommands::command_path_arcTo));
        commandTable.append(PaintCommand(path_cubicTo,        &PaintCommands::command_path_cubicTo));
        commandTable.append(PaintCommand(path_createOutline,  &PaintCommands::command_path_createOutline));
        commandTable.append(PaintCommand(path_lineTo,         &PaintCommands::command_path_lineTo));
        commandTable.append(PaintCommand(path_moveTo,         &PaintCommands::command_path_moveTo));
        commandTable.append(PaintCommand(path_closeSubpath,   &PaintCommands::command_path_closeSubpath));
        commandTable.append(PaintCommand(path_setFillRule,    &PaintCommands::command_path_setFillRule));
        commandTable.append(PaintCommand(path_debugPrint,     &PaintCommands::command_path_debugPrint));

        commandTable.append(PaintCommand(region_addRect,      &PaintCommands::command_region_addRect));
        commandTable.append(PaintCommand(region_addEllipse,   &PaintCommands::command_region_addEllipse));
        commandTable.append(PaintCommand(region_getClipRegion,&PaintCommands::command_region_getClipRegion));
        commandTable.append(PaintCommand(path_getClipPath,    &PaintCommands::command_path_getClipPath));

        commandTable.append(PaintCommand(surface_begin,       &PaintCommands::command_surface_begin));
        commandTable.append(PaintCommand(surface_end,         &PaintCommands::command_surface_end));

        // XForms
        commandTable.append(PaintCommand(resetMatrix,    &PaintCommands::command_resetMatrix));
        commandTable.append(PaintCommand(translate,      &PaintCommands::command_translate));
        commandTable.append(PaintCommand(rotate,         &PaintCommands::command_rotate));
        commandTable.append(PaintCommand(scale,          &PaintCommands::command_scale));
        commandTable.append(PaintCommand(mapQuadToQuad,  &PaintCommands::command_mapQuadToQuad));
        commandTable.append(PaintCommand(setMatrix,      &PaintCommands::command_setMatrix));

        // Other commands
        commandTable.append(PaintCommand(save,                 &PaintCommands::command_save));
        commandTable.append(PaintCommand(restore,              &PaintCommands::command_restore));
        commandTable.append(PaintCommand(pixmap_load,          &PaintCommands::command_pixmap_load));
        commandTable.append(PaintCommand(bitmap_load,          &PaintCommands::command_bitmap_load));
        commandTable.append(PaintCommand(pixmap_setMask,       &PaintCommands::command_pixmap_setMask));
        commandTable.append(PaintCommand(image_load,           &PaintCommands::command_image_load));
        commandTable.append(PaintCommand(image_setNumColors,   &PaintCommands::command_image_setNumColors));
        commandTable.append(PaintCommand(image_setColor,       &PaintCommands::command_image_setColor));
        commandTable.append(PaintCommand(image_convertToFormat,&PaintCommands::command_image_convertToFormat));

        commandTable.append(PaintCommand(textlayout_draw, &PaintCommands::command_textlayout_draw));

        commandTable.append(PaintCommand(abort,           &PaintCommands::command_abort));

        // noops
        commandTable.append(PaintCommand(empty,           &PaintCommands::command_noop));
    }*/

    // paint background
    QPixmap pm(20, 20);
    pm.fill(Qt::white);
    if (m_checkers_background) {
        QPainter pt(&pm);
        pt.fillRect(0, 0, 10, 10, QColor::fromRgba(0xffdfdfdf));
        pt.fillRect(10, 10, 10, 10, QColor::fromRgba(0xffdfdfdf));
    }

    m_painter->drawTiledPixmap(0, 0,
                             m_painter->window().width(),
                             m_painter->window().height(), pm);

    // run each command
    m_abort = false;
    for (int i=0; i<m_commands.size() && !m_abort; ++i) {
        const QString &commandNow = m_commands.at(i);
        m_currentCommand = commandNow;
        m_currentCommandIndex = i;
        runCommand(commandNow.trimmed());
    }
}

/*********************************************************************************
** conversions
**********************************************************************************/
int PaintCommands::convertToInt(const QString &str)
{
    return qRound(convertToDouble(str));
}

float PaintCommands::convertToFloat(const QString &str)
{
    return float(convertToDouble(str));
}

double PaintCommands::convertToDouble(const QString &str)
{
    static QRegExp re("cp([0-9])([xy])");
    if (str.toLower() == "width")
        return m_painter->window().width();
    if (str.toLower() == "height")
        return m_painter->window().height();
    if (re.indexIn(str) >= 0) {
        int index = re.cap(1).toInt();
        bool is_it_x = re.cap(2) == "x";
        if (index < 0 || index >= m_controlPoints.size()) {
            qWarning("ERROR: control point index=%d is out of bounds", index);
            return 0;
        }
        return is_it_x ? m_controlPoints.at(index).x() : m_controlPoints.at(index).y();
    }
    return str.toDouble();
}

QColor PaintCommands::convertToColor(const QString &str)
{
    /*static QHash<QString, QColor> predefinedColors;
    if (predefinedColors.size() == 0) {
        predefinedColors["white"] = QColor(Qt::white);
        predefinedColors["black"] = QColor(Qt::black);
        predefinedColors["red"] = QColor(Qt::red);
        predefinedColors["darkred"] = QColor(Qt::darkRed);
        predefinedColors["green"] = QColor(Qt::green);
        predefinedColors["darkgreen"] = QColor(Qt::darkGreen);
        predefinedColors["blue"] = QColor(Qt::blue);
        predefinedColors["darkblue"] = QColor(Qt::darkBlue);
        predefinedColors["cyan"] = QColor(Qt::cyan);
        predefinedColors["darkcyan"] = QColor(Qt::darkCyan);
        predefinedColors["magenta"] = QColor(Qt::magenta);
        predefinedColors["darkmagenta"] = QColor(Qt::darkMagenta);
        predefinedColors["yellow"] = QColor(Qt::yellow);
        predefinedColors["darkyellow"] = QColor(Qt::darkYellow);
        predefinedColors["gray"] = QColor(Qt::gray);
        predefinedColors["darkgray"] = QColor(Qt::darkGray);
        predefinedColors["lightgray"] = QColor(Qt::lightGray);
        predefinedColors["color0"] = QColor(Qt::color0);
        predefinedColors["color1"] = QColor(Qt::color1);
        predefinedColors["transparent"] = QColor(Qt::transparent);
    }

    if (predefinedColors.contains(str))
        return predefinedColors[str];
*/
    static QRegExp alphaColor("#?([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");
    static QRegExp opaqueColor("#?([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

    Q_ASSERT(alphaColor.isValid());
    Q_ASSERT(opaqueColor.isValid());

    if (alphaColor.indexIn(str) >= 0) {
        return QColor(alphaColor.cap(2).toInt(0, 16),
                      alphaColor.cap(3).toInt(0, 16),
                      alphaColor.cap(4).toInt(0, 16),
                      alphaColor.cap(1).toInt(0, 16));
    } else if (opaqueColor.indexIn(str) >= 0) {
        return QColor(opaqueColor.cap(1).toInt(0, 16),
                      opaqueColor.cap(2).toInt(0, 16),
                      opaqueColor.cap(3).toInt(0, 16));
    }
    return QColor(str);
}

/*********************************************************************************
** command implementations
**********************************************************************************/
void PaintCommands::command_comment(QRegExp)
{
    if (m_verboseMode)
        printf(" - comment: %s\n", m_currentCommand.latin1());
}

/***************************************************************************************************/
void PaintCommands::command_import(QRegExp re)
{
    QString importFile(re.cap(1));
    QFileInfo fi(m_filepath);
    QDir dir = fi.absoluteDir();
    QFile *file = new QFile(dir.absolutePath() + QDir::separator() + importFile);

    if (importFile.isEmpty() || !file->exists()) {
        dir.cdUp();
        dir.cd("data");
        dir.cd("qps");
        delete file;
        file = new QFile(dir.absolutePath() + QDir::separator() + importFile);
    }

    if (importFile.isEmpty() || !file->exists()) {
        dir.cdUp();
        dir.cd("images");
        delete file;
        file = new QFile(dir.absolutePath() + QDir::separator() + importFile);
    }

    if (importFile.isEmpty() || !file->exists()) {
        printf(" - importing non-existing file at line %d (%s)\n", m_currentCommandIndex,
               qPrintable(file->fileName()));
        delete file;
        return;
    }

    if (!file->open(QIODevice::ReadOnly)) {
        printf(" - failed to read file: '%s'\n", qPrintable(file->fileName()));
        delete file;
        return;
    }
    if (m_verboseMode)
        printf(" - importing file at line %d (%s)\n", m_currentCommandIndex,
               qPrintable(fi.fileName()));

    QFileInfo fileinfo(*file);
    m_commands[m_currentCommandIndex] = QString("# import file (%1) start").arg(fileinfo.fileName());
    QTextStream textFile(file);
    QString rawContent = textFile.read();
    QStringList importedData = rawContent.split('\n', QString::SkipEmptyParts);
    importedData.append(QString("# import file (%1) end ---").arg(fileinfo.fileName()));
    insertAt(m_currentCommandIndex, importedData);

    if (m_verboseMode) {
        printf(" - Command buffer now looks like:\n");
        for (int i = 0; i < m_commands.count(); ++i)
            printf(" ---> {%s}\n", qPrintable(m_commands.at(i)));
    }
    delete file;
}

/***************************************************************************************************/
void PaintCommands::command_begin_block(QRegExp re)
{
    const QString &blockName = re.cap(1);
    if (m_verboseMode)
        printf(" - begin_block (%s)\n", blockName.latin1());

    m_commands[m_currentCommandIndex] = QString("# begin block (%1)").arg(blockName);
    QStringList newBlock;
    int i = m_currentCommandIndex + 1;
    for (; i < m_commands.count(); ++i) {
        const QString &nextCmd = m_commands.at(i);
        if (nextCmd.startsWith("end_block")) {
            m_commands[i] = QString("# end block (%1)").arg(blockName);
            break;
        }
        newBlock += nextCmd;
    }

    if (m_verboseMode)
        for (int j = 0; j < newBlock.count(); ++j)
            printf("      %d: %s\n", j, newBlock.at(j).latin1());

    if (i >= m_commands.count())
        printf(" - Warning! Block doesn't have an 'end_block' marker!\n");

    m_blockMap.insert(blockName, newBlock);
}

/***************************************************************************************************/
void PaintCommands::command_end_block(QRegExp)
{
    printf(" - end_block should be consumed by begin_block command.\n");
    printf("   You will never see this if your block markers are in sync\n");
    printf("   (noop)\n");
}

/***************************************************************************************************/
void PaintCommands::command_repeat_block(QRegExp re)
{
    QString blockName = re.cap(1);
    if (m_verboseMode)
        printf(" - repeating block (%s)\n", blockName.latin1());

    QStringList block = m_blockMap.value(blockName);
    if (block.isEmpty()) {
        printf(" - repeated block (%s) is empty!\n", blockName.latin1());
        return;
    }

    m_commands[m_currentCommandIndex] = QString("# repeated block (%1)").arg(blockName);
    insertAt(m_currentCommandIndex, block);
}

/***************************************************************************************************/
void PaintCommands::command_drawLine(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double x1 = convertToDouble(caps.at(1));
    double y1 = convertToDouble(caps.at(2));
    double x2 = convertToDouble(caps.at(3));
    double y2 = convertToDouble(caps.at(4));

    if (m_verboseMode)
        printf(" - drawLine((%.2f, %.2f), (%.2f, %.2f))\n", x1, y1, x2, y2);


    m_painter->drawLine(QLineF(x1, y1, x2, y2));
}

/***************************************************************************************************/
void PaintCommands::command_drawPath(QRegExp re)
{
    if (m_verboseMode)
        printf(" - drawPath(name=%s)\n", re.cap(1).latin1());

    QPainterPath &path = m_pathMap[re.cap(1)];
    m_painter->drawPath(path);
}

/***************************************************************************************************/
void PaintCommands::command_drawPixmap(QRegExp re)
{
    QPixmap pm;
    pm = m_pixmapMap[re.cap(1)]; // try cache first
    if (pm.isNull())
        pm = image_load<QPixmap>(re.cap(1));
    if (pm.isNull()) {
        QFileInfo fi(m_filepath);
        QDir dir = fi.absoluteDir();
        dir.cdUp();
        dir.cd("images");
        QString fileName = QString("%1/%2").arg(dir.absolutePath()).arg(re.cap(1));
        pm = QPixmap(fileName);
        if (pm.isNull() && !fileName.endsWith(".png")) {
            fileName.append(".png");
            pm = QPixmap(fileName);
        }
    }
    if (pm.isNull()) {
        fprintf(stderr, "ERROR(drawPixmap): failed to load pixmap: '%s'\n",
                qPrintable(re.cap(1)));
        return;
    }

    int tx = convertToInt(re.cap(2));
    int ty = convertToInt(re.cap(3));
    int tw = convertToInt(re.cap(4));
    int th = convertToInt(re.cap(5));

    int sx = convertToInt(re.cap(6));
    int sy = convertToInt(re.cap(7));
    int sw = convertToInt(re.cap(8));
    int sh = convertToInt(re.cap(9));

    if (tw == 0) tw = -1;
    if (th == 0) th = -1;
    if (sw == 0) sw = -1;
    if (sh == 0) sh = -1;

    if (m_verboseMode)
        printf(" - drawPixmap('%s' dim=(%d, %d), depth=%d, (%d, %d, %d, %d), (%d, %d, %d, %d)\n",
               re.cap(1).latin1(), pm.width(), pm.height(), pm.depth(),
               tx, ty, tw, th, sx, sy, sw, sh);


    m_painter->drawPixmap(QRect(tx, ty, tw, th), pm, QRect(sx, sy, sw, sh));
}

/***************************************************************************************************/
void PaintCommands::command_drawImage(QRegExp re)
{
    QImage im;
    im = m_imageMap[re.cap(1)]; // try cache first
    if (im.isNull())
        im = image_load<QImage>(re.cap(1));

    if (im.isNull()) {
        QFileInfo fi(m_filepath);
        QDir dir = fi.absoluteDir();
        dir.cdUp();
        dir.cd("images");
        QString fileName = QString("%1/%2").arg(dir.absolutePath()).arg(re.cap(1));
        im = QImage(fileName);
        if (im.isNull() && !fileName.endsWith(".png")) {
            fileName.append(".png");
            im = QImage(fileName);
        }
    }
    if (im.isNull()) {
        fprintf(stderr, "ERROR(drawImage): failed to load image: '%s'\n", qPrintable(re.cap(1)));
        return;
    }

    int tx = convertToInt(re.cap(2));
    int ty = convertToInt(re.cap(3));
    int tw = convertToInt(re.cap(4));
    int th = convertToInt(re.cap(5));

    int sx = convertToInt(re.cap(6));
    int sy = convertToInt(re.cap(7));
    int sw = convertToInt(re.cap(8));
    int sh = convertToInt(re.cap(9));

    if (tw == 0) tw = -1;
    if (th == 0) th = -1;
    if (sw == 0) sw = -1;
    if (sh == 0) sh = -1;

    if (m_verboseMode)
        printf(" - drawImage('%s' dim=(%d, %d), (%d, %d, %d, %d), (%d, %d, %d, %d)\n",
               re.cap(1).latin1(), im.width(), im.height(), tx, ty, tw, th, sx, sy, sw, sh);


    m_painter->drawImage(QRect(tx, ty, tw, th), im, QRect(sx, sy, sw, sh), Qt::OrderedDither | Qt::OrderedAlphaDither);
}

/***************************************************************************************************/
void PaintCommands::command_drawTiledPixmap(QRegExp re)
{
    QPixmap pm;
    pm = m_pixmapMap[re.cap(1)]; // try cache first
    if (pm.isNull())
        pm = image_load<QPixmap>(re.cap(1));
    if (pm.isNull()) {
        QFileInfo fi(m_filepath);
        QDir dir = fi.absoluteDir();
        dir.cdUp();
        dir.cd("images");
        QString fileName = QString("%1/%2").arg(dir.absolutePath()).arg(re.cap(1));
        pm = QPixmap(fileName);
        if (pm.isNull() && !fileName.endsWith(".png")) {
            fileName.append(".png");
            pm = QPixmap(fileName);
        }
    }
    if (pm.isNull()) {
        fprintf(stderr, "ERROR(drawTiledPixmap): failed to load pixmap: '%s'\n",
                qPrintable(re.cap(1)));
        return;
    }

    int tx = convertToInt(re.cap(2));
    int ty = convertToInt(re.cap(3));
    int tw = convertToInt(re.cap(4));
    int th = convertToInt(re.cap(5));

    int sx = convertToInt(re.cap(6));
    int sy = convertToInt(re.cap(7));

    if (tw == 0) tw = -1;
    if (th == 0) th = -1;

    if (m_verboseMode)
        printf(" - drawTiledPixmap('%s' dim=(%d, %d), (%d, %d, %d, %d), (%d, %d)\n",
               re.cap(1).latin1(), pm.width(), pm.height(), tx, ty, tw, th, sx, sy);

    m_painter->drawTiledPixmap(tx, ty, tw, th, pm, sx, sy);
}

/***************************************************************************************************/
void PaintCommands::command_drawPoint(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    float x = convertToFloat(caps.at(1));
    float y = convertToFloat(caps.at(2));

    if (m_verboseMode)
        printf(" - drawPoint(%.2f, %.2f)\n", x, y);

    m_painter->drawPoint(QPointF(x, y));
}

/***************************************************************************************************/
void PaintCommands::command_drawPolygon(QRegExp re)
{
    static QRegExp separators("\\s");
    QStringList caps = re.capturedTexts();
    QString cap = caps.at(1);
    QStringList numbers = cap.split(separators, QString::SkipEmptyParts);

    QPolygonF array;
    for (int i=0; i + 1<numbers.size(); i+=2)
        array.append(QPointF(convertToDouble(numbers.at(i)), convertToDouble(numbers.at(i+1))));

    if (m_verboseMode)
        printf(" - drawPolygon(size=%d)\n", array.size());


    m_painter->drawPolygon(array, caps.at(2).toLower() == "winding" ? Qt::WindingFill : Qt::OddEvenFill);
}

/***************************************************************************************************/
void PaintCommands::command_drawPolyline(QRegExp re)
{
    static QRegExp separators("\\s");
    QStringList numbers = re.cap(1).split(separators, QString::SkipEmptyParts);

    QPolygonF array;
    for (int i=0; i + 1<numbers.size(); i+=2)
        array.append(QPointF(numbers.at(i).toFloat(),numbers.at(i+1).toFloat()));

    if (m_verboseMode)
        printf(" - drawPolyline(size=%d)\n", array.size());


    m_painter->drawPolyline(array.toPolygon());
}

/***************************************************************************************************/
void PaintCommands::command_drawRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    float x = convertToFloat(caps.at(1));
    float y = convertToFloat(caps.at(2));
    float w = convertToFloat(caps.at(3));
    float h = convertToFloat(caps.at(4));

    if (m_verboseMode)
        printf(" - drawRect(%.2f, %.2f, %.2f, %.2f)\n", x, y, w, h);


    m_painter->drawRect(QRectF(x, y, w, h));
}

/***************************************************************************************************/
void PaintCommands::command_drawRoundRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int xs = caps.at(5).isEmpty() ? 50 : convertToInt(caps.at(5));
    int ys = caps.at(6).isEmpty() ? 50 : convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - drawRoundRect(%d, %d, %d, %d, [%d, %d])\n", x, y, w, h, xs, ys);


    m_painter->drawRoundRect(x, y, w, h, xs, ys);
}

/***************************************************************************************************/
void PaintCommands::command_drawEllipse(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    float x = convertToFloat(caps.at(1));
    float y = convertToFloat(caps.at(2));
    float w = convertToFloat(caps.at(3));
    float h = convertToFloat(caps.at(4));

    if (m_verboseMode)
        printf(" - drawEllipse(%.2f, %.2f, %.2f, %.2f)\n", x, y, w, h);


    m_painter->drawEllipse(QRectF(x, y, w, h));
}

/***************************************************************************************************/
void PaintCommands::command_drawPie(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - drawPie(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    m_painter->drawPie(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_drawChord(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - drawChord(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    m_painter->drawChord(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_drawArc(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - drawArc(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    m_painter->drawArc(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));

    if (m_verboseMode)
        printf(" - qt3_drawRect(%d, %d, %d, %d)\n", x, y, w, h);


    static_cast<Q3Painter*>(m_painter)->drawRect(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawRoundRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int xrnd = caps.at(5).isEmpty() ? 25 : convertToInt(caps.at(5));
    int yrnd = caps.at(6).isEmpty() ? 25 : convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - qt3_drawRoundRect(%d, %d, %d, %d), %d, %d\n", x, y, w, h, xrnd, yrnd);


    static_cast<Q3Painter*>(m_painter)->drawRoundRect(x, y, w, h, xrnd, yrnd);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawEllipse(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));

    if (m_verboseMode)
        printf(" - qt3_drawEllipse(%d, %d, %d, %d)\n", x, y, w, h);


    static_cast<Q3Painter*>(m_painter)->drawEllipse(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawPie(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - qt3_drawPie(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    static_cast<Q3Painter*>(m_painter)->drawPie(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawChord(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - qt3_drawChord(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    static_cast<Q3Painter*>(m_painter)->drawChord(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_qt3_drawArc(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));
    int angle = convertToInt(caps.at(5));
    int sweep = convertToInt(caps.at(6));

    if (m_verboseMode)
        printf(" - qt3_drawArc(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, angle, sweep);


    static_cast<Q3Painter*>(m_painter)->drawArc(x, y, w, h, angle, sweep);
}

/***************************************************************************************************/
void PaintCommands::command_drawText(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    QString txt = caps.at(3);

    if (m_verboseMode)
        printf(" - drawText(%d, %d, %s)\n", x, y, txt.latin1());


    m_painter->drawText(x, y, txt);
}

/***************************************************************************************************/
void PaintCommands::command_noop(QRegExp)
{
    if (m_verboseMode)
        printf(" - noop: %s\n", m_currentCommand.latin1());

    if (!m_currentCommand.trimmed().isEmpty()) {
        fprintf(stderr, "unknown command: '%s'\n", m_currentCommand.trimmed().latin1());
    }
}

/***************************************************************************************************/
void PaintCommands::command_path_addText(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x = convertToDouble(caps.at(2));
    double y = convertToDouble(caps.at(3));
    QString text = caps.at(4);

    if (m_verboseMode)
        printf(" - path_addText(%s, %.2f, %.2f, text=%s\n", name.latin1(), x, y, text.latin1());

    m_pathMap[name].addText(x, y, m_painter->font(), text);
}

/***************************************************************************************************/
void PaintCommands::command_path_addEllipse(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x = convertToDouble(caps.at(2));
    double y = convertToDouble(caps.at(3));
    double w = convertToDouble(caps.at(4));
    double h = convertToDouble(caps.at(5));

    if (m_verboseMode)
        printf(" - path_addEllipse(%s, %.2f, %.2f, %.2f, %.2f)\n", name.latin1(), x, y, w, h);


    m_pathMap[name].addEllipse(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_path_addRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x = convertToDouble(caps.at(2));
    double y = convertToDouble(caps.at(3));
    double w = convertToDouble(caps.at(4));
    double h = convertToDouble(caps.at(5));

    if (m_verboseMode)
        printf(" - path_addRect(%s, %.2f, %.2f, %.2f, %.2f)\n", name.latin1(), x, y, w, h);


    m_pathMap[name].addRect(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_path_addPolygon(QRegExp re)
{
    static QRegExp separators("\\s");
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    QString cap = caps.at(2);
    QStringList numbers = cap.split(separators, QString::SkipEmptyParts);

    QPolygonF array;
    for (int i=0; i + 1<numbers.size(); i+=2)
        array.append(QPointF(numbers.at(i).toFloat(),numbers.at(i+1).toFloat()));

    if (m_verboseMode)
        printf(" - path_addPolygon(name=%s, size=%d)\n", name.latin1(), array.size());


    m_pathMap[name].addPolygon(array);
}

/***************************************************************************************************/
void PaintCommands::command_path_arcTo(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x = convertToDouble(caps.at(2));
    double y = convertToDouble(caps.at(3));
    double w = convertToDouble(caps.at(4));
    double h = convertToDouble(caps.at(5));
    double angle = convertToDouble(caps.at(6));
    double length = convertToDouble(caps.at(7));

    if (m_verboseMode)
        printf(" - path_arcTo(%s, %.2f, %.2f, %.2f, %.2f, angle=%.2f, len=%.2f)\n", name.latin1(), x, y, w, h, angle, length);


    m_pathMap[name].arcTo(x, y, w, h, angle, length);
}

/***************************************************************************************************/
void PaintCommands::command_path_createOutline(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    QString newName = caps.at(2);
    QPen pen = m_painter->pen();

    if (m_verboseMode)
        printf(" - path_createOutline(%s, name=%s, width=%d)\n",
               name.latin1(), newName.latin1(), pen.width());

    if (!m_pathMap.contains(name)) {
        fprintf(stderr, "createOutline(), unknown path: %s\n", name.latin1());
        return;
    }
    QPainterPathStroker stroker;
    stroker.setWidth(pen.widthF());
    stroker.setDashPattern(pen.style());
    stroker.setCapStyle(pen.capStyle());
    stroker.setJoinStyle(pen.joinStyle());
    m_pathMap[newName] = stroker.createStroke(m_pathMap[name]);
}

/***************************************************************************************************/
void PaintCommands::command_path_cubicTo(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x1 = convertToDouble(caps.at(2));
    double y1 = convertToDouble(caps.at(3));
    double x2 = convertToDouble(caps.at(4));
    double y2 = convertToDouble(caps.at(5));
    double x3 = convertToDouble(caps.at(6));
    double y3 = convertToDouble(caps.at(7));

    if (m_verboseMode)
        printf(" - path_cubicTo(%s, (%.2f, %.2f), (%.2f, %.2f), (%.2f, %.2f))\n", name.latin1(), x1, y1, x2, y2, x3, y3);


    m_pathMap[name].cubicTo(x1, y1, x2, y2, x3, y3);
}

/***************************************************************************************************/
void PaintCommands::command_path_moveTo(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x1 = convertToDouble(caps.at(2));
    double y1 = convertToDouble(caps.at(3));

    if (m_verboseMode)
        printf(" - path_moveTo(%s, (%.2f, %.2f))\n", name.latin1(), x1, y1);

    m_pathMap[name].moveTo(x1, y1);
}

/***************************************************************************************************/
void PaintCommands::command_path_lineTo(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    double x1 = convertToDouble(caps.at(2));
    double y1 = convertToDouble(caps.at(3));

    if (m_verboseMode)
        printf(" - path_lineTo(%s, (%.2f, %.2f))\n", name.latin1(), x1, y1);


    m_pathMap[name].lineTo(x1, y1);
}

/***************************************************************************************************/
void PaintCommands::command_path_setFillRule(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    bool winding = caps.at(2).toLower() == "winding";

    if (m_verboseMode)
        printf(" - path_setFillRule(name=%s, winding=%d)\n", name.latin1(), winding);

    m_pathMap[name].setFillRule(winding ? Qt::WindingFill : Qt::OddEvenFill);
}

/***************************************************************************************************/
void PaintCommands::command_path_closeSubpath(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);

    if (m_verboseMode)
        printf(" - path_closeSubpath(name=%s)\n", name.latin1());

    m_pathMap[name].closeSubpath();
}

/***************************************************************************************************/
void PaintCommands::command_path_getClipPath(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);

    if (m_verboseMode)
        printf(" - path_closeSubpath(name=%s)\n", name.latin1());

    m_pathMap[name] = m_painter->clipPath();
}

/***************************************************************************************************/
static void qt_debug_path(const QPainterPath &path, const QString &name)
{
    const char *names[] = {
        "MoveTo     ",
        "LineTo     ",
        "CurveTo    ",
        "CurveToData"
    };

    printf("\nQPainterPath (%s): elementCount=%d\n", qPrintable(name), path.elementCount());
    for (int i=0; i<path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        Q_ASSERT(e.type >= 0 && e.type <= QPainterPath::CurveToDataElement);
        printf(" - %3d:: %s, (%.2f, %.2f)\n", i, names[e.type], e.x, e.y);
    }
}

/***************************************************************************************************/
void PaintCommands::command_path_debugPrint(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    qt_debug_path(m_pathMap[name], name);
}

/***************************************************************************************************/
void PaintCommands::command_region_addRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    int x = convertToInt(caps.at(2));
    int y = convertToInt(caps.at(3));
    int w = convertToInt(caps.at(4));
    int h = convertToInt(caps.at(5));

    if (m_verboseMode)
        printf(" - region_addRect(%s, %d, %d, %d, %d)\n", name.latin1(), x, y, w, h);


    m_regionMap[name] += QRect(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_region_addEllipse(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    int x = convertToInt(caps.at(2));
    int y = convertToInt(caps.at(3));
    int w = convertToInt(caps.at(4));
    int h = convertToInt(caps.at(5));

    if (m_verboseMode)
        printf(" - region_addEllipse(%s, %d, %d, %d, %d)\n", name.latin1(), x, y, w, h);


    m_regionMap[name] += QRegion(x, y, w, h, QRegion::Ellipse);
}

/***************************************************************************************************/
void PaintCommands::command_region_getClipRegion(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString name = caps.at(1);
    QRegion region = m_painter->clipRegion();

    if (m_verboseMode)
        printf(" - region_getClipRegion(name=%s), bounds=[%d, %d, %d, %d]\n", name.latin1(),
               region.boundingRect().x(),
               region.boundingRect().y(),
               region.boundingRect().width(),
               region.boundingRect().height());

    m_regionMap[name] = region;
}

/***************************************************************************************************/
void PaintCommands::command_resetMatrix(QRegExp)
{
    if (m_verboseMode)
        printf(" - resetMatrix()\n");

    m_painter->resetTransform();
}

/***************************************************************************************************/
void PaintCommands::command_restore(QRegExp)
{
    if (m_verboseMode)
        printf(" - restore()\n");


    m_painter->restore();
}

/***************************************************************************************************/
void PaintCommands::command_rotate(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int angle = convertToInt(caps.at(1));

    if (m_verboseMode)
        printf(" - rotate(%d)\n", angle);


    m_painter->rotate(angle);
}

/***************************************************************************************************/
void PaintCommands::command_save(QRegExp)
{
    if (m_verboseMode)
        printf(" - save()\n");


    m_painter->save();
}

/***************************************************************************************************/
void PaintCommands::command_mapQuadToQuad(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double x1 = convertToDouble(caps.at(1));
    double y1 = convertToDouble(caps.at(2));
    double x2 = convertToDouble(caps.at(3));
    double y2 = convertToDouble(caps.at(4));
    double x3 = convertToDouble(caps.at(5));
    double y3 = convertToDouble(caps.at(6));
    double x4 = convertToDouble(caps.at(7));
    double y4 = convertToDouble(caps.at(8));
    QPolygonF poly1(4);
    poly1[0] = QPointF(x1, y1);
    poly1[1] = QPointF(x2, y2);
    poly1[2] = QPointF(x3, y3);
    poly1[3] = QPointF(x4, y4);

    double x5 = convertToDouble(caps.at(9));
    double y5 = convertToDouble(caps.at(10));
    double x6 = convertToDouble(caps.at(11));
    double y6 = convertToDouble(caps.at(12));
    double x7 = convertToDouble(caps.at(13));
    double y7 = convertToDouble(caps.at(14));
    double x8 = convertToDouble(caps.at(15));
    double y8 = convertToDouble(caps.at(16));
    QPolygonF poly2(4);
    poly2[0] = QPointF(x5, y5);
    poly2[1] = QPointF(x6, y6);
    poly2[2] = QPointF(x7, y7);
    poly2[3] = QPointF(x8, y8);

    if (m_verboseMode)
        printf(" - mapQuadToQuad(%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f ->\n\t"
               ",%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)\n",
               x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, x7, y7, x8, y8);

    QTransform trans;

    if (!QTransform::quadToQuad(poly1, poly2, trans)) {
        qWarning("Couldn't perform quad to quad transformation!");
    }

    m_painter->setTransform(trans, true);
}

/***************************************************************************************************/
void PaintCommands::command_setMatrix(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double m11 = convertToDouble(caps.at(1));
    double m12 = convertToDouble(caps.at(2));
    double m13 = convertToDouble(caps.at(3));
    double m21 = convertToDouble(caps.at(4));
    double m22 = convertToDouble(caps.at(5));
    double m23 = convertToDouble(caps.at(6));
    double m31 = convertToDouble(caps.at(7));
    double m32 = convertToDouble(caps.at(8));
    double m33 = convertToDouble(caps.at(9));

    if (m_verboseMode)
        printf(" - setMatrix(%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)\n",
               m11, m12, m13, m21, m22, m23, m31, m32, m33);

    QTransform trans;
    trans.setMatrix(m11, m12, m13,
                    m21, m22, m23,
                    m31, m32, m33);

    m_painter->setTransform(trans, true);
}

/***************************************************************************************************/
void PaintCommands::command_scale(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double sx = convertToDouble(caps.at(1));
    double sy = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - scale(%.2f, %.2f)\n", sx, sy);


    m_painter->scale(sx, sy);
}

/***************************************************************************************************/
void PaintCommands::command_setBackground(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QColor color = convertToColor(caps.at(1));
    QString pattern = caps.at(2);

    int style = translateEnum(brushStyleTable, pattern, Qt::LinearGradientPattern);
    if (style < 0)
        style = Qt::SolidPattern;

    if (m_verboseMode)
        printf(" - setBackground(%s, %s)\n", color.name().latin1(), pattern.latin1());

    m_painter->setBackground(QBrush(color, Qt::BrushStyle(style)));
}

/***************************************************************************************************/
void PaintCommands::command_setOpacity(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double opacity = convertToDouble(caps.at(1));

    if (m_verboseMode)
        printf(" - setOpacity(%lf)\n", opacity);

    m_painter->setOpacity(opacity);
}

/***************************************************************************************************/
void PaintCommands::command_setBgMode(QRegExp re)
{
    QString cap = re.cap(1);
    Qt::BGMode mode = Qt::TransparentMode;
    if (cap == "OpaqueMode" || cap == "Opaque")
        mode = Qt::OpaqueMode;

    if (m_verboseMode)
        printf(" - setBackgroundMode(%s)\n", mode == Qt::OpaqueMode ? "OpaqueMode" : "TransparentMode");


    m_painter->setBackgroundMode(mode);
}

/***************************************************************************************************/
void PaintCommands::command_setBrush(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QPixmap pm = image_load<QPixmap>(caps.at(1));
    if (!pm.isNull()) { // Assume pixmap
        if (m_verboseMode)
            printf(" - setBrush(pixmap=%s, width=%d, height=%d)\n",
                   caps.at(1).latin1(), pm.width(), pm.height());

        m_painter->setBrush(QBrush(pm));
    } else if (caps.at(1).toLower() == "nobrush") {
        m_painter->setBrush(Qt::NoBrush);
        if (m_verboseMode)
            printf(" - setBrush(Qt::NoBrush)\n");
    } else {
        QColor color = convertToColor(caps.at(1));
        QString pattern = caps.at(2);

        int style = translateEnum(brushStyleTable, pattern, Qt::LinearGradientPattern);
        if (style < 0)
            style = Qt::SolidPattern;

        if (m_verboseMode)
            printf(" - setBrush(%s, %s (%d))\n", color.name().latin1(), pattern.latin1(), style);

        m_painter->setBrush(QBrush(color, Qt::BrushStyle(style)));
    }
}

/***************************************************************************************************/
void PaintCommands::command_setBrushOrigin(QRegExp re)
{
    int x = convertToInt(re.cap(1));
    int y = convertToInt(re.cap(2));

    if (m_verboseMode)
        printf(" - setBrushOrigin(%d, %d)\n", x, y);

    m_painter->setBrushOrigin(x, y);
}

/***************************************************************************************************/
void PaintCommands::command_brushTranslate(QRegExp re)
{
#if QT_VERSION > 0x040200
    QStringList caps = re.capturedTexts();
    double dx = convertToDouble(caps.at(1));
    double dy = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - brushTranslate(%f, %f)\n", dx, dy);

    QBrush new_brush = m_painter->brush();
    QTransform brush_matrix = new_brush.transform();
    brush_matrix.translate(dx, dy);
    new_brush.setTransform(brush_matrix);
    m_painter->setBrush(new_brush);
#endif
}

/***************************************************************************************************/
void PaintCommands::command_brushScale(QRegExp re)
{
#if QT_VERSION > 0x040200
    QStringList caps = re.capturedTexts();
    double sx = convertToDouble(caps.at(1));
    double sy = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - brushScale(%f, %f)\n", sx, sy);

    QBrush new_brush = m_painter->brush();
    QTransform brush_matrix = new_brush.transform();
    brush_matrix.scale(sx, sy);
    new_brush.setTransform(brush_matrix);
    m_painter->setBrush(new_brush);
#endif
}

/***************************************************************************************************/
void PaintCommands::command_brushRotate(QRegExp re)
{
#if QT_VERSION > 0x040200
    QStringList caps = re.capturedTexts();
    double rot = convertToDouble(caps.at(1));

    if (m_verboseMode)
        printf(" - brushScale(%f)\n", rot);

    QBrush new_brush = m_painter->brush();
    QTransform brush_matrix = new_brush.transform();
    brush_matrix.rotate(rot);
    new_brush.setTransform(brush_matrix);
    m_painter->setBrush(new_brush);
#endif
}

/***************************************************************************************************/
void PaintCommands::command_brushShear(QRegExp re)
{
#if QT_VERSION > 0x040200
    QStringList caps = re.capturedTexts();
    double sx = convertToDouble(caps.at(1));
    double sy = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - brushShear(%f, %f)\n", sx, sy);

    QBrush new_brush = m_painter->brush();
    QTransform brush_matrix = new_brush.transform();
    brush_matrix.shear(sx, sy);
    new_brush.setTransform(brush_matrix);
    m_painter->setBrush(new_brush);
#endif
}

/***************************************************************************************************/
void PaintCommands::command_setClipping(QRegExp re)
{
    bool clipping = re.cap(1).toLower() == "true";

    if (m_verboseMode)
        printf(" - setClipping(%d)\n", clipping);


    m_painter->setClipping(clipping);
}

/***************************************************************************************************/
void PaintCommands::command_setClipRect(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    int x = convertToInt(caps.at(1));
    int y = convertToInt(caps.at(2));
    int w = convertToInt(caps.at(3));
    int h = convertToInt(caps.at(4));

    int combine = translateEnum(clipOperationTable, caps.at(5), Qt::UniteClip + 1);
    if (combine == -1)
        combine = Qt::ReplaceClip;

    if (m_verboseMode)
        printf(" - setClipRect(%d, %d, %d, %d), %s\n", x, y, w, h, clipOperationTable[combine]);


    m_painter->setClipRect(x, y, w, h, Qt::ClipOperation(combine));
}

/***************************************************************************************************/
void PaintCommands::command_setClipPath(QRegExp re)
{
    int combine = translateEnum(clipOperationTable, re.cap(2), Qt::UniteClip + 1);
    if (combine == -1)
        combine = Qt::ReplaceClip;

    if (m_verboseMode)
        printf(" - setClipPath(name=%s), %s\n", re.cap(1).latin1(), clipOperationTable[combine]);

    if (!m_pathMap.contains(re.cap(1)))
        fprintf(stderr, " - setClipPath, no such path");
    m_painter->setClipPath(m_pathMap[re.cap(1)], Qt::ClipOperation(combine));
}

/***************************************************************************************************/
void PaintCommands::command_setClipRegion(QRegExp re)
{
    int combine = translateEnum(clipOperationTable, re.cap(2), Qt::UniteClip + 1);
    if (combine == -1)
        combine = Qt::ReplaceClip;
    QRegion r = m_regionMap[re.cap(1)];

    if (m_verboseMode)
        printf(" - setClipRegion(name=%s), bounds=[%d, %d, %d, %d], %s\n",
               re.cap(1).latin1(),
               r.boundingRect().x(),
               r.boundingRect().y(),
               r.boundingRect().width(),
               r.boundingRect().height(),
               clipOperationTable[combine]);

    m_painter->setClipRegion(m_regionMap[re.cap(1)], Qt::ClipOperation(combine));
}

/***************************************************************************************************/
void PaintCommands::command_setFont(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QString family = caps.at(1);
    int size = convertToInt(caps.at(2));
    int weight = caps.at(3).toLower() == "bold" ? QFont::Bold : QFont::Normal;
    bool italic = caps.at(4).toLower() == "true" || caps.at(4).toLower() == "italic";

    if (m_verboseMode)
        printf(" - setFont(family=%s, size=%d, weight=%d, italic=%d\n",
               family.latin1(), size, weight, italic);

    m_painter->setFont(QFont(family, size, weight, italic));
}

/***************************************************************************************************/
void PaintCommands::command_setPen(QRegExp re)
{
    QString cap = re.cap(1);
    int style = translateEnum(penStyleTable, cap, Qt::DashDotDotLine + 1);
    if (style >= 0) {
        if (m_verboseMode)
            printf(" - setPen(%s)\n", cap.latin1());

        m_painter->setPen(Qt::PenStyle(style));
    } else if (cap.toLower() == "brush") {
        QPen pen(m_painter->brush(), 0);
        if (m_verboseMode) {
            printf(" - setPen(brush), style=%d, color=%08x\n",
                   pen.brush().style(), pen.color().rgba());
        }
        m_painter->setPen(pen);
    } else {
        QColor color = convertToColor(cap);
        if (m_verboseMode)
            printf(" - setPen(%s)\n", color.name().latin1());

        m_painter->setPen(color);
    }
}

/***************************************************************************************************/
void PaintCommands::command_setPen2(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QBrush brush;

    if (caps.at(1).toLower() == "brush")
        brush = m_painter->brush();
    else
        brush = convertToColor(caps.at(1));

    double width = convertToDouble(caps.at(2));
    int penStyle = translateEnum(penStyleTable, caps.at(3), Qt::DashDotDotLine + 1);
    if (penStyle < 0)
        penStyle = Qt::SolidLine;

    Qt::PenCapStyle capStyle = Qt::SquareCap;
    if (caps.at(4).toLower() == "flatcap") capStyle = Qt::FlatCap;
    else if (caps.at(4).toLower() == "squarecap") capStyle = Qt::SquareCap;
    else if (caps.at(4).toLower() == "roundcap") capStyle = Qt::RoundCap;
    else if (!caps.at(4).isEmpty())
        fprintf(stderr, "ERROR: setPen, unknown capStyle: %s\n", caps.at(4).latin1());

    Qt::PenJoinStyle joinStyle = Qt::BevelJoin;
    if (caps.at(5).toLower() == "miterjoin") joinStyle = Qt::MiterJoin;
    else if (caps.at(5).toLower() == "beveljoin") joinStyle = Qt::BevelJoin;
    else if (caps.at(5).toLower() == "roundjoin") joinStyle = Qt::RoundJoin;
    else if (!caps.at(5).isEmpty())
        fprintf(stderr, "ERROR: setPen, unknown joinStyle: %s\n", caps.at(5).latin1());

    if (m_verboseMode)
        printf(" - setPen(%s, width=%f, style=%d, cap=%d, join=%d)\n",
               brush.color().name().latin1(), width, penStyle, capStyle, joinStyle);


    m_painter->setPen(QPen(brush, width, Qt::PenStyle(penStyle), capStyle, joinStyle));
}

/***************************************************************************************************/
void PaintCommands::command_setRenderHint(QRegExp re)
{
    QString hintString = re.cap(1).toLower();
    bool on = re.cap(2).isEmpty() || re.cap(2).toLower() == "true";
    if (hintString.contains("antialiasing")) {
        if (m_verboseMode)
            printf(" - setRenderHint Antialiasing\n");

        m_painter->setRenderHint(QPainter::Antialiasing, on);
    } else if (hintString.contains("smoothpixmaptransform")) {
        if (m_verboseMode)
            printf(" - setRenderHint SmoothPixmapTransform\n");
        m_painter->setRenderHint(QPainter::SmoothPixmapTransform, on);
    } else {
        fprintf(stderr, "ERROR(setRenderHint): unknown hint '%s'\n", hintString.latin1());
    }
}

/***************************************************************************************************/
void PaintCommands::command_clearRenderHint(QRegExp re)
{
    m_painter->setRenderHint(QPainter::Antialiasing, false);
    m_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (m_verboseMode)
        printf(" - clearRenderHint\n");
}

/***************************************************************************************************/
void PaintCommands::command_setCompositionMode(QRegExp re)
{
    QString modeString = re.cap(1).toLower();
    int mode = translateEnum(compositionModeTable, modeString, 12);

    if (mode < 0 || mode > QPainter::CompositionMode_Xor) {
        fprintf(stderr, "ERROR: invalid mode: %s\n", qPrintable(modeString));
        return;
    }

    if (m_verboseMode)
        printf(" - setCompositionMode: %d: %s\n", mode, qPrintable(modeString));

    m_painter->setCompositionMode(QPainter::CompositionMode(mode));
}

/***************************************************************************************************/
void PaintCommands::command_translate(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double dx = convertToDouble(caps.at(1));
    double dy = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - translate(%f, %f)\n", dx, dy);

    m_painter->translate(dx, dy);
}

/***************************************************************************************************/
void PaintCommands::command_pixmap_load(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString fileName = caps.at(1);
    QString name = caps.at(2);

    if (name.isEmpty())
        name = fileName;

    QImage im = image_load<QImage>(fileName);
    QPixmap px = QPixmap::fromImage(im, Qt::OrderedDither | Qt::OrderedAlphaDither);

    if (m_verboseMode)
        printf(" - pixmap_load(%s as %s), size=[%d, %d], depth=%d\n",
               qPrintable(fileName), qPrintable(name),
               px.width(), px.height(), px.depth());

    m_pixmapMap[name] = px;
}

/***************************************************************************************************/
void PaintCommands::command_bitmap_load(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString fileName = caps.at(1);
    QString name = caps.at(2);

    if (name.isEmpty())
        name = fileName;

    QBitmap bm = image_load<QBitmap>(fileName);

    if (m_verboseMode)
        printf(" - bitmap_load(%s as %s), size=[%d, %d], depth=%d\n",
               qPrintable(fileName), qPrintable(name),
               bm.width(), bm.height(), bm.depth());

    m_pixmapMap[name] = bm;
}

/***************************************************************************************************/
void PaintCommands::command_pixmap_setMask(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    QBitmap mask = image_load<QBitmap>(caps.at(2));

    if (m_verboseMode)
        printf(" - pixmap_setMask(%s, %s)\n", caps.at(1).latin1(), caps.at(2).latin1());

    if (!m_pixmapMap[caps.at(1)].isNull())
        m_pixmapMap[caps.at(1)].setMask(mask);
}

/***************************************************************************************************/
void PaintCommands::command_image_load(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString fileName = caps.at(1);
    QString name = caps.at(2);

    if (name.isEmpty())
        name = fileName;

    QImage image = image_load<QImage>(fileName);

    if (m_verboseMode)
        printf(" - image_load(%s as %s), size=[%d, %d], format=%d\n",
               qPrintable(fileName), qPrintable(name),
               image.width(), image.height(), image.format());

    m_imageMap[name] = image;
}

/***************************************************************************************************/
void PaintCommands::command_image_setNumColors(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString name = caps.at(1);
    int count = convertToInt(caps.at(2));

    if (m_verboseMode)
        printf(" - image_setNumColors(%s), %d -> %d\n",
               qPrintable(name), m_imageMap[name].numColors(), count);

    m_imageMap[name].setNumColors(count);
}

/***************************************************************************************************/
void PaintCommands::command_image_setColor(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString name = caps.at(1);
    int index = convertToInt(caps.at(2));
    QColor color = convertToColor(caps.at(3));

    if (m_verboseMode)
        printf(" - image_setColor(%s), %d = %08x\n", qPrintable(name), index, color.rgba());

    m_imageMap[name].setColor(index, color.rgba());
}

/***************************************************************************************************/
void PaintCommands::command_abort(QRegExp)
{
    m_abort = true;
}

/***************************************************************************************************/
void PaintCommands::command_gradient_clearStops(QRegExp)
{
    if (m_verboseMode)
        printf(" - gradient_clearStops\n");
    m_gradientStops.clear();
}

/***************************************************************************************************/
void PaintCommands::command_gradient_appendStop(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double pos = convertToDouble(caps.at(1));
    QColor color = convertToColor(caps.at(2));

    if (m_verboseMode)
        printf(" - gradient_appendStop(%.2f, %x)\n", pos, color.rgba());

    m_gradientStops << QGradientStop(pos, color);
}

/***************************************************************************************************/
void PaintCommands::command_gradient_setLinear(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double x1 = convertToDouble(caps.at(1));
    double y1 = convertToDouble(caps.at(2));
    double x2 = convertToDouble(caps.at(3));
    double y2 = convertToDouble(caps.at(4));

    if (m_verboseMode) {
        printf(" - gradient_setLinear (%.2f, %.2f), (%.2f, %.2f), spread=%d\n",
               x1, y1, x2, y2, m_gradientSpread);
    }

    QLinearGradient lg(QPointF(x1, y1), QPointF(x2, y2));
    lg.setStops(m_gradientStops);
    lg.setSpread(m_gradientSpread);
    QBrush brush(lg);
#if QT_VERSION > 0x040200
    QTransform brush_matrix = m_painter->brush().transform();
    brush.setTransform(brush_matrix);
#endif
    m_painter->setBrush(brush);
}

/***************************************************************************************************/
void PaintCommands::command_gradient_setRadial(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double cx = convertToDouble(caps.at(1));
    double cy = convertToDouble(caps.at(2));
    double rad = convertToDouble(caps.at(3));
    double fx = convertToDouble(caps.at(4));
    double fy = convertToDouble(caps.at(5));

    if (m_verboseMode) {
        printf(" - gradient_setRadial center=(%.2f, %.2f), radius=%.2f focal=(%.2f, %.2f), "
               "spread=%d\n",
               cx, cy, rad, fx, fy, m_gradientSpread);
    }

    QRadialGradient rg(QPointF(cx, cy), rad, QPointF(fx, fy));
    rg.setStops(m_gradientStops);
    rg.setSpread(m_gradientSpread);
    QBrush brush(rg);
#if QT_VERSION > 0x040200
    QTransform brush_matrix = m_painter->brush().transform();
    brush.setTransform(brush_matrix);
#endif
    m_painter->setBrush(brush);
}

/***************************************************************************************************/
void PaintCommands::command_gradient_setConical(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double cx = convertToDouble(caps.at(1));
    double cy = convertToDouble(caps.at(2));
    double angle = convertToDouble(caps.at(3));

    if (m_verboseMode) {
        printf(" - gradient_setConical center=(%.2f, %.2f), angle=%.2f\n, spread=%d",
               cx, cy, angle, m_gradientSpread);
    }

    QConicalGradient cg(QPointF(cx, cy), angle);
    cg.setStops(m_gradientStops);
    cg.setSpread(m_gradientSpread);
    QBrush brush(cg);
#if QT_VERSION > 0x040200
    QTransform brush_matrix = m_painter->brush().transform();
    brush.setTransform(brush_matrix);
#endif
    m_painter->setBrush(brush);
}

/***************************************************************************************************/
void PaintCommands::command_gradient_setSpread(QRegExp re)
{
    int spreadMethod = translateEnum(spreadMethodTable, re.cap(1), 3);

    if (m_verboseMode)
        printf(" - gradient_setSpread %d=[%s]\n", spreadMethod, spreadMethodTable[spreadMethod]);

    m_gradientSpread = QGradient::Spread(spreadMethod);
}

/***************************************************************************************************/
void PaintCommands::command_surface_begin(QRegExp re)
{
    QStringList caps = re.capturedTexts();
    double x = convertToDouble(caps.at(1));
    double y = convertToDouble(caps.at(2));
    double w = convertToDouble(caps.at(3));
    double h = convertToDouble(caps.at(4));

    if (m_surface_painter) {
        fprintf(stderr, "ERROR: surface already active");
        return;
    }

    if (m_verboseMode)
        printf(" - surface_begin, pos=[%.2f, %.2f], size=[%.2f, %.2f]\n", x, y, w, h);

    m_surface_painter = m_painter;

#if QT_VERSION > 0x040200
    if (m_type == OpenGLType || m_type == OpenGLPBufferType) {
#ifndef QT_NO_OPENGL
        m_surface_pbuffer = new QGLPixelBuffer(qRound(w), qRound(h));
        m_painter = new QPainter(m_surface_pbuffer);
        m_painter->fillRect(QRect(0, 0, qRound(w), qRound(h)), Qt::transparent);
#endif
#ifdef Q_WS_X11
    } else if (m_type == WidgetType) {
        m_surface_pixmap = QPixmap(qRound(w), qRound(h));
        m_surface_pixmap.fill(Qt::transparent);
        m_painter = new QPainter(&m_surface_pixmap);
#endif
    } else
#endif
        {
        m_surface_image = QImage(qRound(w), qRound(h), QImage::Format_ARGB32_Premultiplied);
        m_surface_image.fill(0);
        m_painter = new QPainter(&m_surface_image);
    }
    m_surface_rect = QRectF(x, y, w, h);
}

/***************************************************************************************************/
void PaintCommands::command_surface_end(QRegExp)
{
    if (!m_surface_painter) {
        fprintf(stderr, "ERROR: surface not active");
        return;
    }

    if (m_verboseMode)
        printf(" - surface_end, pos=[%.2f, %.2f], size=[%.2f, %.2f]\n",
               m_surface_rect.x(),
               m_surface_rect.y(),
               m_surface_rect.width(),
               m_surface_rect.height());
    m_painter->end();

    delete m_painter;
    m_painter = m_surface_painter;
    m_surface_painter = 0;

    if (m_type == OpenGLType || m_type == OpenGLPBufferType) {
#ifndef QT_NO_OPENGL
        QImage image = m_surface_pbuffer->toImage();
        QImage new_image(image.bits(), image.width(),
                         image.height(), QImage::Format_ARGB32_Premultiplied);
        QPaintDevice *pdev = m_painter->device();
        if (pdev->devType() == QInternal::Widget) {
            QWidget *w = static_cast<QWidget *>(pdev);
            static_cast<QGLWidget *>(w)->makeCurrent();
        } else if (pdev->devType() == QInternal::Pbuffer) {
            static_cast<QGLPixelBuffer *>(pdev)->makeCurrent();
        }

        m_painter->drawImage(m_surface_rect, new_image);

        delete m_surface_pbuffer;
        m_surface_pbuffer = 0;
#endif
#ifdef Q_WS_X11
    } else if (m_type == WidgetType) {
        m_painter->drawPixmap(m_surface_rect.topLeft(), m_surface_pixmap);
        m_surface_pixmap = QPixmap();
#endif
    } else {
        m_painter->drawImage(m_surface_rect, m_surface_image);
        m_surface_image = QImage();
    }
    m_surface_rect = QRectF();
}

/***************************************************************************************************/
void PaintCommands::command_image_convertToFormat(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString srcName = caps.at(1);
    QString destName = caps.at(2);

    if (!m_imageMap.contains(srcName)) {
        fprintf(stderr, "ERROR(convertToFormat): no such image '%s'\n", qPrintable(srcName));
        return;
    }

    int format = translateEnum(imageFormatTable, caps.at(3),
                                QImage::Format_ARGB32_Premultiplied + 1);
    if (format < 0 || format > QImage::Format_ARGB32_Premultiplied) {
        fprintf(stderr, "ERROR(convertToFormat): invalid format %d = '%s'\n",
                format, qPrintable(caps.at(3)));
        return;
    }

    QImage src = m_imageMap[srcName];
    QImage dest = src.convertToFormat(QImage::Format(format),
                                      Qt::OrderedAlphaDither | Qt::OrderedDither);

    if (m_verboseMode) {
        printf(" - convertToFormat %s:%d -> %s:%d\n",
               qPrintable(srcName), src.format(),
               qPrintable(destName), dest.format());
    }

    m_imageMap[destName] = dest;
}

/***************************************************************************************************/
void PaintCommands::command_textlayout_draw(QRegExp re)
{
    QStringList caps = re.capturedTexts();

    QString text = caps.at(1);
    double width = convertToDouble(caps.at(2));

    if (m_verboseMode)
        printf(" - textlayout_draw text='%s', width=%f\n",
               qPrintable(text), width);

    QFont copy = m_painter->font();
    copy.setPointSize(10);

    QTextLayout layout(text, copy, m_painter->device());
    layout.beginLayout();

    double y_offset = 0;

    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width);
        line.setPosition(QPointF(0, y_offset));

        y_offset += line.height();
    }

    layout.draw(m_painter, QPointF(0, 0));
}

/***************************************************************************************************/
void PaintCommands::command_pen_setDashPattern(QRegExp re)
{
    static QRegExp separators("\\s");
    QStringList caps = re.capturedTexts();
    QString cap = caps.at(1);
    QStringList numbers = cap.split(separators, QString::SkipEmptyParts);

    QVector<qreal> pattern;
    for (int i=0; i<numbers.size(); ++i)
        pattern.append(convertToDouble(numbers.at(i)));

    if (m_verboseMode)
        printf(" - pen_setDashPattern(size=%d)\n", pattern.size());

    QPen p = m_painter->pen();
    p.setDashPattern(pattern);
    m_painter->setPen(p);
}

/***************************************************************************************************/
void PaintCommands::command_pen_setCosmetic(QRegExp re)
{
    QString hm = re.capturedTexts().at(1);
    bool on = hm == "true" || hm == "yes" || hm == "on";

    if (m_verboseMode) {
        printf(" - pen_setCosmetic(%s)\n", on ? "true" : "false");
    }

    QPen p = m_painter->pen();
    p.setCosmetic(on);

    m_painter->setPen(p);
}

/***************************************************************************************************/
void PaintCommands::command_drawConvexPolygon(QRegExp re)
{
    static QRegExp separators("\\s");
    QStringList caps = re.capturedTexts();
    QString cap = caps.at(1);
    QStringList numbers = cap.split(separators, QString::SkipEmptyParts);

    QPolygonF array;
    for (int i=0; i + 1<numbers.size(); i+=2)
        array.append(QPointF(convertToDouble(numbers.at(i)), convertToDouble(numbers.at(i+1))));

    if (m_verboseMode)
        printf(" - drawConvexPolygon(size=%d)\n", array.size());


    m_painter->drawConvexPolygon(array);
}
