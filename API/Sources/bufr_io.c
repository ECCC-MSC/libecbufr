/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2009.

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

 * fichier:  bufr_io.c
 *
 * auteur:  Vanh Souvanlasy (avril 1996)
 *
 * fonction: outils pour ecrire les fichiers BUFR
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

#include "private/bufr_util.h"
#include "bufr_array.h"
#include "bufr_tables.h"
#include "bufr_io.h"
#include "bufr_sequence.h"
#include "bufr_api.h"

#define DEBUG  0

static FILE *debug_fp=NULL;
static char *debug_filename=NULL;
static FILE *output_fp=NULL;
static char *output_filename=NULL;

static int  debugmode=0;
static int  verbosemode=0;

static void (*udf_abort)(const char *msg) = NULL;

static int   bufr_wr_section0 ( bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr );
static int   bufr_wr_section1 ( bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr );
static int   bufr_wr_section2 ( bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr );
static int   bufr_wr_section3 ( bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr );
static int   bufr_wr_section4 ( bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr );
static int   bufr_wr_section5 ( bufr_write_callback writecb,
                                void *cd);

static void  bufr_write_int3b ( bufr_write_callback writecb,
                                void *cd, int val );
static void      bufr_write_int2b ( bufr_write_callback writecb,
                                void *cd, int val );

static int       bufr_read_int3b  ( bufr_read_callback readcb, void *cd );
static int       bufr_read_int2b  ( bufr_read_callback readcb, void *cd );

static int       bufr_rd_section0 ( bufr_read_callback readcb,
                                void *cd, BUFR_Message * );
static int       bufr_rd_section1 ( bufr_read_callback readcb,
                                void *cd, BUFR_Message * );
static int       bufr_rd_section2 ( bufr_read_callback readcb,
                                void *cd, BUFR_Message * );
static int       bufr_rd_section3 ( bufr_read_callback readcb,
                                void *cd, BUFR_Message * );
static uint64_t  bufr_rd_section4 ( bufr_read_callback readcb,
                                void *cd, BUFR_Message * );
static int   bufr_rd_section5 ( bufr_read_callback readcb, void *cd );

static int   bufr_seek_msg_start( bufr_read_callback readcb, void *cd, char **tagstr, int *len );
static int   bufr_wr_header_string ( bufr_write_callback writecb, void *cd, BUFR_Message *bufr );

/**
 * bufr_write_octet
 * @english
 * internal function to write a single octet to a BUFR "sink"
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup bufr_io.c
  * nom: bufr_write_octet
 *
 * auteur:  Chris Beauregard
 *
 * fonction: internal function to write a single octet to a BUFR "sink"
 *
 * parametres: 
 *      writecb   : write callback
 *      cd : write client data
 *      b : octet to write

 */
static inline int bufr_write_octet(bufr_write_callback writecb,
                                void *cd, unsigned char b)
	{
		if( writecb == NULL ) return errno=EINVAL, -1;
		return writecb( cd, 1, &b );
	}

/**
 * bufr_read_octet
 * @english
 * internal function to read a single octet from a BUFR "source"
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup bufr_io.c
  * nom: bufr_read_octet
 *
 * auteur:  Chris Beauregard
 *
 * fonction: internal function to read a single octet from a BUFR "source"
 *
 * parametres: 
 *      readcb   : read callback
 *      cd : read client data
 *      octet : buffer to store octet
 * return:
 *      1 - byte read, 0 - EOF, <0 - othe rfailure

 */
static inline int bufr_read_octet(bufr_read_callback readcb, void *cd,
                                  unsigned char* octet)
	{
		return readcb( cd, 1, octet );
	}

/**
 * bufr_wr_section0
 * @english
 * ecrire la section 0 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section0
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 0 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section0(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
	if( writecb == NULL ) return errno=EINVAL, -1;

   writecb( cd, 4, "BUFR" );

   if (bufr_is_verbose())
      {
      bufr_print_debug( "### Writing BUFR Message\n" );
      bufr_print_message( bufr, bufr_print_debug );
      }

   bufr_write_int3b( writecb, cd, bufr->len_msg );
	bufr_write_octet( writecb, cd, bufr->edition );

   return 0;
   }

/**
 * bufr_wr_section1
 * @english
 * ecrire la section 1 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section1
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 1 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section1(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
	if( writecb == NULL ) return errno=EINVAL, -1;

   bufr_write_int3b( writecb, cd, bufr->s1.len );  /* octet 1 - 3 */

	/* octet 4 */
	bufr_write_octet( writecb, cd, bufr->s1.bufr_master_table);

   if (bufr->edition == 2)
      {
      bufr_write_int2b( writecb, cd, bufr->s1.orig_centre ); /* octet 5 - 6 */
      }
   else if (bufr->edition == 3)
      {
		/* octets 5, 6 */
		bufr_write_octet( writecb, cd, bufr->s1.orig_sub_centre);
		bufr_write_octet( writecb, cd, bufr->s1.orig_centre);
      }
   else if (bufr->edition >= 4)
      {
		/* octet 5 - 6 */
      bufr_write_int2b( writecb, cd, bufr->s1.orig_centre );
		/* octet 7 - 8 */
      bufr_write_int2b( writecb, cd, bufr->s1.orig_sub_centre );
      }

	bufr_write_octet( writecb, cd, bufr->s1.upd_seq_no );
	bufr_write_octet( writecb, cd, bufr->s1.flag );
	bufr_write_octet( writecb, cd, bufr->s1.msg_type );

   if (bufr->edition >= 4)
      {
		bufr_write_octet( writecb, cd, bufr->s1.msg_inter_subtype );
      }
	bufr_write_octet( writecb, cd, bufr->s1.msg_local_subtype );

	bufr_write_octet( writecb, cd, bufr->s1.master_table_version );
	bufr_write_octet( writecb, cd, bufr->s1.local_table_version );

   if (bufr->edition >= 4)
      {
      bufr_write_int2b( writecb, cd, bufr->s1.year );
      }
   else
      {
      unsigned char yy;

      yy = (bufr->s1.year - 1)%100 + 1;
		bufr_write_octet( writecb, cd, yy );
      }

	bufr_write_octet( writecb, cd, bufr->s1.month );
	bufr_write_octet( writecb, cd, bufr->s1.day );
	bufr_write_octet( writecb, cd, bufr->s1.hour );
	bufr_write_octet( writecb, cd, bufr->s1.minute );

   if (bufr->edition >= 4)
      {
		bufr_write_octet( writecb, cd, bufr->s1.second );
      }

