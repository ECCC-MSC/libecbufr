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
 
 *  file      :  BUFR_DATASET.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADER FILE FOR BUFR DATASET
 *               A DATASET CONTAINS 1 OR MANY DATASUBSETS
 *               EACH SUBSET IS AN INSTANCE OF DATA BASED ON THE TEMPLATE
 *               FOUND IN SECTION 3 OF THE BUFR MESSAGE
 *               A DATASET IS A DECODED REPRESENTATION OF THE BUFR MESSAGE
 *
 */

#ifndef _bufr_dataset_h
#define _bufr_dataset_h

#include  <stdio.h>
#include  "bufr_array.h"
#include  "bufr_io.h"
#include  "bufr_desc.h"
#include  "bufr_value.h"
#include  "bufr_af.h"
#include  "bufr_tables.h"
#include  "bufr_template.h"
#include  "bufr_message.h"
#include  "bufr_sequence.h"
#include  "bufr_ddo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define   BUFR_SET_ORIG_CENTRE(d,val)    (d)->s1.orig_centre=(val)
#define   BUFR_SET_MASTER_TABLE(d,val)   (d)->s1.bufr_master_table=(val)
#define   BUFR_SET_SUB_CENTRE(d,val)     (d)->s1.orig_sub_centre=(val)
#define   BUFR_SET_UPD_SEQUENCE(d,val)   (d)->s1.upd_seq_no=(val)
#define   BUFR_SET_DATA_CATEGORY(d,val)  (d)->s1.msg_type=(val)
#define   BUFR_SET_INTERN_SUB_CAT(d,val) (d)->s1.msg_inter_subtype=(val)
#define   BUFR_SET_LOCAL_SUB_CAT(d,val)  (d)->s1.msg_local_subtype=(val)
#define   BUFR_SET_MSTR_TBL_VRSN(d,val)  (d)->s1.master_table_version=(val)
#define   BUFR_SET_LOCAL_TBL_VRSN(d,val) (d)->s1.local_table_version=(val)
#define   BUFR_SET_YEAR(d,val)           (d)->s1.year=(val)
#define   BUFR_SET_MONTH(d,val)          (d)->s1.month=(val)
#define   BUFR_SET_DAY(d,val)            (d)->s1.day=(val)
#define   BUFR_SET_HOUR(d,val)           (d)->s1.hour=(val)
#define   BUFR_SET_MINUTE(d,val)         (d)->s1.minute=(val)
#define   BUFR_SET_SECOND(d,val)         (d)->s1.second=(val)

#define   BUFR_GET_ORIG_CENTRE(d)        (d)->s1.orig_centre
#define   BUFR_GET_MASTER_TABLE(d)       (d)->s1.bufr_master_table
#define   BUFR_GET_SUB_CENTRE(d)         (d)->s1.orig_sub_centre
#define   BUFR_GET_UPD_SEQUENCE(d)       (d)->s1.upd_seq_no
#define   BUFR_GET_DATA_CATEGORY(d)      (d)->s1.msg_type
#define   BUFR_GET_INTERN_SUB_CAT(d)     (d)->s1.msg_inter_subtype
#define   BUFR_GET_LOCAL_SUB_CAT(d)      (d)->s1.msg_local_subtype
#define   BUFR_GET_MSTR_TBL_VRSN(d)      (d)->s1.master_table_version
#define   BUFR_GET_LOCAL_TBL_VRSN(d)     (d)->s1.local_table_version
#define   BUFR_GET_YEAR(d)               (d)->s1.year
#define   BUFR_GET_MONTH(d)              (d)->s1.month
#define   BUFR_GET_DAY(d)                (d)->s1.day
#define   BUFR_GET_HOUR(d)               (d)->s1.hour
#define   BUFR_GET_MINUTE(d)             (d)->s1.minute
#define   BUFR_GET_SECOND(d)             (d)->s1.second


#define  BUFR_FLAG_INVALID               (1<<8)
#define  BUFR_FLAG_SUSPICIOUS            (1<<9)
#define  BUFR_IS_INVALID(d)              ((d)->data_flag & BUFR_FLAG_INVALID)
#define  BUFR_IS_SUSPICIOUS(d)           ((d)->data_flag & BUFR_FLAG_SUSPICIOUS)


typedef ArrayPtr DataSubsetArray;

typedef struct bufr_dataset
   {
   DataSubsetArray       datasubsets;  /* ARRAY OF SUBSETS */
   BUFR_Template        *tmplte;       /* TEMPLATE OF DESCRIPTORS FOR WHOLE DATASET */
   BufrSection1          s1;           /* BUFR MESSAGE SECTION 1 INFOS */
   int                   data_flag;    /* FLAG OF SECTION 3 */
   char                 *header_string;
   } BUFR_Dataset;

typedef struct bufr_datasubset
   {
   BufrDescriptorArray  data;
   BufrDPBM            *dpbm;
   } DataSubset;


extern BUFR_Dataset    *bufr_create_dataset         ( BUFR_Template *tmplt );
void                    bufr_free_dataset           ( BUFR_Dataset * );


extern BUFR_Template   *bufr_get_dataset_template   ( BUFR_Dataset *dts );

extern int              bufr_create_datasubset      ( BUFR_Dataset *dts );

extern DataSubset      *bufr_get_datasubset         ( BUFR_Dataset *dts, int pos );

extern int              bufr_count_datasubset       ( BUFR_Dataset *dts );

extern int              bufr_dataset_compressible   ( BUFR_Dataset *dts );

extern int              bufr_add_datasubset         ( BUFR_Dataset *dts, BUFR_Sequence *bcl, BufrDDOp * );

extern int              bufr_datasubset_count_descriptor  ( DataSubset *subset );

extern BufrDescriptor  *bufr_datasubset_get_descriptor    ( DataSubset *ss, int pos );
extern BufrDescriptor  *bufr_datasubset_next_descriptor   ( DataSubset *ss, int *pos );


extern BUFR_Dataset    *bufr_decode_message         ( BUFR_Message *msg, BUFR_Tables *local_tables );
extern BUFR_Message    *bufr_encode_message         ( BUFR_Dataset *dts , int x_compress );
extern BUFR_Dataset    *bufr_decode_message_subsets ( BUFR_Message *msg, BUFR_Tables *local_tables, int subset_from, int subset_to );


extern int              bufr_merge_dataset          ( BUFR_Dataset *dest, int dest_pos, 
                                                      BUFR_Dataset *src,  int src_pos, int nb );

extern int              bufr_expand_datasubset      ( BUFR_Dataset *dts, int pos );

extern int              bufr_load_dataset           ( BUFR_Dataset *dts, const char *infile );

extern int              bufr_read_dataset_dump      ( BUFR_Dataset *dts, FILE *fp );

extern int              bufr_dump_dataset           ( BUFR_Dataset *dts, const char *outfile );
extern int              bufr_fdump_dataset          ( BUFR_Dataset *dts, FILE *fp );

extern BUFR_Dataset    *bufr_create_dataset_from_sequence
                                                    ( BUFR_Sequence *cl, BUFR_Tables *tbls, int edition );
extern int              bufr_genmsgs_from_dump
                                    ( BUFR_Template *tmplt, const char *infile, const char *outfile, int );

#ifdef __cplusplus
}
#endif

#endif
