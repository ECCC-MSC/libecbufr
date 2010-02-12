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
 
 *  file      :  BUFR_DDO.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR DATA DESCRIPTION OPERATORS
 *               THIS IS MOSTLY FOR LIBRARY INTERNAL USE ONLY
 *               USERS SHOULD NEVER NEED TO CALL THESE FUNCTIONS DIRECTLY
 *
 */

#ifndef _bufr_d_d_o_h_
#define _bufr_d_d_o_h_

#include  <stdio.h>
#include  "bufr_desc.h"
#include  "bufr_linklist.h"
#include  "bufr_tables.h"

typedef struct 
   {
   int8_t   *bit_map;
   int      *index;
   int       nb_codes;
   int      *dp;
   int       nb_dp;
   } BufrDPBM;

#define  DDO_DEFINE_EVENT         0x001
#define  DDO_SUBST_VAL_FOLLOW     0x002
#define  DDO_FO_STATS_VAL_FOLLOW  0x004
#define  DDO_QUAL_INFO_FOLLOW     0x008
#define  DDO_BIT_MAP_FOLLOW       0x010
#define  DDO_USE_PREV_BIT_MAP     0x020
#define  DDO_HAS_LOCATION         0x040

typedef struct _oper_table
   {
   unsigned int            flags;
   int                     add_nbits;
   short                   multiply_scale;
   unsigned char           change_ref_val_op;
   short                   add_af_nbits;
   unsigned char           local_nbits_follows;
   unsigned char           use_ieee_fp;
   short                   change_ref_value;

   LocationValueArray      current_location;
   EntryTableBArray        override_tableb;
   LinkedList             *af_list;
   ListNode               *current;
   LocationEncodingArray   tlc_arr;

   BufrDPBM               *dpbm;
   ListNode               *start_dpi;
   int                     remain_dpi;
   } BufrDDOp;

extern BufrDPBM      *bufr_create_BufrDPBM          ( int cnt );
extern void           bufr_free_BufrDPBM            ( BufrDPBM * );
extern void           bufr_init_dpbm                ( BufrDPBM *dpbm, ListNode *node );

extern BufrDDOp      *bufr_create_BufrDDOp          ( void );
extern void           bufr_free_BufrDDOp            ( BufrDDOp * );
extern int            bufr_resolve_tableC_v2        ( BufrDescriptor *, BufrDDOp *, 
                                                      int x, int y, int v, ListNode * );
extern int            bufr_resolve_tableC_v3        ( BufrDescriptor *, BufrDDOp *, 
                                                      int x, int y, int v, ListNode * );
extern int            bufr_resolve_tableC_v4        ( BufrDescriptor *, BufrDDOp *, 
                                                      int x, int y, int v, ListNode * );
extern int            bufr_resolve_tableC_v5        ( BufrDescriptor *, BufrDDOp *, 
                                                      int x, int y, int v, ListNode * );
extern void           bufr_set_descriptor_afd       ( BufrDescriptor *, LinkedList * );
extern int            bufr_is_start_dpbm            ( int descriptor  );
extern int            bufr_is_marker_dpbm           ( int descriptor );
extern int            bufr_is_sig_datawidth         ( int descriptor );

extern void           bufr_keep_location            ( BufrDDOp *ddo, int desc,  float );
extern int            bufr_is_location              ( int desc );
extern void           bufr_assoc_location           ( BufrDescriptor *bc, BufrDDOp *ddo );
extern void           bufr_clear_location           ( BufrDDOp *ddo );
extern void           bufr_set_current_location     ( BufrDDOp *ddo, int desc, float value, int npos );
extern LocationValue *bufr_current_location         ( BufrDDOp *ddo, BufrRTMD *meta, int *nbtlc );


#endif