/*
 * write reserved octet(s) if any

*/
   if ((bufr->edition >= 2)&&(bufr->s1.data_len > 0))
      {
      int len = bufr->s1.data_len;

      if (writecb( cd, len, bufr->s1.data ) != len) return -1;
      }
   else if ((bufr->edition == 2)||(bufr->edition == 3))
      {
      bufr_write_octet( writecb, cd, 0 ); /* filler */
      }

   return 0;
   }

/**
 * bufr_wr_section2
 * @english
 * ecrire la section 2 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section2
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 2 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section2(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
	if( writecb == NULL ) return errno=EINVAL, -1;
   if (bufr == NULL) return errno=EINVAL,-1;
   if (bufr->s2.len <= 0) return 0;
   
   bufr_write_int3b( writecb, cd, bufr->s2.len );

	bufr_write_octet( writecb, cd, 0 );	/* reserved */

	writecb( cd, bufr->s2.data_len, bufr->s2.data );

   return 0;
   }

/**
 * bufr_wr_section3
 * @english
 * ecrire la section 3 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section3
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 3 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section3(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
   int len;
	if( writecb == NULL ) return errno=EINVAL, -1;

   bufr_write_int3b( writecb, cd, bufr->s3.len );

	bufr_write_octet( writecb, cd, 0 );	/* reserved */

   bufr_write_int2b( writecb, cd, bufr->s3.no_data_subsets );

	bufr_write_octet( writecb, cd, bufr->s3.flag );

   len = bufr->s3.len - bufr->s3.header_len;
	writecb( cd, len, bufr->s3.data );

   return 0;
   }

/**
 * bufr_wr_section4
 * @english
 * ecrire la section 4 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section4
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 4 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section4(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
   int64_t len;
	if( writecb == NULL ) return errno=EINVAL, -1;

   bufr_write_int3b( writecb, cd, bufr->s4.len );

	bufr_write_octet( writecb, cd, 0 );	/* reserved */

   len = bufr->s4.len - bufr->s4.header_len;
   writecb ( cd, len, bufr->s4.data );

   return 0;
   }

/**
 * bufr_wr_section5
 * @english
 * ecrire la section 5 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_section5
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 5 d'un message BUFR
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_section5(bufr_write_callback writecb,
                                void *cd)
   {
	if( writecb == NULL ) return errno=EINVAL, -1;
	writecb( cd, 4, "7777" );
   return 0;
   }

 /*
 * nom: bufr_callback_write_message
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: termine l'encodage de message BUFR et l'ecrire
 *
 * parametres: 
 *      writecb   : writer callback
 *      client_data : client data for writer callback
 *      bufr : la structure de donnees BUFR
 */
int bufr_callback_write_message(bufr_write_callback writecb,
                         void* client_data, BUFR_Message *bufr)
   {
	if( writecb == NULL ) return errno=EINVAL, -1;

   if (bufr->len_msg > BUFR_MAX_MSG_LEN)
      {
      char errmsg[256];

      sprintf( errmsg, "Error: cannot create BUFR message with length %d octets\n", 
            bufr->len_msg  );
      fprintf( stderr, errmsg );
      return errno=EINVAL, -1;
      }

   bufr_wr_header_string( writecb, client_data, bufr );

   bufr_wr_section0( writecb, client_data, bufr );
   bufr_wr_section1( writecb, client_data, bufr );
   if (bufr->s1.flag  & 1)
      bufr_wr_section2( writecb, client_data, bufr );
   bufr_wr_section3( writecb, client_data, bufr );
   bufr_wr_section4( writecb, client_data, bufr );
   bufr_wr_section5( writecb, client_data );
   return 0;
   }

/**
 * bufr_write_fn
 * @english
 * internal callback to write to a byte-oriented buffer sink
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup bufr_io.c
  * name: bufr_write_fn
 *
 * author:  Chris Beauregard
 *
 * function: internal callback to write to a byte-oriented buffer sink
 *
 * parameters: 
 *      client_data : handle to output stream
 *      len : number of bytes to write
 *      buffer : data buffer to write from
 *
 * returns:
 *      number of bytes written. Zero or negative means
 *      some kind of error.

 */
static ssize_t bufr_write_fn( void *client_data, size_t len, const char *buffer)
	{
	/* write to a file descriptor, handling short writes correctly */
	int wrote = 0;
	while( len > 0 )
		{
		size_t rc = fwrite( buffer, 1, len, (FILE*) client_data );
		if( rc <= 0 )
			{
			/* EAGAIN happens when non-blocking I/O is used, EINTR
			 * happens when a signal triggers. Neither of them should
			 * interrupt writing.
			 */
			if( errno != EINTR && errno != EAGAIN ) break;
			errno = 0;
			continue;
			}
		wrote += rc;
		len -= rc;
		buffer += rc;
		}
	return wrote;
	}

/**
 * bufr_write_message
 * @english
 * termine l'encodage de message BUFR et l'ecrire au fichier
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_write_message
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: termine l'encodage de message BUFR et l'ecrire au fichier
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
int bufr_write_message(FILE* fp, BUFR_Message *bufr)
   {
	int rc = bufr_callback_write_message( bufr_write_fn, (void*) fp, bufr);
   fflush( fp );
   return rc;
   }

/**
 * bufr_getstring
 * @english
 * ajouter une chaine de caracteres comme donnees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_getstring
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ajouter une chaine de caracteres comme donnees
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 *      str  : la chaine a ajouter 
 *      len  : longueur de la chaine

 */
int bufr_getstring( BUFR_Message *bufr, char *str, int len)
   {
   int        i;
   uint64_t   c;
   int        errcode;

   for ( i = 0 ; i < len ; i++ )
      {
      c = bufr_getbits( bufr, 8, &errcode );
      str[i] = c & 0xff;
      if (errcode < 0) break;
      }
   str[len] = '\0';
   return errcode;
   }

/**
 * bufr_getbits
 * @english
 * ajouter des bits comme donnees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_getbits
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ajouter des bits comme donnees
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 *      val  : la valeur contenue dans un entier
 *      nbbits : nombre de bits (max: 32)
 *      errcode : return error code, less than 0 if error

 */
