/*  pcfdrivr.c

    FreeType font driver for pcf files

    Copyright (C) 2000, 2001, 2002, 2003, 2004 by
    Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <ft2build.h>

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H
#include FT_GZIP_H
#include FT_LZW_H
#include FT_ERRORS_H
#include FT_BDF_H

#include "pcf.h"
#include "pcfdrivr.h"
#include "pcfread.h"

#include "pcferror.h"
#include "pcfutil.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pcfread

#include FT_SERVICE_BDF_H
#include FT_SERVICE_XFREE86_NAME_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pcfdriver


  typedef struct  PCF_CMapRec_
  {
    FT_CMapRec    root;
    FT_UInt       num_encodings;
    PCF_Encoding  encodings;

  } PCF_CMapRec, *PCF_CMap;


  FT_CALLBACK_DEF( FT_Error )
  pcf_cmap_init( FT_CMap     pcfcmap,   /* PCF_CMap */
                 FT_Pointer  init_data )
  {
    PCF_CMap  cmap = (PCF_CMap)pcfcmap;
    PCF_Face  face = (PCF_Face)FT_CMAP_FACE( pcfcmap );

    FT_UNUSED( init_data );


    cmap->num_encodings = (FT_UInt)face->nencodings;
    cmap->encodings     = face->encodings;

    return PCF_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  pcf_cmap_done( FT_CMap  pcfcmap )         /* PCF_CMap */
  {
    PCF_CMap  cmap = (PCF_CMap)pcfcmap;


    cmap->encodings     = NULL;
    cmap->num_encodings = 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  pcf_cmap_char_index( FT_CMap    pcfcmap,  /* PCF_CMap */
                       FT_UInt32  charcode )
  {
    PCF_CMap      cmap      = (PCF_CMap)pcfcmap;
    PCF_Encoding  encodings = cmap->encodings;
    FT_UInt       min, max, mid;
    FT_UInt       result    = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        result = encodings[mid].glyph + 1;
        break;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  pcf_cmap_char_next( FT_CMap    pcfcmap,   /* PCF_CMap */
                      FT_UInt32  *acharcode )
  {
    PCF_CMap      cmap      = (PCF_CMap)pcfcmap;
    PCF_Encoding  encodings = cmap->encodings;
    FT_UInt       min, max, mid;
    FT_UInt32     charcode  = *acharcode + 1;
    FT_UInt       result    = 0;


    min = 0;
    max = cmap->num_encodings;

    while ( min < max )
    {
      FT_UInt32  code;


      mid  = ( min + max ) >> 1;
      code = encodings[mid].enc;

      if ( charcode == code )
      {
        result = encodings[mid].glyph + 1;
        goto Exit;
      }

      if ( charcode < code )
        max = mid;
      else
        min = mid + 1;
    }

    charcode = 0;
    if ( min < cmap->num_encodings )
    {
      charcode = encodings[min].enc;
      result   = encodings[min].glyph + 1;
    }

  Exit:
    *acharcode = charcode;
    return result;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_CMap_ClassRec  pcf_cmap_class =
  {
    sizeof ( PCF_CMapRec ),
    pcf_cmap_init,
    pcf_cmap_done,
    pcf_cmap_char_index,
    pcf_cmap_char_next
  };


  FT_CALLBACK_DEF( void )
  PCF_Face_Done( FT_Face  pcfface )         /* PCF_Face */
  {
    PCF_Face   face   = (PCF_Face)pcfface;
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( face->encodings );
    FT_FREE( face->metrics );

    /* free properties */
    {
      PCF_Property  prop = face->properties;
      FT_Int        i;


      for ( i = 0; i < face->nprops; i++ )
      {
        prop = &face->properties[i];

        FT_FREE( prop->name );
        if ( prop->isString )
          FT_FREE( prop->value );
      }

      FT_FREE( face->properties );
    }

    FT_FREE( face->toc.tables );
    FT_FREE( pcfface->family_name );
    FT_FREE( pcfface->available_sizes );
    FT_FREE( face->charset_encoding );
    FT_FREE( face->charset_registry );

    FT_TRACE4(( "PCF_Face_Done: done face\n" ));

    /* close gzip/LZW stream if any */
    if ( pcfface->stream == &face->gzip_stream )
    {
      FT_Stream_Close( &face->gzip_stream );
      pcfface->stream = face->gzip_source;
    }
  }


  FT_CALLBACK_DEF( FT_Error )
  PCF_Face_Init( FT_Stream      stream,
                 FT_Face        pcfface,        /* PCF_Face */
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    PCF_Face  face  = (PCF_Face)pcfface;
    FT_Error  error = PCF_Err_Ok;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );


    error = pcf_load_font( stream, face );
    if ( error )
    {
      FT_Error  error2;


      /* this didn't work, try gzip support! */
      error2 = FT_Stream_OpenGzip( &face->gzip_stream, stream );
      if ( FT_ERROR_BASE( error2 ) == FT_Err_Unimplemented_Feature )
        goto Fail;

      error = error2;
      if ( error )
      {
        FT_Error  error3;


        /* this didn't work, try LZW support! */
        error3 = FT_Stream_OpenLZW( &face->gzip_stream, stream );
        if ( FT_ERROR_BASE( error3 ) == FT_Err_Unimplemented_Feature )
          goto Fail;

        error = error3;
        if ( error )
          goto Fail;

        face->gzip_source = stream;
        pcfface->stream   = &face->gzip_stream;

        stream = pcfface->stream;

        error = pcf_load_font( stream, face );
        if ( error )
          goto Fail;
      }
      else
      {
        face->gzip_source = stream;
        pcfface->stream   = &face->gzip_stream;

        stream = pcfface->stream;

        error = pcf_load_font( stream, face );
        if ( error )
          goto Fail;
      }
    }

    /* set up charmap */
    {
      FT_String  *charset_registry = face->charset_registry;
      FT_String  *charset_encoding = face->charset_encoding;
      FT_Bool     unicode_charmap  = 0;


      if ( charset_registry && charset_encoding )
      {
        char*  s = charset_registry;


        /* Uh, oh, compare first letters manually to avoid dependency
           on locales. */
        if ( ( s[0] == 'i' || s[0] == 'I' ) &&
             ( s[1] == 's' || s[1] == 'S' ) &&
             ( s[2] == 'o' || s[2] == 'O' ) )
        {
          s += 3;
          if ( !ft_strcmp( s, "10646" )                      ||
               ( !ft_strcmp( s, "8859" ) &&
                 !ft_strcmp( face->charset_encoding, "1" ) ) )
          unicode_charmap = 1;
        }
      }

      {
        FT_CharMapRec  charmap;


        charmap.face        = FT_FACE( face );
        charmap.encoding    = FT_ENCODING_NONE;
        charmap.platform_id = 0;
        charmap.encoding_id = 0;

        if ( unicode_charmap )
        {
          charmap.encoding    = FT_ENCODING_UNICODE;
          charmap.platform_id = 3;
          charmap.encoding_id = 1;
        }

        error = FT_CMap_New( &pcf_cmap_class, NULL, &charmap, NULL );

#if 0
        /* Select default charmap */
        if ( pcfface->num_charmaps )
          pcfface->charmap = pcfface->charmaps[0];
#endif
      }
    }

  Exit:
    return error;

  Fail:
    FT_TRACE2(( "[not a valid PCF file]\n" ));
    error = PCF_Err_Unknown_File_Format;  /* error */
    goto Exit;
  }


  FT_CALLBACK_DEF( FT_Error )
  PCF_Set_Pixel_Size( FT_Size  size,
                      FT_UInt  pixel_width,
                      FT_UInt  pixel_height )
  {
    PCF_Face  face = (PCF_Face)FT_SIZE_FACE( size );

    FT_UNUSED( pixel_width );
    FT_UNUSED( pixel_height );


    FT_TRACE4(( "rec %d - pres %d\n", size->metrics.y_ppem,
                                      face->root.available_sizes->y_ppem >> 6 ));

    if ( size->metrics.y_ppem == face->root.available_sizes->y_ppem >> 6 )
    {
      size->metrics.ascender    = face->accel.fontAscent << 6;
      size->metrics.descender   = face->accel.fontDescent * (-64);
#if 0
      size->metrics.height      = face->accel.maxbounds.ascent << 6;
#endif
      size->metrics.height      = size->metrics.ascender -
                                  size->metrics.descender;

      size->metrics.max_advance = face->accel.maxbounds.characterWidth << 6;

      return PCF_Err_Ok;
    }
    else
    {
      FT_TRACE4(( "size WRONG\n" ));
      return PCF_Err_Invalid_Pixel_Size;
    }
  }


  FT_CALLBACK_DEF( FT_Error )
  PCF_Set_Point_Size( FT_Size     size,
                      FT_F26Dot6  char_width,
                      FT_F26Dot6  char_height,
                      FT_UInt     horz_resolution,
                      FT_UInt     vert_resolution )
  {
    FT_UNUSED( char_width );
    FT_UNUSED( char_height );
    FT_UNUSED( horz_resolution );
    FT_UNUSED( vert_resolution );

    return PCF_Set_Pixel_Size( size, 0, 0 );
  }


  FT_CALLBACK_DEF( FT_Error )
  PCF_Glyph_Load( FT_GlyphSlot  slot,
                  FT_Size       size,
                  FT_UInt       glyph_index,
                  FT_Int32      load_flags )
  {
    PCF_Face    face   = (PCF_Face)FT_SIZE_FACE( size );
    FT_Stream   stream = face->root.stream;
    FT_Error    error  = PCF_Err_Ok;
    FT_Bitmap*  bitmap = &slot->bitmap;
    PCF_Metric  metric;
    int         bytes;

    FT_UNUSED( load_flags );


    FT_TRACE4(( "load_glyph %d ---", glyph_index ));

    if ( !face )
    {
      error = PCF_Err_Invalid_Argument;
      goto Exit;
    }

    if ( glyph_index > 0 )
      glyph_index--;

    metric = face->metrics + glyph_index;

    bitmap->rows       = metric->ascent + metric->descent;
    bitmap->width      = metric->rightSideBearing - metric->leftSideBearing;
    bitmap->num_grays  = 1;
    bitmap->pixel_mode = FT_PIXEL_MODE_MONO;

    FT_TRACE6(( "BIT_ORDER %d ; BYTE_ORDER %d ; GLYPH_PAD %d\n",
                  PCF_BIT_ORDER( face->bitmapsFormat ),
                  PCF_BYTE_ORDER( face->bitmapsFormat ),
                  PCF_GLYPH_PAD( face->bitmapsFormat ) ));

    switch ( PCF_GLYPH_PAD( face->bitmapsFormat ) )
    {
    case 1:
      bitmap->pitch = ( bitmap->width + 7 ) >> 3;
      break;

    case 2:
      bitmap->pitch = ( ( bitmap->width + 15 ) >> 4 ) << 1;
      break;

    case 4:
      bitmap->pitch = ( ( bitmap->width + 31 ) >> 5 ) << 2;
      break;

    case 8:
      bitmap->pitch = ( ( bitmap->width + 63 ) >> 6 ) << 3;
      break;

    default:
      return PCF_Err_Invalid_File_Format;
    }

    /* XXX: to do: are there cases that need repadding the bitmap? */
    bytes = bitmap->pitch * bitmap->rows;

    error = ft_glyphslot_alloc_bitmap( slot, bytes );
    if ( error )
      goto Exit;

    if ( FT_STREAM_SEEK( metric->bits )          ||
         FT_STREAM_READ( bitmap->buffer, bytes ) )
      goto Exit;

    if ( PCF_BIT_ORDER( face->bitmapsFormat ) != MSBFirst )
      BitOrderInvert( bitmap->buffer, bytes );

    if ( ( PCF_BYTE_ORDER( face->bitmapsFormat ) !=
           PCF_BIT_ORDER( face->bitmapsFormat )  ) )
    {
      switch ( PCF_SCAN_UNIT( face->bitmapsFormat ) )
      {
      case 1:
        break;

      case 2:
        TwoByteSwap( bitmap->buffer, bytes );
        break;

      case 4:
        FourByteSwap( bitmap->buffer, bytes );
        break;
      }
    }

    slot->bitmap_left = metric->leftSideBearing;
    slot->bitmap_top  = metric->ascent;

    slot->metrics.horiAdvance  = metric->characterWidth << 6;
    slot->metrics.horiBearingX = metric->leftSideBearing << 6;
    slot->metrics.horiBearingY = metric->ascent << 6;
    slot->metrics.width        = ( metric->rightSideBearing -
                                   metric->leftSideBearing ) << 6;
    slot->metrics.height       = bitmap->rows << 6;

    slot->linearHoriAdvance = (FT_Fixed)bitmap->width << 16;
    slot->format            = FT_GLYPH_FORMAT_BITMAP;

    FT_TRACE4(( " --- ok\n" ));

  Exit:
    return error;
  }


 /*
  *
  *  BDF SERVICE
  *
  */

  static FT_Error
  pcf_get_bdf_property( PCF_Face          face,
                        const char*       prop_name,
                        BDF_PropertyRec  *aproperty )
  {
    PCF_Property  prop;


    prop = pcf_find_property( face, prop_name );
    if ( prop != NULL )
    {
      if ( prop->isString )
      {
        aproperty->type   = BDF_PROPERTY_TYPE_ATOM;
        aproperty->u.atom = prop->value.atom;
      }
      else
      {
        /* Apparently, the PCF driver loads all properties as signed integers!
         * This really doesn't seem to be a problem, because this is
         * sufficient for any meaningful values.
         */
        aproperty->type      = BDF_PROPERTY_TYPE_INTEGER;
        aproperty->u.integer = prop->value.integer;
      }
      return 0;
    }

    return PCF_Err_Invalid_Argument;
  }


  static FT_Error
  pcf_get_charset_id( PCF_Face      face,
                      const char*  *acharset_encoding,
                      const char*  *acharset_registry )
  {
    *acharset_encoding = face->charset_encoding;
    *acharset_registry = face->charset_registry;

    return 0;
  }


  static const FT_Service_BDFRec  pcf_service_bdf =
  {
    (FT_BDF_GetCharsetIdFunc)pcf_get_charset_id,
    (FT_BDF_GetPropertyFunc) pcf_get_bdf_property
  };


 /*
  *
  *  SERVICE LIST
  *
  */

  static const FT_ServiceDescRec  pcf_services[] =
  {
    { FT_SERVICE_ID_BDF,       &pcf_service_bdf },
    { FT_SERVICE_ID_XF86_NAME, FT_XF86_FORMAT_PCF },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  pcf_driver_requester( FT_Module    module,
                        const char*  name )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( pcf_services, name );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  pcf_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER        |
      FT_MODULE_DRIVER_NO_OUTLINES,
      sizeof ( FT_DriverRec ),

      "pcf",
      0x10000L,
      0x20000L,

      0,

      0,
      0,
      pcf_driver_requester
    },

    sizeof ( PCF_FaceRec ),
    sizeof ( FT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),

    PCF_Face_Init,
    PCF_Face_Done,
    0,                      /* FT_Size_InitFunc */
    0,                      /* FT_Size_DoneFunc */
    0,                      /* FT_Slot_InitFunc */
    0,                      /* FT_Slot_DoneFunc */

    PCF_Set_Point_Size,
    PCF_Set_Pixel_Size,

    PCF_Glyph_Load,

    0,                      /* FT_Face_GetKerningFunc  */
    0,                      /* FT_Face_AttachFunc      */
    0                       /* FT_Face_GetAdvancesFunc */
  };


/* END */
