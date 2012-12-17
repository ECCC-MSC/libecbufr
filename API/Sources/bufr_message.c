/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majest� la Reine du Chef du Canada, Environnement Canada, 2009.

This file is part of libECBUFR.

    libECBUFR is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License,
    version 3, as published by the Free Software Foundation.

    libECBUFR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public
    License along with libECBUFR.  If not, see <http://www.gnu.org/licenses/>.

 * fichier:  bufr_message.c
 *
 * auteur:  Vanh Souvanlasy (septembre 2007)
 *
 * fonction: fonctions de la structure de donnees  BUFR_Message
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>

#include "bufr_util.h"
#include "bufr_array.h"
#include "bufr_tables.h"
#include "bufr_message.h"
#include "bufr_sequence.h"
#include "bufr_i18n.h"


static int   bufr_encode_sect3 ( BUFR_Message *bufr );
static void  bufr_alloc_sect3  ( BUFR_Message *bufr );

/**
 * @english
 *    bufr_create_message()
 *    (int edition)
 * This is used to create one message without a template. It would normally
 * be used in the encoding process for ad-hoc messages but is usually not
 * needed nor recommended.
 * @return BUFR_Message
 * @endenglish
 * @francais
 * @brief constructeur de la structure BUFR_Message
 * @todo translate to French
 * @endfrancais
 * @ingroup message
 */
BUFR_Message *bufr_create_message(int edition)
   {
   BUFR_Message *r;

   r = (BUFR_Message *)malloc(sizeof(BUFR_Message));
   r->s1.data         = NULL;
   r->s1.data_len     = 0;
   r->s2.data         = NULL;
   r->s2.data_len     = 0;
   r->s3.data         = NULL;
   r->s3.desc_list    = (IntArray)arr_create( 2000, sizeof(int), 100 );
   r->s4.data         = NULL;
   r->s4.current      = NULL;
   r->s4.bitno        = 0;
   r->s3.max_len      = 0;
   r->s4.max_len      = 0;
   r->s4.max_data_len = 0;
   r->len_msg         = -1;        /* need to be set */
   r->header_string   = NULL;
   r->header_len      = 0;
   bufr_init_header( r, edition );
   return r;
   }

/**
 * @english
 *    bufr_free_message( msg )
 *    (BUFR_Message *bufr)
 * The purpose of this call is to discard the message structure no longer
 * needed that was created by bufr_read_messageor bufr_create_message.
 * @return void
 * @endenglish
 * @francais
 * @brief destructeur de la structure BUFR_Message
 * @todo translate to French
 * @param r la structure a detruire
 * @endfrancais
 * @ingroup message
 */
void  bufr_free_message( BUFR_Message *r )
   {
   if ( r == NULL ) return;
   if (r->s3.data != NULL) free( r->s3.data );
   if (r->s2.data != NULL) free( r->s2.data );
   if (r->s1.data != NULL) free( r->s1.data );
   arr_free( &(r->s3.desc_list) );
   if (r->s4.data != NULL) 
      free( r->s4.data );
   if (r->header_string != NULL)
      free( r->header_string );

   r->header_string = NULL;
   r->s4.data = NULL;
   r->s3.data = NULL;
   r->s2.data = NULL;
   free( r );
   }

/**
 * @english
 * @brief print message header infos
 * This call prints a formatted message describing each section’s
 * version, length, and header information. It does not print the content
 * of the data.
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup debug io message
 */