uint64_t bufr_getbits ( BUFR_Message *bufr, int nbbits, int *errcode)
   {
   unsigned char *ptrData ;
   int            p1 ;
   int            nbit_take, nbit_left, nbit_shift;
   int            bitno ;
   uint64_t       bits;
   char           errmsg[256];

   *errcode = 0;
   if (nbbits > 64)
      {
      sprintf( errmsg, "Warning: bufr_getbits( %d ), max_nbbits=64\n", nbbits );
      bufr_print_debug( errmsg );
      *errcode = -2;
      return 0;
      }

   ptrData = bufr->s4.current;
   bitno = bufr->s4.bitno;

	if( ptrData > (bufr->s4.data + bufr->s4.len) )
		{
      sprintf( errmsg, "Warning: bufr_getbits( %d ), out of bounds!\n", nbbits);
      bufr_print_debug( errmsg );
      *errcode = -1;
      return 0;
		}

/*
 * extraire par tranche de 8 bits

*/
   p1  = bitno % 8 ;
   nbit_take = nbbits < (8-p1) ? nbbits : (8-p1) ;
   nbit_left = nbbits - nbit_take ;
   nbit_shift = 8 - ( nbit_take + p1 ) ;
   bits  = ( *ptrData >> nbit_shift ) & ( (1ULL<<nbit_take) -1 ) ;

   bitno += nbit_take;
   bitno = bitno % 8;
   if (bitno == 0)
      {
	   if ( ptrData >= (bufr->s4.data + bufr->s4.len) )
		   {
         sprintf( errmsg, "Warning: bufr_getbits( %d ), out of bounds!\n", nbbits);
         bufr_print_debug( errmsg );
		   }
      else
         {
         ++ptrData;
         }
      }

/*
 * repeter l'insertion si le nombre de bits est plus grand que 8
 * repeat insertion if number of bits to insert is greater than 8 

*/
   while ( nbit_left > 0 )
      {
      nbit_take = nbit_left < 8 ? nbit_left : 8 ;
      nbit_left -= nbit_take ;
      nbit_shift = 8 - nbit_take ;
      bits = ( bits << nbit_take ) | ( ( *ptrData >> nbit_shift ) & ((1ULL<<nbit_take)-1) ) ;

      bitno += nbit_take;
      bitno = bitno % 8;
      if (bitno == 0)
         {
	      if( ptrData >= (bufr->s4.data + bufr->s4.len) )
		      {
            sprintf( errmsg, "Warning: bufr_getbits( %d ), out of bounds!\n", nbbits);
            bufr_print_debug( errmsg );
            *errcode = -1;
            bufr->s4.bitno = bitno;
            bufr->s4.current = ptrData;
            return bits;
		      }
         else
            {
            ++ptrData;
            }
         }
      }

   bitno = bitno % 8;
   bufr->s4.bitno = bitno;
   bufr->s4.current = ptrData;

   if (bufr_is_debug())
      {
      bufr_print_debug( " " );
      bufr_print_binary( errmsg, bits, nbbits );
      bufr_print_debug( errmsg );
      bufr_print_debug( " " );
      }

   return bits;
   }

/**
 * bufr_putstring
 * @english
 * ajouter une chaine de caracteres comme donnees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_putstring
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ajouter une chaine de caracteres comme donnees
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 *      str  : la chaine a ajouter 
 *      len  : longueur de la chaine

 */
void bufr_putstring( BUFR_Message *bufr, const char *str, int len)
   {
   int i;
   unsigned char  c;

   for ( i = 0 ; i < len ; i++ )
      {
      c = (unsigned char)str[i];
      bufr_putbits( bufr, (uint64_t)c, 8 );
      }
   }

/**
 * bufr_putbits
 * @english
 * ajouter des bits comme donnees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_putbits
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ajouter des bits comme donnees
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 *      val  : la valeur contenue dans un entier
 *      nbbits : nombre de bits (max: 32)

 */
void bufr_putbits ( BUFR_Message *bufr, uint64_t v1, int nbbits)
   {
	unsigned char *ptrData ;
	uint64_t       valeur ;
	int32_t        p1 ;
	int32_t        nbit_take, nbit_left, nbit_shift, nbit_move ;
   int32_t        bitno ;
   char           errmsg[256];

   if (nbbits <= 0) return;

   if (nbbits > 64)
      {
      sprintf( errmsg, "Warning: bufr_putbits() max_nbbits=64 < nbbits=%d\n", nbbits );
      bufr_abort( errmsg );
      }

	valeur = v1 ;

	ptrData = bufr->s4.current;
	bitno = bufr->s4.bitno;

	if ( bitno == 0 ) *ptrData = 0 ;
/*
 * inserer par tranche de 8 bits / insert by group of 8 bits 

*/
	p1  = bitno % 8 ;
	nbit_take = nbbits < (8-p1) ? nbbits : (8-p1) ;
	nbit_left = nbbits - nbit_take ;
	nbit_shift = nbit_left ;
	nbit_move = 8 -  bitno - nbit_take ;
	*ptrData |= ( ( valeur >> nbit_shift ) & ( (1ULL<<nbit_take) -1 ) ) << nbit_move ;

   bitno += nbit_take;
   bitno = bitno % 8;
   if (bitno == 0)
      {
      bufr->s4.filled += 1;
      ++ptrData;
      }
/*
 * repeter l'insertion si le nombre de bits est plus grand que 8
 * repeat insertion if number of bits to insert is greater than 8 

*/
	while ( nbit_left > 0 )
      {
		nbit_take = nbit_left < 8 ? nbit_left : 8 ;
		nbit_left -= nbit_take ;
		nbit_shift = nbit_left ;
		nbit_move = 8 - nbit_take ;
		*ptrData = ( ( valeur >> nbit_shift ) & ( (1ULL<<nbit_take) -1 ) ) << nbit_move ;
      bitno += nbit_take;
      bitno = bitno % 8;
      if (bitno == 0)
         {
         bufr->s4.filled += 1;
         ++ptrData;
		   *ptrData = 0 ;
         }
      }

   bufr->s4.bitno = bitno % 8;
	bufr->s4.current = ptrData;

   if (bufr_is_debug())
      {
      sprintf( errmsg, "bitno=%d  start=%x current=%x len=%d, at=%d, filled=%d max=%d\n", 
            bitno,  bufr->s4.data, ptrData, bufr->s4.len, ptrData-bufr->s4.data, bufr->s4.filled, bufr->s4.max_data_len );
      bufr_print_debug( errmsg );
      }

   if (bufr->s4.filled > bufr->s4.max_data_len) 
      {
/*
 * agrandir l'allocation de la section 4 si necessaire

*/
      bufr_alloc_sect4( bufr, bufr->s4.max_data_len + 4096 );
      }
   }

