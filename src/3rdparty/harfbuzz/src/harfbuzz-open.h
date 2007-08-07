/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Copyright 2006  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
#ifndef HARFBUZZ_OPEN_H
#define HARFBUZZ_OPEN_H

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER

/* Use this if a feature applies to all glyphs */
#define HB_ALL_GLYPHS                    0xFFFF

#define HB_DEFAULT_LANGUAGE              0xFFFF

#define HB_MAX_NESTING_LEVEL             100

#define HB_Err_Invalid_SubTable_Format   0x1000
#define HB_Err_Invalid_SubTable          0x1001
#define HB_Err_Not_Covered               0x1002
#define HB_Err_Too_Many_Nested_Contexts  0x1003
#define HB_Err_No_MM_Interpreter         0x1004
#define HB_Err_Empty_Script              0x1005


/* Script list related structures */

struct  HB_LangSys_
{
  HB_UShort   LookupOrderOffset;      /* always 0 for TT Open 1.0  */
  HB_UShort   ReqFeatureIndex;        /* required FeatureIndex     */
  HB_UShort   FeatureCount;           /* number of Feature indices */
  HB_UShort*  FeatureIndex;           /* array of Feature indices  */
};

typedef struct HB_LangSys_  HB_LangSys;


struct  HB_LangSysRecord_
{
  HB_UInt     LangSysTag;            /* LangSysTag identifier */
  HB_LangSys  LangSys;               /* LangSys table         */
};

typedef struct HB_LangSysRecord_  HB_LangSysRecord;


struct  HB_ScriptTable_
{
  HB_LangSys         DefaultLangSys; /* DefaultLangSys table     */
  HB_UShort           LangSysCount;   /* number of LangSysRecords */
  HB_LangSysRecord*  LangSysRecord;  /* array of LangSysRecords  */
};

typedef struct HB_ScriptTable_  HB_ScriptTable;


struct  HB_ScriptRecord_
{
  HB_UInt        ScriptTag;              /* ScriptTag identifier */
  HB_ScriptTable  Script;                 /* Script table         */
};

typedef struct HB_ScriptRecord_  HB_ScriptRecord;


struct  HB_ScriptList_
{
  HB_UShort          ScriptCount;     /* number of ScriptRecords */
  HB_ScriptRecord*  ScriptRecord;    /* array of ScriptRecords  */
};

typedef struct HB_ScriptList_  HB_ScriptList;


/* Feature list related structures */

struct HB_Feature_
{
  HB_UShort   FeatureParams;          /* always 0 for TT Open 1.0     */
  HB_UShort   LookupListCount;        /* number of LookupList indices */
  HB_UShort*  LookupListIndex;        /* array of LookupList indices  */
};

typedef struct HB_Feature_  HB_Feature;


struct  HB_FeatureRecord_
{
  HB_UInt     FeatureTag;            /* FeatureTag identifier */
  HB_Feature  Feature;               /* Feature table         */
};

typedef struct HB_FeatureRecord_  HB_FeatureRecord;


struct  HB_FeatureList_
{
  HB_UShort           FeatureCount;   /* number of FeatureRecords */
  HB_FeatureRecord*  FeatureRecord;  /* array of FeatureRecords  */
  HB_UShort*		ApplyOrder;	/* order to apply features */
  HB_UShort		ApplyCount;	/* number of elements in ApplyOrder */
};

typedef struct HB_FeatureList_  HB_FeatureList;


/* Lookup list related structures */

typedef struct HB_SubTable_  HB_SubTable;


struct  HB_Lookup_
{
  HB_UShort      LookupType;          /* Lookup type         */
  HB_UShort      LookupFlag;          /* Lookup qualifiers   */
  HB_UShort      SubTableCount;       /* number of SubTables */
  HB_SubTable*  SubTable;            /* array of SubTables  */
};

typedef struct HB_Lookup_  HB_Lookup;


/* The `Properties' field is not defined in the OpenType specification but
   is needed for processing lookups.  If properties[n] is > 0, the
   functions HB_GSUB_Apply_String() resp. HB_GPOS_Apply_String() will
   process Lookup[n] for glyphs which have the specific bit not set in
   the `properties' field of the input string object.                  */

struct  HB_LookupList_
{
  HB_UShort    LookupCount;           /* number of Lookups       */
  HB_Lookup*  Lookup;                /* array of Lookup records */
  HB_UInt*     Properties;            /* array of flags          */
};

