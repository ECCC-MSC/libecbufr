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
 
 *  file      :  BUFR_TEMPLATE.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR TEMPLATE
 *               A TEMPLATE IS NEEDED FOR INSTANTIATING A DATASET
 *
 */

#ifndef _bufr_template_h
#define _bufr_template_h

#include  <stdio.h>
#include  <math.h>
#include  "bufr_io.h"
#include  "bufr_tables.h"
#include  "bufr_desc.h"
#include  "bufr_value.h"

#ifdef __cplusplus
extern "C" {
#endif

#define    HAS_DELAYED_REPLICATION   0x1
#define    HAS_IEEE_FP               0x2


typedef struct bufr_tdescriptor
   {
   int          descriptor;
   BufrValue  **values;
   int          nbval;
   } BufrDescValue;

typedef ArrayPtr  BufrDescValueArray;

typedef struct bufr_template
   {
   BufrDescValueArray      codets;          /* array of BufrDescValue */
   int                     edition;
   BufrDescriptorArray     gabarit;         /* expanded, except delayed replication */
   BUFR_Tables            *tables;
   int                     flags;
   EntryTableBArray        ddo_tbe;         /* Data Descriptor Operators Table B Entries */
   } BUFR_Template;



extern BUFR_Template   *bufr_create_template        ( BufrDescValue *, int nb, BUFR_Tables *, int );
extern void             bufr_template_add_DescValue ( BUFR_Template *, BufrDescValue *, int nb );
extern int              bufr_finalize_template      ( BUFR_Template *tmplt );
extern void             bufr_free_template          ( BUFR_Template * );
extern BUFR_Template   *bufr_copy_template          ( BUFR_Template * );

extern BUFR_Template   *bufr_load_template          ( const char *filename, BUFR_Tables *mtbls );
extern int              bufr_save_template          ( const char *filename, BUFR_Template *tmplt );

extern int              bufr_compare_template       ( BUFR_Template *, BUFR_Template * );

extern void             bufr_init_DescValue         ( BufrDescValue *dscv );
extern void             bufr_valloc_DescValue       ( BufrDescValue *dscv, int nb_values );
extern void             bufr_vfree_DescValue        ( BufrDescValue *dscv );

#ifdef __cplusplus
}
#endif

#endif