/**
 * bufr_put_bitstream
 * @english
 * ajouter des bits comme donnees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_put_bitstream
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ajouter des bits comme donnees
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR
 *      val  : la valeur contenue dans un entier
 *      nbbits : nombre de bits (max: 32)

 */
void bufr_put_bitstream ( BUFR_Message *bufr, const unsigned char *str, int nbbits)
   {
	unsigned char *ptrData ;
	int            valeur ;
	int            p1 ;
	int            nbit_take, nbit_left, nbit_shift, nbit_move ;
   int            nbit_remain;
   int           bitno ;
   int           pos;

/*
 * agrandir l'allocation de la section 4 si necessaire

*/
   if ((bufr->s4.filled+(nbbits/8+4096)) >= bufr->s4.max_data_len) 
      {
      bufr_alloc_sect4( bufr, bufr->s4.max_data_len + 4096 );
      }

	ptrData = bufr->s4.current;
	bitno = bufr->s4.bitno;
   pos = 0;
	valeur = str[pos++] ;
	if ( bitno == 0 ) *ptrData = 0 ;

/*
 * inserer par tranche de 8 bits / insert by group of 8 bits 

*/
   nbit_remain = (nbbits >= 8) ? 8 : nbbits ;
	p1  = bitno % 8 ;
	nbit_take = nbbits < (8-p1) ? nbbits : (8-p1) ;
	nbit_left = nbbits - nbit_take ;
   nbit_remain = nbit_remain - nbit_take;
   nbit_shift= nbit_remain;
	nbit_move = 8 -  bitno - nbit_take ;
	*ptrData |= ( ( valeur >> nbit_shift ) & ( (1ULL<<nbit_take) -1 ) ) << nbit_move ;
   bitno = (bitno + nbit_take) % 8;
/*
 * repeter l'insertion si le nombre de bits est plus grand que 8
 * repeat insertion if number of bits to insert is greater than 8 

*/
	while ( nbit_left > 0 )
      {
      if (bitno == 0)
         {
		   ++ptrData;
		   *ptrData = 0 ;
         bufr->s4.filled += 1;
         }
      if (nbit_remain == 0)
         {
         valeur = str[pos++];
         nbit_remain = (nbit_left < 8) ? nbit_left : 8;
         }
		nbit_take = nbit_remain;
      nbit_remain -= nbit_take;
		nbit_left -= nbit_take ;
      nbit_shift= 0;
		nbit_move = 8 - nbit_take ;
		*ptrData = ( ( valeur >> nbit_shift ) & ( (1ULL<<nbit_take) -1 ) ) << nbit_move ;
      bitno = nbit_take % 8;
      }

   if (bitno == 0)
      {
		++ptrData;
		*ptrData = 0 ;
      bufr->s4.filled += 1;
      }
   bufr->s4.bitno = bitno;
	bufr->s4.current = ptrData;
   }

/**
 * bufr_print_output
 * @english
 * ecrire un message de sortie
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_print_output
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction:  ecrire un message de sortie
 *
 * parametres:
 *    msg :  message a afficher

 */
void bufr_print_output(const char *msg)
   {
/*
 * end of message means closing file pointer

*/
   if (msg == NULL)
      {
      if (output_fp)
         {
         fclose( output_fp );
         output_fp = NULL;
         }
      return;
      }

   if (output_filename)
      {
      if (output_fp == NULL)
         output_fp = fopen( output_filename, "a+" );
      if (output_fp)
         {
         fprintf( output_fp, "%s", msg );
         return;
         }
      }

   fprintf( stdout, "%s", msg );
   }

/**
 * bufr_ile
 * @english
 * ecrire un message d'erreur
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_ile
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction:  ecrire un message d'erreur
 *
 * parametres:
 *    msg :  message a afficher

 */
/**
 * @english
 * This call will redirect output of the decoder into a file.
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * ecrire un message d'erreur
 * @todo translate to French
 * @param msg message a afficher
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io debug
 */
void bufr_set_output_file(const char *filename)
   {
   if (output_filename)
      {
      free( output_filename );
      output_filename = NULL;
      }
   if (output_fp)
      {
      fclose( output_fp );
      output_fp = NULL;
      }

   if (filename == NULL) return;

   output_filename = strdup( filename );
   output_fp = fopen( output_filename, "w" );
   }


/**
 * bufr_abort
 * @english
 * pour quitter en catastrophe
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_abort
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction:  pour quitter en catastrophe
 *
 * parametres:
 *    msg :  message a afficher avant de quitter

 */
void bufr_abort(const char *msg)
   {
   if (udf_abort) 
      {
/*
 * user override of the abort function

*/
      (*udf_abort)( msg );
      return;
      } 
   else 
      {
      fprintf( stderr, "%s\n", msg );
      bufr_print_debug( msg );
      exit(1);
      }
   }

/**
 * @english
 *    bufr_print_debug( string )
 *    (char *str)
 * This sends the string received to the file specified by
 * bufr_set_debug_file().
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * ecrire un message d'erreur
 * @param msg message a afficher
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io debug
 */
void bufr_print_debug(const char *msg)
   {
/*
 * end of message means closing file pointer

*/
   if (msg == NULL)
      {
      if (debug_fp) 
         {
         fclose( debug_fp );
         debug_fp = NULL;
         }
      return;
      }

   if (debug_filename)
      {
      if (debug_fp == NULL)
         debug_fp = fopen( debug_filename, "a+" );
      if (debug_fp)
         {
         fprintf( debug_fp, "%s", msg );
#if DEBUG
         fclose( debug_fp );
         debug_fp = NULL;
#endif
         return;
         }
      }
   fprintf( stderr, "%s", msg );
   }

/**
 * @english
 *    bufr_set_debug_file( str_debug )
 *    (int mode)
 * This call will redirect output of the debug printout into a file.
 * @warning Not thread-safe
 * @return void
 * @endenglish
 * @francais
 * ecrire un message d'erreur
 * @todo translate to French
 * @param msg message a afficher
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io debug
 */