typedef struct HB_LookupList_  HB_LookupList;


/* Possible LookupFlag bit masks.  `HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS' comes from the
   OpenType 1.2 specification; HB_LOOKUP_FLAG_RIGHT_TO_LEFT has been (re)introduced in
   OpenType 1.3 -- if set, the last glyph in a cursive attachment
   sequence has to be positioned on the baseline -- regardless of the
   writing direction.                                                    */

#define HB_LOOKUP_FLAG_RIGHT_TO_LEFT         0x0001
#define HB_LOOKUP_FLAG_IGNORE_BASE_GLYPHS    0x0002
#define HB_LOOKUP_FLAG_IGNORE_LIGATURES      0x0004
#define HB_LOOKUP_FLAG_IGNORE_MARKS          0x0008
#define HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS  0xFF00


struct  HB_CoverageFormat1_
{
  HB_UShort   GlyphCount;             /* number of glyphs in GlyphArray */
  HB_UShort*  GlyphArray;             /* array of glyph IDs             */
};

typedef struct HB_CoverageFormat1_  HB_CoverageFormat1;


struct HB_RangeRecord_
{
  HB_UShort  Start;                   /* first glyph ID in the range */
  HB_UShort  End;                     /* last glyph ID in the range  */
  HB_UShort  StartCoverageIndex;      /* coverage index of first
					 glyph ID in the range       */
};

typedef struct HB_RangeRecord_  HB_RangeRecord;


struct  HB_CoverageFormat2_
{
  HB_UShort         RangeCount;       /* number of RangeRecords */
  HB_RangeRecord*  RangeRecord;      /* array of RangeRecords  */
};

typedef struct HB_CoverageFormat2_  HB_CoverageFormat2;


struct  HB_Coverage_
{
  HB_UShort  CoverageFormat;          /* 1 or 2 */

  union
  {
    HB_CoverageFormat1  cf1;
    HB_CoverageFormat2  cf2;
  } cf;
};

typedef struct HB_Coverage_  HB_Coverage;


struct  HB_ClassDefFormat1_
{
  HB_UShort   StartGlyph;             /* first glyph ID of the
					 ClassValueArray             */
  HB_UShort   GlyphCount;             /* size of the ClassValueArray */
  HB_UShort*  ClassValueArray;        /* array of class values       */
};

typedef struct HB_ClassDefFormat1_  HB_ClassDefFormat1;


struct  HB_ClassRangeRecord_
{
  HB_UShort  Start;                   /* first glyph ID in the range    */
  HB_UShort  End;                     /* last glyph ID in the range     */
  HB_UShort  Class;                   /* applied to all glyphs in range */
};

typedef struct HB_ClassRangeRecord_  HB_ClassRangeRecord;


struct  HB_ClassDefFormat2_
{
  HB_UShort              ClassRangeCount;
				      /* number of ClassRangeRecords */
  HB_ClassRangeRecord*  ClassRangeRecord;
				      /* array of ClassRangeRecords  */
};

typedef struct HB_ClassDefFormat2_  HB_ClassDefFormat2;


/* The `Defined' field is not defined in the OpenType specification but
   apparently needed for processing fonts like trado.ttf: This font
   refers to a class which contains not a single element.  We map such
   classes to class 0.                                                 */

struct  HB_ClassDefinition_
{
  FT_Bool    loaded;

  FT_Bool*   Defined;                 /* array of Booleans.
					 If Defined[n] is FALSE,
					 class n contains no glyphs. */
  HB_UShort  ClassFormat;             /* 1 or 2                      */

  union
  {
    HB_ClassDefFormat1  cd1;
    HB_ClassDefFormat2  cd2;
  } cd;
};

typedef struct HB_ClassDefinition_  HB_ClassDefinition;


struct HB_Device_
{
  HB_UShort   StartSize;              /* smallest size to correct      */
  HB_UShort   EndSize;                /* largest size to correct       */
  HB_UShort   DeltaFormat;            /* DeltaValue array data format:
					 1, 2, or 3                    */
  HB_UShort*  DeltaValue;             /* array of compressed data      */
};

typedef struct HB_Device_  HB_Device;


enum  HB_Type_
{
  HB_Type_GSUB,
  HB_Type_GPOS
};

typedef enum HB_Type_  HB_Type;


FT_END_HEADER

#endif /* HARFBUZZ_OPEN_H */
