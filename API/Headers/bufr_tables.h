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
 
 *  file      :  bufr_tables.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR TABLES
 *               FOR HANDLING OF TABLE B AND D
 *
 */

#ifndef _bufr_tables_h
#define _bufr_tables_h

#include "bufr_array.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FXY_TO_DESC(f,x,y)  ((f)*100000+(x)*1000+(y))

/*
 * ERROR CODES
 */
#define BUFR_NOERROR		     0
#define BUFR_TB_OVERFLOW	    -1
#define BUFR_TB_UNDERFLOW	    -2
#define BUFR_TD_NOTFOUND	    -3
#define BUFR_TB_NOTFOUND	    -4
#define DATA_BLKCNT_ILLEGAL	    -5
#define BUFR_NT_UNMATCH		    -6
#define BUFR_TB_INVALID             -7


typedef enum
   {
   TYPE_UNDEFINED,
   TYPE_REPLICATOR,
   TYPE_OPERATOR, /* TABLEC */
   TYPE_SEQUENCE, /* TABLED */
   TYPE_NUMERIC,   /* TABLEB */
   TYPE_CCITT_IA5, /* TABLEB */ 
   TYPE_CODETABLE,        /* TABLEB */ 
   TYPE_FLAGTABLE,        /* TABLEB */ 
   TYPE_CHNG_REF_VAL_OP,  /* TABLEC 2-03-YYY */ 
   TYPE_IEEE_FP           /* TABLEC 2-09-YYY */
   } BufrDataType;

typedef struct
   {
   BufrDataType    type;
   int             scale;
   int             reference;
   int             nbits ;
   unsigned char   af_nbits;
   } BufrValueEncoding;


typedef struct entry_tableb
{
   int                descriptor;
   BufrValueEncoding  encoding;
   char              *description;
   char              *unit;
} EntryTableB ;


typedef struct entry_tabled
   {
   int   descriptor;
   int  *descriptors;
   char *description;
   int   count;
} EntryTableD ;

typedef enum
   {
   TYPE_REFERENCED,
   TYPE_ALLOCATED
   } BufrStorageType;

typedef ArrayPtr EntryTableBArray;
typedef ArrayPtr EntryTableDArray;

typedef struct
   {
   int                version;
   EntryTableBArray   tableB;
   BufrStorageType    tableBtype;
   EntryTableDArray   tableD;
   BufrStorageType    tableDtype;
   } BufrTablesSet;

typedef struct _TblsBufr
   {
   BufrTablesSet master;
   BufrTablesSet local;
   int        data_cat;
   char       data_cat_desc[65];
   } BUFR_Tables;

extern EntryTableB   *bufr_fetch_tableB           ( BUFR_Tables *, int desc );
extern EntryTableD   *bufr_fetch_tableD           ( BUFR_Tables *, int desc );
extern EntryTableD   *bufr_match_tableD_sequence  ( BUFR_Tables *,
                                                    int ndesc, int desc[] );

extern int            bufr_load_l_tableB          ( BUFR_Tables *, const char *filename );
extern int            bufr_load_l_tableD          ( BUFR_Tables *, const char *filename );
extern int            bufr_load_m_tableB          ( BUFR_Tables *, const char *filename );
extern int            bufr_load_m_tableD          ( BUFR_Tables *, const char *filename );

extern void           bufr_merge_tables           ( BUFR_Tables *dest, BUFR_Tables *source );
extern void           bufr_tableb_free            ( EntryTableBArray tableb );
extern void           bufr_set_tables_category    ( BUFR_Tables *, int cat, const char *desc );

extern EntryTableD   *bufr_new_EntryTableD        ( int desc, const char *name, int len, int *descs, int count );
extern EntryTableB   *bufr_new_EntryTableB        ( void );
extern void           bufr_free_EntryTableB       ( EntryTableB *r );
extern void           bufr_copy_EntryTableB       ( EntryTableB *dest, EntryTableB *src );

extern EntryTableB   *bufr_tableb_fetch_entry     ( EntryTableBArray tableb, int desc);
extern EntryTableB   *bufr_tableb_fetch_entry_desc( EntryTableBArray addr, const char *desc);
extern EntryTableD   *bufr_tabled_match_sequence  ( EntryTableDArray tabled,
                                                    int ndesc, int desc[] );

extern BUFR_Tables   *bufr_create_tables           ( void );
extern void           bufr_free_tables            ( BUFR_Tables * );

extern BufrDataType   bufr_unit_to_datatype       ( const char *unit );
extern BufrDataType   bufr_descriptor_to_datatype ( BUFR_Tables *, EntryTableB *e, int desc, int *len );
extern int            bufr_parse_columns          ( const char *ligne, int *column, int limit );
extern int            bufr_is_local_descriptor    ( int desc );
extern int            bufr_is_table_b             ( int code );
extern int            bufr_is_descriptor          ( int desc );

extern void           bufr_descriptor_to_fxy      ( int desc, int *f, int *x, int *y );
extern int            bufr_fxy_to_descriptor      ( int f, int x, int y );
extern uint16_t       bufr_fxy_to_descriptor_i16  ( int f, int x, int y );
extern uint16_t       bufr_descriptor_i32_to_i16  ( int desc );
extern int            bufr_value_nbits            ( int64_t ival );
extern int            bufr_leftest_bit            ( uint64_t val );
extern uint32_t       bufr_cvt_fval_to_i32        ( int desc, BufrValueEncoding *be, float fval );
extern uint64_t       bufr_cvt_dval_to_i64        ( int desc, BufrValueEncoding *be, double fval );
extern float          bufr_cvt_i32_to_fval        ( BufrValueEncoding *be, uint32_t ival );
extern double         bufr_cvt_i64_to_dval        ( BufrValueEncoding *be, int64_t lval );

extern int            bufr_get_tberror            ( BufrValueEncoding *be, int *reference, int *nbits );

#ifdef __cplusplus
}
#endif

#endif