void  bufr_print_message( BUFR_Message *bufr, void (*print_proc)(const char *) )
   {
   char  *str;
   int    len;

   if (bufr == NULL) 
      {
      bufr_print_debug( _("Warning: bufr_print_message( NULL )\n") );
      return;
      }

/*
 * header_string length could be ???
 */
   len = 256;
   if ( bufr->header_string )
      len += strlen( bufr->header_string );
   str = (char *)malloc( len * sizeof(char) );
   if ( bufr->header_string )
      {
      sprintf( str, _("### Message header            : \"%s\"\n"), bufr->header_string );
      print_proc( str );
      }

   sprintf( str, _("### BUFR Edition              : %d\n"),      bufr->edition  );
   print_proc( str );
   sprintf( str, _("###          length           : %d\n"),      bufr->len_msg  );
   print_proc( str );
   sprintf( str, _("###   Section 0\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s0.len   );
   print_proc( str );
   sprintf( str, _("###   Section 1\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s1.len   );
   print_proc( str );
	if( bufr->s1.data_len>0 )
		{
		sprintf( str, _("###      data length          : %d\n"),
			bufr->s1.data_len   );
		print_proc( str );
		}
   sprintf( str, _("###      BUFR master table    : %d\n"),      bufr->s1.bufr_master_table );
   print_proc( str );
   if (bufr->edition == 2)
      {
      sprintf( str, _("###      originating center   : %d\n"),      bufr->s1.orig_centre );
      print_proc( str );
      }
   else if (bufr->edition >= 3)
      {
      sprintf( str, _("###      originating center   : %d\n"),      bufr->s1.orig_centre );
      print_proc( str );
      sprintf( str, _("###      sub center           : %d\n"),      bufr->s1.orig_sub_centre );
      print_proc( str );
      }
   sprintf( str, _("###      update sequence number : %d\n"),      bufr->s1.upd_seq_no );
   print_proc( str );
   sprintf( str, _("###      Data category        : %d\n"),      bufr->s1.msg_type );
   print_proc( str );
   if (bufr->edition == 3)
      {
      sprintf( str, _("###      Data subcategory     : %d\n"),      bufr->s1.msg_local_subtype );
      print_proc( str );
      }
   else if (bufr->edition >= 4)
      {
      sprintf( str, _("###      International sub category  : %d\n"),      bufr->s1.msg_inter_subtype );
      print_proc( str );
      sprintf( str, _("###      Local sub category   : %d\n"),      bufr->s1.msg_local_subtype );
      print_proc( str );
      }
   sprintf( str, _("###      master table version : %d\n"),      bufr->s1.master_table_version );
   print_proc( str );
   sprintf( str, _("###      local table version  : %d\n"),      bufr->s1.local_table_version );
   print_proc( str );
   sprintf( str, _("###      Year                 : %d\n"),      bufr->s1.year );
   print_proc( str );
   sprintf( str, _("###      Month                : %d\n"),      bufr->s1.month );
   print_proc( str );
   sprintf( str, _("###      Day                  : %d\n"),      bufr->s1.day );
   print_proc( str );
   sprintf( str, _("###      Hour                 : %d\n"),      bufr->s1.hour );
   print_proc( str );
   sprintf( str, _("###      Min                  : %d\n"),      bufr->s1.minute );
   print_proc( str );
   if (bufr->edition >= 4)
      {
      sprintf( str, _("###      Sec                  : %d\n"),      bufr->s1.second );
      print_proc( str );
      }
   sprintf( str, _("###      octet 8              : %d\n"),      bufr->s1.flag   );
   print_proc( str );
   sprintf( str, _("###         optional section  :") );
   print_proc( str );
   if (bufr->s1.flag & 1 )
      print_proc( _(" Yes\n") );
   else
      print_proc( _(" No\n") );


   sprintf( str, _("###   Section 2\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s2.len   );
   print_proc( str );
   sprintf( str, _("###   Section 3\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s3.len   );
   print_proc( str );
   sprintf( str, _("###      datasubsets          : %d\n"),      bufr->s3.no_data_subsets   );
   print_proc( str );
   sprintf( str, _("###      octet 7              : %d\n"),      bufr->s3.flag   );
   print_proc( str );
   sprintf( str, _("###         compression       :") );
   print_proc( str );
   if (BUFR_IS_COMPRESSED( bufr ))
      print_proc( _(" Yes\n") );
   else
      print_proc( _(" No\n") );
   if ( bufr->s3.flag & BUFR_FLAG_OBSERVED )
      sprintf( str, _("###         observed data\n") );
   else
      sprintf( str, _("###         other data\n") );
   print_proc( str );
   sprintf( str, _("###   Section 4\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s4.len   );
   print_proc( str );
   sprintf( str, _("###   Section 5\n") );
   print_proc( str );
   sprintf( str, _("###      length               : %d\n"),      bufr->s5.len   );
   print_proc( str );

   print_proc( "###\n" );
   free( str );
   }

/**
 * @english
 * @brief set the contents of the optional and arbitrary BUFR section 2
 *
 * Note that only the last set section 2 data will actually be encoded
 * to the message.
 * 
 * @param r BUFR message to update
 * @param data buffer containing len octets to write
 * @param len number of octets to write into section 2
 * @endenglish
 * @francais
 * @brief mettre des donnees dans la section 2
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @ingroup encode message
 */
void  bufr_sect2_set_data( BUFR_Message *r, const char *data, int len )
   {
   int  len2;
   if ( r == NULL ) return;

   if (r->s2.data != NULL)
      free( r->s2.data );

/*
 * padding to even octets if necessary when edition is 3

*/
   len2 = (r->edition <= 3)&&(len % 2) ? len + 1 : len;
   r->s2.data = (char *)malloc( len2 * sizeof(char) );
   memcpy( r->s2.data, data, len );
   /*
    * even octets padding
    */
   if (len < len2)
      {
      r->s2.data[len2-1] = 0;
      }

   r->s2.data_len = len2;
   r->s2.len = r->s2.header_len + len2;

   r->s1.flag  |= 1;  /* has optional section */

	/* HACK: if section 2 changes, it's necessary to recalculate
	 * the overall message length.
	 */
	bufr_end_message(r);
   }

/**
 * @english
 * @todo translation
 * @endenglish
 * @francais
 * initialiser infos de BUFR_Message
 * @param     r : la structure a initialiser
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_init_header(BUFR_Message *bufr, int edition)
   {
   char errmsg[256];

   if ((edition < 2)||(edition > 5))
      {
      sprintf( errmsg, _("Warning: unsupported BUFR edition=%d, default=4\n"), edition );
      bufr_print_debug( errmsg );
      edition = 4;
      }

   bufr->edition         = edition;

   bufr_init_sect1( &(bufr->s1) );

   bufr->s0.len = 8;
   if (edition >= 4)
      {
      bufr->s1.header_len = 22;
      bufr->s1.len        = bufr->s1.header_len;
      }
   else /* edition == 2 or 3 */
      {
      /* 
       * length of s1 may be bigger than 18 if s1 contains additionnal data 
       */
      bufr->s1.header_len = 17;  
      bufr->s1.len        = bufr->s1.header_len + 1;
      }

   bufr->s2.header_len           = 4;
   bufr->s2.len                  = 0;

   bufr->s3.header_len           = 7;
   bufr->s3.len                  = bufr->s3.header_len;
   bufr->s4.header_len           = 4;
   bufr->s4.len                  = bufr->s4.header_len;
   bufr->s5.len                  = 4;

   BUFR_SET_NB_DATASET(bufr, 0);
   bufr->s3.flag                 = 0;             /* other data, non-compressed */

   bufr->s4.current              = NULL;
   bufr->s4.bitno                = 0;
   bufr->s4.filled               = 0;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * initialiser l'espace memoire pour la section 1
 * @param     bufr : la structure de donnees BUFR
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_init_sect1( BufrSection1 *s1 )
   {
   s1->header_len           = 0;
   s1->len                  = 0;
   s1->bufr_master_table    = 0;      /* 0 == Meteorology maintained by WMO */
   s1->orig_centre          = 54;     /* from CMC */
   s1->orig_sub_centre      = 0;
   s1->upd_seq_no           = 0; 
   s1->flag                 = 0;             /* no optional section */
   s1->msg_type             = MSGDTYPE_SURFACE_LAND;
   s1->msg_inter_subtype    = 0;
   s1->msg_local_subtype    = 0;
   s1->master_table_version = 2;
   s1->local_table_version  = 0;
   s1->year                 = 0;
   s1->month                = 0;
   s1->day                  = 0;
   s1->hour                 = 0;
   s1->minute               = 0;
   s1->second               = 0;
   s1->data                 = NULL;
   s1->data_len             = 0;
   }

/**
 * @english
 * This creates the BUFR message header (called in bufr-encode_message)
 * @return void
 * @endenglish
 * @francais
 * @brief transferer une copie des infos de la section 1
 * @todo translate to French
 * @endfrancais
 * @ingroup internal
 */
void bufr_copy_sect1( BufrSection1 *dest, BufrSection1 *src )
   {
   dest->bufr_master_table    = src->bufr_master_table;
   dest->orig_centre          = src->orig_centre;
   dest->orig_sub_centre      = src->orig_sub_centre;
   dest->upd_seq_no           = src->upd_seq_no; 
   dest->msg_type             = src->msg_type;
   dest->msg_inter_subtype    = src->msg_inter_subtype;
   dest->msg_local_subtype    = src->msg_local_subtype;
   dest->master_table_version = src->master_table_version;
   dest->local_table_version  = src->local_table_version;
   dest->year                 = src->year;
   dest->month                = src->month;
   dest->day                  = src->day;
   dest->hour                 = src->hour;
   dest->minute               = src->minute;
   dest->second               = src->second;
   }

/**
 * bufr_alloc_sect3
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * allouer l'espace memoire pour la section 3
 * @param     bufr : la structure de donnees BUFR
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_message.c

 */
static void bufr_alloc_sect3(BUFR_Message *bufr)
   {
   int len;

   len = arr_count(bufr->s3.desc_list) * 2; 
   /* 
    * for even octets padding 
    */
   if (bufr->edition <= 3) len += 1;
   if (bufr->s3.max_len < len) 
      {
      if (bufr->s3.data == NULL)
         bufr->s3.data = (char *)malloc(len * sizeof(char));
      else
         bufr->s3.data = (char *)realloc(bufr->s3.data, len * sizeof(char));
      bufr->s3.data[len - 1] = 0;
      bufr->s3.max_len = len;
      }
   }

/**
 * @english
 * This is a low level function to create a message structure called in
 * bufr_encode_message; this empties the BUFR message structure if it was
 * already used. This would be used if encoding without a template or as a
 * low-level call and you then you won’t see it. If there is a template,
 * this call isn’t needed. The support for Table C or Table D is absent
 * with this call.
 * @return void
 * @endenglish
 * @francais
 * @brief debuter l'encodage de message BUFR
 * @todo translate to French
 * @param bufr la structure de donnees BUFR
 * @endfrancais
 * @ingroup internal encode
 */
void bufr_begin_message(BUFR_Message *bufr)
   {
   arr_del( bufr->s3.desc_list, arr_count(bufr->s3.desc_list) );

   bufr->s4.current = bufr->s4.data;
   bufr->s4.bitno = 0;
   bufr->s4.filled = 0;
   }

/**
 * @english
 * This is a low level call called within bufr_encode_message. This would
 * be used if encoding without a template or as a low-level call (you
 * won’t see this). If there is a template, this call isn’t needed. The
 * support for Table C or Table D is absent with this call, it is up to the
 * user to make all the needed other calls. 
 * @endenglish
 * @francais
 * @brief termine l'encodage de message BUFR et l'ecrire au fichier
 * @todo translate to French
 * @param fp pointeur au fichier de sortie
 * @param bufr la structure de donnees BUFR
 * @endfrancais
 * @ingroup internal encode
 */
void bufr_end_message(BUFR_Message *bufr)
   {
   int rem;

   if (bufr->s1.flag  & 1)
      {
      bufr->s2.len = bufr->s2.header_len + bufr->s2.data_len;
      }
   else
      bufr->s2.len = 0;

   bufr_encode_sect3( bufr );

   if ((bufr->edition <= 3)&&(bufr->s3.len % 2))
      {
      bufr->s3.len += 1;
      }
/*
 * actual length of section 4

*/
   rem = (bufr->s4.bitno > 0) ? 1 : 0;
   bufr->s4.len = bufr->s4.filled + bufr->s4.header_len + rem;
/*
 * pad with 0 to have an even octets count

*/
   if ((bufr->edition <= 3)&&(bufr->s4.len % 2))
      {
      int bpos, nbits;

      bpos = bufr->s4.bitno % 8 ;
      nbits = 8 - bpos ;
      if (bpos != 0) nbits += 8;
      bufr_putbits( bufr, 0, nbits );
      rem = (bufr->s4.bitno > 0) ? 1 : 0;
      bufr->s4.len = bufr->s4.filled + bufr->s4.header_len + rem;
      }

   bufr->len_msg = bufr->s0.len +
                   bufr->s1.len +
                   bufr->s2.len +
                   bufr->s3.len +
                   bufr->s4.len +
                   bufr->s5.len ;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * encoder la section 3
 * @param     bufr : la structure de donnees BUFR
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_encode_sect3(BUFR_Message *bufr)
   {
   unsigned short code;
   int            i, count;
   unsigned char *ptr;
   int           *e1;
   int            debug=bufr_is_debug();
   char          errmsg[256];

   bufr_alloc_sect3( bufr );
   count = arr_count(bufr->s3.desc_list);
   e1 = (int *)arr_get( bufr->s3.desc_list, 0 );
   ptr = (unsigned char *)bufr->s3.data;
   for ( i = 0 ; i < count ; i++ ) 
      {
      code = bufr_descriptor_i32_to_i16( e1[i] );
      if (debug)
         {
         sprintf( errmsg, "#%.3d: %.6d --> %d\n", i+1,e1[i], code );
         bufr_print_debug( errmsg );
         }
      *ptr++ = code >> 8;
      *ptr++ = code % 256;
      }
   bufr->s3.len = bufr->s3.header_len + count*2 ;

   if (bufr->edition == 3)
      {
      if (bufr->s3.len % 2) 
         bufr->s3.len += 1;
      }

   return 0;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * initialiser la partie temps a l'heure GMT actuel
 * @param     s1  : pointeur a la section 1
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode
 */
void bufr_set_gmtime(BufrSection1 *s1)
   {
   time_t  temps;

   time( &temps );

   bufr_set_time_sect1( s1, temps );
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * initialiser le temps de la section 1
 * @param     s1  : pointeur a la section 1
 * @param     temps : le temps gmt
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode
 */
void bufr_set_time_sect1( BufrSection1 *s1, time_t temps )
   {
   struct  tm *gmt;

   gmt = gmtime( &temps );

   s1->year = gmt->tm_year + 1900;
   s1->month = gmt->tm_mon+1;
   s1->day = gmt->tm_mday;
   s1->hour = gmt->tm_hour;
   s1->minute = gmt->tm_min;
   s1->second = gmt->tm_sec;
   }