void bufr_set_debug_file(const char *filename)
   {
   if (debug_filename)
      {
      free( debug_filename );
      debug_filename = NULL;
      }
   if (debug_fp) 
      {
      fclose( debug_fp );
      debug_fp = NULL;
      }

   if (filename == NULL) return;

   debug_filename = strdup( filename );

   if (bufr_is_debug())
      debug_fp = fopen( debug_filename, "w" );
   }

/**
 * bufr_set_abort
 * @english
 * definir la routine pour quitter en catastrophe
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_set_abort
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: definir la routine pour quitter en catastrophe
 *
 * parametres:
 *    uabort : pointeur au routine a appeler 

 */
void bufr_set_abort( void (*uabort)(const char *msg) )
   {
   udf_abort = uabort;
/**
 * bufr_is_debug
 * @english
 * retourne le code d'erreur du logiciel
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
    tabled_set_abort( uabort ); */
   }

/*
 * nom: bufr_is_debug
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: retourne le code d'erreur du logiciel
 *
 * parametres:

 */
int bufr_is_debug(void) 
   {
   return (debugmode != 0) ? 1 : 0;
   }

/**
 * bufr_is_verbose
 * @english
 * retourne le code d'erreur du logiciel
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_is_verbose
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: retourne le code d'erreur du logiciel
 *
 * parametres:

 */
int bufr_is_verbose(void) 
   {
   return (verbosemode != 0) ? 1 : 0;
   }

/**
 * bufr_set_debug
 * @english
 * etablit le code d'erreur du logiciel
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_set_debug
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: etablit le code d'erreur du logiciel
 *
 * parametres:

 */
void bufr_set_debug(int mode) 
   {
   debugmode = mode;
   if (mode == 0)
      {
      if (verbosemode == -1) verbosemode = 0;
      }
   else
      {
/*
 * turn on verbose mode during debug mode

*/
      if (verbosemode == 0) verbosemode = -1;
      }
   }

/**
 * bufr_set_verbose
 * @english
 * etablit le code informative du logiciel
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_set_verbose
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: etablit le code informative du logiciel
 *
 * parametres:

 */
void bufr_set_verbose(int mode) 
   {
/*
 * verbose stays on when in debug mode

*/
   if (debugmode)
      {
      if (mode == 0)
         {
         verbosemode = -1;
         return;
         }
      }

   verbosemode = mode;
   }

/**
 * bufr_write_int3b
 * @english
 * ecrit un entier de 3 octets 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_write_int3b
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrit un entier de 3 octets 
 *
 * parametres:

 */
static void bufr_write_int3b ( bufr_write_callback writecb,
                                void *cd, int value )
   {
   bufr_write_octet ( writecb, cd, (value >> 16) & 0xff );
   bufr_write_octet ( writecb, cd, (value >> 8) & 0xff );
   bufr_write_octet ( writecb, cd, value & 0xff );
   }

/**
 * bufr_write_int2b
 * @english
 * ecrit un entier de 2 octets 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_write_int2b
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrit un entier de 2 octets 
 *
 * parametres:

 */
static void bufr_write_int2b ( bufr_write_callback writecb,
                                void *cd, int value )
   {
   bufr_write_octet ( writecb, cd, (value >> 8) & 0xff );
   bufr_write_octet ( writecb, cd, value & 0xff );
   }

/**
 * bufr_read_int3b
 * @english
 * lire un entier de 3 octets 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_read_int3b
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire un entier de 3 octets 
 *
 * parametres:

 */
static int bufr_read_int3b ( bufr_read_callback readcb, void *cd )
   {
	unsigned char c;
   uint64_t      c64;
	int val = 0;

	if( bufr_read_octet( readcb, cd, &c ) != 1 ) return -1;
   c64 = c;
	val |= (c64 << 16);
	if( bufr_read_octet( readcb, cd, &c ) != 1 ) return -1;
   c64 = c;
	val |= (c64 << 8);
	if( bufr_read_octet( readcb, cd, &c ) != 1 ) return -1;
	val |= c;
	return val;
   }

/**
 * bufr_read_int2b
 * @english
 * lire un entier de 2 octets 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_read_int2b
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire un entier de 2 octets 
 *
 * parametres:

 */
static int bufr_read_int2b ( bufr_read_callback readcb, void *cd )
   {
	unsigned char c;
   uint64_t      c64;
	int val = 0;

	if( bufr_read_octet( readcb, cd, &c ) != 1 ) return -1;
   c64 = c;
	val |= (c64 << 8);
	if( bufr_read_octet( readcb, cd, &c ) != 1 ) return -1;
	val |= c;
	return val;
   }


/**
 * bufr_seek_msg_start
 * @english
 * seek start pos of message in BUFR file
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_seek_msg_start
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: seek start pos of message in BUFR file
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie

 */
static int bufr_seek_msg_start( bufr_read_callback readcb, void *cd, char **tagstr, int *len )
   {
   unsigned char  c;
   int  notfound= 1;
   char  *str, *str2;
   int    i, tagsize;
   int    rtrn = -1;

   *tagstr = NULL;
   *len = 0;
   tagsize = 64;
   str = (char *)malloc( (tagsize+1) * sizeof(char) );
   i = 0;

	if( bufr_read_octet( readcb, cd, &c ) != 1 ) goto bailout ;
   if ( c != '\004' )
      append_char_to_string( &str, &tagsize, &i, c );
   while ( notfound )
      {
      while ( (c != 'B') )
         {
			if( bufr_read_octet( readcb, cd, &c ) != 1 ) goto bailout ;
         if (c != '\004')
            append_char_to_string( &str, &tagsize, &i, c );
         }
      if (c == 'B')
         {
			if( bufr_read_octet( readcb, cd, &c ) != 1 ) goto bailout ;
         append_char_to_string( &str, &tagsize, &i, c );
         if (c == 'U')
            {
				if( bufr_read_octet( readcb, cd, &c ) != 1 ) goto bailout ;
            append_char_to_string( &str, &tagsize, &i, c );
            if (c == 'F')
               {
					if( bufr_read_octet( readcb, cd, &c ) != 1 ) goto bailout ;
               append_char_to_string( &str, &tagsize, &i, c );
               if (c == 'R')
                  {
                  notfound = 0;
                  i -= 4; /* remove the string "BUFR" */
                  }
               }
            }
         }
      }

/*
 * remove control characters

*/
   if (i > 0)
      {
      *len = i;
      *tagstr = str_schar2oct ( str, len, &tagsize );
      }
   else
      {
      *tagstr = NULL;
      *len = 0;
      }

   rtrn = (notfound == 0) ? 1 : 0 ;
bailout:
   free( str );

	if( notfound ) errno = ENOMSG;
   return rtrn;
   }

