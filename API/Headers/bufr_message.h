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
 
 *  file      :  BUFR_MESSAGE.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR MESSAGES
 *
 *
 */

#ifndef _bufr_message_h_
#define _bufr_message_h_

#include "bufr_array.h"
#include "bufr_tables.h"

#include <math.h>
#include <limits.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  BUFR_MAX_MSG_LEN       16777216

#define  BUFR_FLAG_OBSERVED     (1<<7)
#define  BUFR_FLAG_COMPRESSED   (1<<6)
#define  BUFR_FLAG_INVALID      (1<<5)

#define  BUFR_SET_COMPRESSED(b)      ((b)->s3.flag |= BUFR_FLAG_COMPRESSED)
#define  BUFR_SET_OBSERVED(b)        ((b)->s3.flag |= BUFR_FLAG_OBSERVED)
#define  BUFR_SET_OTHER_DATA(b)      ((b)->s3.flag &= ~(BUFR_FLAG_OBSERVED))
#define  BUFR_SET_UNCOMPRESSED(b)    ((b)->s3.flag &= ~(BUFR_FLAG_COMPRESSED))
#define  BUFR_SET_INVALID(b)         ((b)->s3.flag |= BUFR_FLAG_INVALID)

#define  BUFR_SET_MSGDTYPE(b,mt)     (b)->s1.msg_type = (mt)
#define  BUFR_SET_NB_DATASET(b,nb)   (b)->s3.no_data_subsets = (nb)

#define  BUFR_IS_COMPRESSED(b)       ((b)->s3.flag & BUFR_FLAG_COMPRESSED)
#define  BUFR_IS_INVALID(b)          ((b)->s3.flag & BUFR_FLAG_INVALID)

#define  MSGDTYPE_SURFACE_LAND           0
#define  MSGDTYPE_SURFACE_SEA            1
#define  MSGDTYPE_VERT_SOUNDING_OTH_SAT  2
#define  MSGDTYPE_VERT_SOUNDING_SAT      3
#define  MSGDTYPE_1LVL_UA_OTH_SAT        4
#define  MSGDTYPE_1LVL_UA_SAT            5
#define  MSGDTYPE_RADAR                  6
#define  MSGDTYPE_SYNOPTIC_FEATURE       7
#define  MSGDTYPE_PHYS_CHEM_CONST        8
#define  MSGDTYPE_DISPERSAL_TRANSPORT    9
#define  MSGDTYPE_RADIOLOGICAL           10
#define  MSGDTYPE_TABLES_REPLACE_UPDATE  11
#define  MSGDTYPE_SURFACE_SAT            12
#define  MSGDTYPE_STATUS_INFO            20
#define  MSGDTYPE_RADIANCES_SAT          21
#define  MSGDTYPE_OCEANOGRAPHIC          31
#define  MSGDTYPE_IMAGE                  101
#define  MSGDTYPE_FOR_LOCAL_USE          255
#define  MSGDTYPE_FEXPU_LLIMIT           240
#define  MSGDTYPE_FEXPU_ULIMIT           254

typedef struct _Section0
   {
   int   len;
   } BufrSection0;

typedef struct _Section1
   {
   int   len;
   int   header_len;
   short bufr_master_table;
   int   orig_centre;
   short orig_sub_centre;
   short upd_seq_no;
   short flag;
   short msg_type;
   short msg_inter_subtype;
   short msg_local_subtype;
   short master_table_version;
   short local_table_version;
   short year;
   short month;
   short day;
   short hour;
   short minute;
   short second;
   int   data_len;
   unsigned char *data;
   } BufrSection1;

typedef struct _Section2
   {
   int             len;
   int             header_len;
   int             data_len;
   unsigned char  *data;
   } BufrSection2;

typedef struct _Section3
   {
   int           len;
   int           header_len;
   int           max_len;
   int           no_data_subsets;
   unsigned char flag;
   IntArray      desc_list;  /* array of int */
   unsigned char *data;
   } BufrSection3;

typedef struct _Section4
   {
   unsigned int   len;
   int            header_len;
   unsigned int   max_len;
   unsigned int   filled;     
   unsigned int   max_data_len;     
   unsigned char *data;
   unsigned char *current;
   unsigned short bitno;
   } BufrSection4;

typedef struct _Section5
   {
   int   len;
   } BufrSection5;

typedef struct _DBufr
   {
   int           edition;
   unsigned int  len_msg;

   BufrSection0 s0;
   BufrSection1 s1;
   BufrSection2 s2;
   BufrSection3 s3;
   BufrSection4 s4;
   BufrSection5 s5;
   char *header_string;
   int   header_len;
   } BUFR_Message;

/*
 * constructeurs et destructeurs
 */

extern BUFR_Message  *bufr_create_message  ( int edition );
extern void           bufr_free_message    ( BUFR_Message *bufr );
extern void           bufr_print_message   ( BUFR_Message *bufr, void (*print_proc)(const char *) );

/*
 * fonctions outils de bas niveau
 */
extern void           bufr_init_header     ( BUFR_Message *bufr, int edition );
extern void           bufr_begin_message   ( BUFR_Message *bufr );
extern void           bufr_end_message     ( BUFR_Message *bufr );

extern void           bufr_set_gmtime      ( BufrSection1 *s1 );

extern void           bufr_init_sect1      ( BufrSection1 *s1 );
extern void           bufr_copy_sect1      ( BufrSection1 *dest, BufrSection1 *src );
extern void           bufr_set_time_sect1  ( BufrSection1 *s1, time_t );

extern void           bufr_sect2_set_data  ( BUFR_Message *r,
                                             const char *data,
															int len);

#ifdef __cplusplus
}
#endif

#endif