/**
 * bufr_callback_read_message
 * @english
 * read a BUFR report from a callback "source"
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_callback_read_message
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: read a BUFR report from a callback "source"
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      rtrn :     pointer return handle

 */
int bufr_callback_read_message( bufr_read_callback readcb, void *cd,
                       BUFR_Message **rtrn )
   {
   BUFR_Message  *bufr;
   char          *tagstr;
   int            len;

	if( readcb == NULL ) return errno=EINVAL, -1;

   *rtrn = NULL;

   if (bufr_seek_msg_start( readcb, cd, &tagstr, &len ) <= 0) return -1;

   bufr = bufr_create_message ( 4 );
   if (tagstr)
      {
      bufr->header_string = tagstr;
      bufr->header_len = len;
      }

	if( bufr == NULL ) return -1;
   if (bufr_rd_section0( readcb, cd, bufr ) < 0)
      {
      bufr_free_message( bufr );
      return -1;
      }

   if (bufr_rd_section1( readcb, cd , bufr ) < 0) 
      {
      bufr_free_message( bufr );
      return -1;
      }

   if (bufr_rd_section2( readcb, cd , bufr ) < 0) 
      {
      bufr_free_message( bufr );
      return -1;
      }

   if (bufr_rd_section3( readcb, cd , bufr ) < 0) 
      {
      bufr_free_message( bufr );
      return -1;
      }

   bufr_decode_sect3( bufr );

   if (bufr_rd_section4( readcb, cd , bufr ) < 0) 
      {
      bufr_free_message( bufr );
      return -1;
      }

   if (bufr_rd_section5( readcb, cd ) < 0) 
      {
      bufr_free_message( bufr );
      return -1;
      }

   *rtrn = bufr;
   return 1;
   }

/**
 * bufr_read_fn
 * @english
 * internal callback to read from a byte-oriented buffer sink
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup bufr_io.c
  * name: bufr_read_fn
 *
 * author:  Chris Beauregard
 *
 * function: internal callback to read from a byte-oriented buffer sink
 *
 * parameters: 
 *      client_data : handle to input stream
 *      len : number of bytes to read
 *      buffer : data buffer to read into
 *
 * returns:
 *      number of bytes read. Zero means EOF, negative means
 *      some kind of error.

 */
static ssize_t bufr_read_fn( void *client_data, size_t len, char *buffer)
	{
	/* read from a stream, handling short writes correctly */
	int got = 0;
	while( len > 0 )
		{
		size_t rc = fread( buffer, 1, len, (FILE*) client_data );
		if( rc == 0 ) break;	/* EOF */
		if( rc <= 0 )
			{
			/* EAGAIN happens when non-blocking I/O is used, EINTR
			 * happens when a signal triggers. Neither of them should
			 * interrupt writing.
			 */
			if( errno != EINTR && errno != EAGAIN ) break;
			errno = 0;
			continue;
			}
		got += rc;
		len -= rc;
		buffer += rc;
		}
	return got;
	}

/**
 * @english
 *    rtrn = bufr_read_message( fpBufr, &msg )
 *    (FILE *fpBufr, BUFR_Message **rtrn)
 * This is a low-level I/O call to read all sections of the BUFR message
 * from 0 to 5. It reads the entire message into the address of the pointer
 * passed from the second argument (â~@~\msgâ~@~]).
 * @warning When the message is no longer needed, the storage should be
 * freed by calling bufr_free_message.
 * @return int, It returns a flag code for errors that would be greater
 * than zero if there is no error. If it returns 0 then there are no more
 * messages in the file; if it returns less than zero then there is an
 * error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @param fp   pointeur au fichier de sortie
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io decode message
 */
int bufr_read_message( FILE *fp, BUFR_Message **rtrn )
   {
	int rc;
	fpos_t pos;
	int seekable = !fgetpos( fp, &pos );
   if (feof( fp )) return 0;
	rc = bufr_callback_read_message( bufr_read_fn, (void*) fp, rtrn );
	if( rc <= 0 && seekable )
		{
		/* if fgetpos worked and the message read failed, try to
		 * reposition back to where we started.
		 */
		fsetpos( fp, &pos );
		} 
	return rc;
	}

/**
 * bufr_rd_section0
 * @english
 * lire la section 0 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section0
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire la section 0 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      bufr : la structure de donnees BUFR

 */
static int bufr_rd_section0(bufr_read_callback readcb, void *cd, BUFR_Message *bufr)
   {
   unsigned char   c;

   if (readcb == NULL) return errno=EINVAL, -1;

	/*
	 * no need to read "BUFR", this is already read by bufr_seek_msg_start
	 */
 
   bufr->len_msg = bufr_read_int3b( readcb, cd );
   if (bufr->len_msg < 0) return -1;

	if( 1 != bufr_read_octet( readcb, cd, &c ) ) return -1;
   if (bufr->edition != c)
      bufr_init_header( bufr, c );

   if (bufr_is_debug())
      {
		char buffer[256];
      sprintf( buffer, "### Reading BUFR edition: %d\n", bufr->edition );
      bufr_print_debug( buffer );
      sprintf( buffer, "### Message length: %d\n", bufr->len_msg );
      bufr_print_debug( buffer );
      }

   return bufr->len_msg;
   }

/**
 * bufr_rd_section1
 * @english
 * lire la section 1 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section1
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire la section 1 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      bufr : la structure de donnees BUFR

 */
static int bufr_rd_section1(bufr_read_callback readcb, void *cd,
                            BUFR_Message *bufr)
   {
   int           c;
	unsigned char          octet;
   char          errmsg[256];

   c = bufr_read_int3b( readcb, cd );
   if (c < 0) return -1;
   if (c != bufr->s1.len)
      {
      if ((bufr->edition >= 2)&&(c > bufr->s1.len))
         {
         bufr->s1.data_len = c - bufr->s1.header_len;
         if (bufr_is_verbose())
            {
            sprintf( errmsg, "### Section1 contains additionnal data length=%d octets\n", 
                     bufr->s1.data_len );
            bufr_print_debug( errmsg );
            }
         }
      else
         {
         if (bufr_is_debug())
            {
            sprintf( errmsg, "Warning: length of Section 1 is %d, should have been %d\n", c, bufr->s1.len );
            bufr_print_debug( errmsg );
            }
         return -1;
         }
      }


	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.bufr_master_table = octet;

   if (bufr->edition == 3)
      {
		if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      bufr->s1.orig_sub_centre = octet;
		if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      bufr->s1.orig_centre = octet;
      }
   else if ((bufr->edition == 2)||(bufr->edition >= 4))
      {
      bufr->s1.orig_centre = bufr_read_int2b( readcb, cd );
      if (bufr->s1.orig_centre < 0) return -1;
      }

   if (bufr->edition >= 4)
      {
      bufr->s1.orig_sub_centre = bufr_read_int2b( readcb, cd );
      if (bufr->s1.orig_sub_centre < 0) return -1;
      }

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.upd_seq_no = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.flag = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.msg_type = octet;

   if (bufr->edition >= 4)
      {
		if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      bufr->s1.msg_inter_subtype = octet;
      }

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.msg_local_subtype = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.master_table_version = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.local_table_version = octet;

   if (bufr->edition >= 4)
      {
      bufr->s1.year = bufr_read_int2b( readcb, cd );
      if (bufr->s1.year < 0) return -1;
      }
   else
      {
		if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      bufr->s1.year = octet;
      }

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.month = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.day = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.hour = octet;

	if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
   bufr->s1.minute = octet;

   if (bufr->edition >= 4)
      {
		if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      bufr->s1.second = octet;
      }

   if ((bufr->edition >= 2)&&(bufr->s1.data_len > 0))
      {
      int len = bufr->s1.data_len;

      if ( bufr->s1.data != NULL ) free( bufr->s1.data );
      bufr->s1.data = (char *)malloc( len * sizeof(char) );
		if( bufr->s1.data == NULL ) return -1;
      if (readcb( cd, len, bufr->s1.data ) != len ) return -1;
      }
   else if ((bufr->edition == 2)||(bufr->edition == 3))
      {
      /* discard padding octets */
      if( 1 != bufr_read_octet( readcb, cd, &octet ) ) return -1;
      }

   return bufr->s1.len;
   }

/**
 * bufr_rd_section2
 * @english
 * ecrire la section 2 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section2
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 2 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      bufr : la structure de donnees BUFR

 */
static int bufr_rd_section2(bufr_read_callback readcb, void *cd,
                            BUFR_Message *bufr)
   {
   unsigned int len, len2;
   char         errmsg[256];
   unsigned char          c;

   if (readcb == NULL) return errno=EINVAL, -1;
   if (bufr == NULL) return errno=EINVAL, -1;

   if ((bufr->s1.flag & 1)==0) return 0;

   bufr->s2.len = bufr_read_int3b( readcb, cd );

   /* discard */
	if( 1 != bufr_read_octet( readcb, cd, &c ) ) return -1;

   bufr->s2.data_len = len2 = len = bufr->s2.len - bufr->s2.header_len;

   if (bufr_is_verbose())
      {
      sprintf( errmsg, "### Section2 contains additionnal data length=%d octets\n", 
            bufr->s2.data_len );
      bufr_print_debug( errmsg );
      }

   if ( bufr->s2.data != NULL ) free( bufr->s2.data );
/*
 * even octets padding

*/
   if ( bufr->edition == 3 )
      {
      len2 = (bufr->s2.len % 2) ? len + 1 : len;
      }

   bufr->s2.data = (char *)malloc( len2 * sizeof(char) );
	if( bufr->s2.data == NULL ) return -1;
   if (readcb( cd, len, bufr->s2.data ) != len ) return -1;
/*
 * even octets padding

*/
   if (len < len2)
      {
      bufr->s2.data[len2-1] = 0; 
      }
   return len;
   }

/**
 * bufr_rd_section3
 * @english
 * lire la section 3 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section3
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire la section 3 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      bufr : la structure de donnees BUFR

 */
static int bufr_rd_section3(bufr_read_callback readcb, void *cd,
                            BUFR_Message *bufr)
   {
   unsigned int  len, len2;
   unsigned char  c;

   bufr->s3.len = bufr_read_int3b( readcb, cd );
   if (bufr->s3.len < 0) return -1;

   /* discard */
	if( 1 != bufr_read_octet( readcb, cd, &c ) ) return -1;

   bufr->s3.no_data_subsets = bufr_read_int2b( readcb, cd );

	if( 1 != bufr_read_octet( readcb, cd, &c ) ) return -1;
   bufr->s3.flag = c;

   len2 = len = bufr->s3.len - bufr->s3.header_len;
   if (bufr->edition == 3) 
      {
      len2 = (bufr->s3.len % 2) ? len + 1 : len;
      }
   bufr->s3.data = (char *)malloc( sizeof(char) * len2 );
	if( bufr->s3.data == NULL ) return -1;
   bufr->s3.max_len = len2;
   if (readcb( cd, len, bufr->s3.data ) != len ) return -1;
   if (len2 != len)
      {
      bufr->s3.data[len2-1] = 0;
      }

   return len;
   }

/**
 * bufr_rd_section4
 * @english
 * ecrire la section 4 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section4
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 4 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data
 *      bufr : la structure de donnees BUFR

 */
static uint64_t bufr_rd_section4(bufr_read_callback readcb, void *cd,
                            BUFR_Message *bufr)
   {
   uint64_t        len;
   unsigned char   c;

   bufr->s4.len = bufr_read_int3b( readcb, cd );
   if (bufr->s4.len < 0) return -1;

   /* discard */
	if( 1 != bufr_read_octet( readcb, cd, &c ) ) return -1;

   len = bufr->s4.len - bufr->s4.header_len;

   bufr_alloc_sect4( bufr, len );

	if( bufr->s4.data == NULL ) return -1;
   if (readcb( cd, len, bufr->s4.data ) != len ) return -1;

   return len;
   }

/**
 * bufr_rd_section5
 * @english
 * ecrire la section 5 d'un message BUFR
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_rd_section5
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: ecrire la section 5 d'un message BUFR
 *
 * parametres: 
 *      readcb   : read callback
 *      cd   :     read callback client data

 */
static int bufr_rd_section5( bufr_read_callback readcb, void *cd )
   {
   char buffer[32];

   if (readcb( cd, 4, buffer ) != 4) return -1;

   if (strncmp( buffer, "7777", 4 ) != 0)
      {
      bufr_print_debug( "Warning: BUFR message not ending with 7777\n" );
      return -1;
      }

   return 0;
   }

/**
 * bufr_decode_sect3
 * @english
 * encoder la section 3
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_decode_sect3
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: encoder la section 3
 *
 * parametres: 
 *      bufr : la structure de donnees BUFR

 */
int bufr_decode_sect3(BUFR_Message *bufr)
   {
   uint32_t       c32;
   unsigned short code;
   int            i, count;
   int            descriptor;
   unsigned char *ptr;
   int            f, x, y;
   char           errmsg[256];

   count = (bufr->s3.len - bufr->s3.header_len) / 2;

   if (bufr_is_verbose()||bufr_is_debug())
      {
      sprintf( errmsg, "### Decoding Section 3: contains %d items\n", count );
      bufr_print_debug( errmsg );
      }

   ptr = (unsigned char *)bufr->s3.data;
   for ( i = 0 ; i < count ; i++ ) 
      {
      c32 = *ptr++;
      code = c32 << 8;
      code |= *ptr++;
      f = (code >> 14)&3;
      x = (code >> 8)&63;
      y = code & 255;
      descriptor = bufr_fxy_to_descriptor( f, x, y );
      if (bufr_is_debug())
         {
         sprintf( errmsg, "   #%.3d: %d --> %.6d\n", i+1, code, descriptor );
         bufr_print_debug( errmsg );
         }
      arr_add( bufr->s3.desc_list, (char *)&descriptor );
      }

   return count;
   }

/**
 * @english
 * This is a low-level function called within bufr_encode_message; this
 * uses len calculated from section 3 in order to allocate storage
 * for section 4.
 * @return void
 * @endenglish
 * @francais
 * allouer l'espace memoire pour la section 4
 * @todo translate to French
 * @param bufr la structure de donnees BUFR
 * @param len longueur en octets
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_alloc_sect4(BUFR_Message *bufr, unsigned int len)
   {
   uint64_t llen = len + 10;

   if (bufr->s4.max_len < llen) 
      {
      if (bufr->s4.data == NULL) 
         {
         bufr->s4.data = (unsigned char *)malloc(llen * sizeof(char));
         bufr->s4.current = bufr->s4.data;
         }
      else 
         {
         int64_t  offset;

         offset = bufr->s4.current - bufr->s4.data;
         bufr->s4.data = (unsigned char *)realloc(bufr->s4.data, llen * sizeof(char));
         bufr->s4.current = bufr->s4.data + offset;
         }
      bufr->s4.max_len = llen;
      bufr->s4.max_data_len = len;
      }
   bufr->s4.len = bufr->s4.header_len + len;   
   }

/*************************************************************************/
struct bufr_mem {
	char* mem;
	uint64_t pos;
	uint64_t max_len;
};

static ssize_t bufr_memread_fn( void* cd, size_t len, char* buf )
	{
	struct bufr_mem* mem = (struct bufr_mem*) cd;
	if( mem->pos + len >= mem->max_len )
		{
		len = mem->max_len - mem->pos;
		}
	memcpy( buf, mem->mem + mem->pos, len );
	mem->pos += len;
	return len;
	}

static ssize_t bufr_memwrite_fn( void* cd, size_t len, const char* buf )
	{
	struct bufr_mem* mem = (struct bufr_mem*) cd;
	if( mem->pos + len >= mem->max_len )
		{
		len = mem->max_len - mem->pos;
		}
	memcpy( mem->mem + mem->pos, buf, len );
	mem->pos += len;
	return len;
	}

/**
 * @english
 * @brief read a BUFR report from a memory buffer
 *
 * This function reads one message at a time for the buffer. For
 * subsequent calls, the caller should increment the buffer position by
 * the number of bytes read from the previous call. i.e.
 *
 * @param mem buffer to read from
 * @param mem_len maximum number of bytes in the buffer
 * @return number of bytes read (including initial skipped bytes) on
 *      success, <=0 on failure.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Chris Beauregard
 *
 *       bufr_message *msg;
 *       size_t buflen = ...
 *       char *buffer = ...
 *       ssize_t rtrn;
 *       while( (rtrn = bufr_memread_message( buffer, buflen, &msg)) > 0)
 *       	{
 *       	   buffer += rtrn;
 *       	   buflen -= rtrn;
 *
 *       	   ... process msg ...
 *       	}
 *
 * @ingroup decode message
 */
ssize_t bufr_memread_message( const char *mem, size_t mem_len,
                             BUFR_Message **rtrn )
   {
	int rc;
	struct bufr_mem cd;
	cd.mem = (char*) mem;
	cd.pos = 0;
	cd.max_len = mem_len;
	rc = bufr_callback_read_message( bufr_memread_fn, (void*) &cd, rtrn );
	if( rc <= 0 ) return rc;
	return cd.pos;
	}

/**
 * @english
 * @brief write a BUFR message into a memory buffer
 *
 * Estimating the size of the buffer is left as an exercise for the caller.
 *
 * @param mem buffer to write into
 * @param mem_len maximum number of bytes to write
 * @return number of bytes written, or zero/negative on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Chris Beauregard
 * @ingroup encode message
 */
ssize_t bufr_memwrite_message(char *mem, size_t mem_len, BUFR_Message *bufr)
   {
	int rc;
	struct bufr_mem cd;
	cd.mem = mem;
	cd.pos = 0;
	cd.max_len = mem_len;
	rc = bufr_callback_write_message( bufr_memwrite_fn, (void*) &cd, bufr );
	if( rc <= 0 ) return rc;
	return cd.pos;
   }

void bufr_write_header_of_message( BUFR_Message *msg )
   {
   }
/**
 * bufr_wr_header_string
 * @english
 * write the header string preceding a message
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_io.c
  * nom: bufr_wr_header_string
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: write the header string preceding a message
 *
 * parametres: 
 *      fp   : pointeur au fichier de sortie
 *      bufr : la structure de donnees BUFR

 */
static int bufr_wr_header_string(bufr_write_callback writecb,
                                void *cd, BUFR_Message *bufr)
   {
   char *str;
   int   len;

	if( writecb == NULL ) return errno=EINVAL, -1;

   if ( bufr->header_string == NULL ) return 0;

   str = bufr->header_string;
   len = bufr->header_len;

   str = str_oct2char( bufr->header_string, &len );
	writecb( cd, len, str );
   free( str );

   return 0;
   }

