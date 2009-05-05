/*
 *
 *  file      :  BUFR_PRIV_VALUE.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR PRIVATE DECLARATION OF BUFR VALUES TYPE
 *
 *
 */

#ifndef _bufr_priv_value_h
#define _bufr_priv_value_h

#include <inttypes.h>
#include "bufr_af.h"

typedef struct
   {
   ValueType         type;
   BufrAF            *af;
   int8_t            value;
   } ValueINT8;

typedef struct
   {
   ValueType          type;
   BufrAF            *af;
   int32_t            value;
   } ValueINT32;

typedef struct
   {
   ValueType          type;
   BufrAF            *af;
   int64_t            value;
   } ValueINT64;

typedef struct
   {
   ValueType          type;
   BufrAF            *af;
   float              value;
   } ValueFLT32;

typedef struct
   {
   ValueType         type;
   BufrAF            *af;
   double            value;
   } ValueFLT64;

typedef struct
   {
   ValueType         type;
   BufrAF            *af;
   char             *value;
   int16_t           len;
   } ValueSTRING;

#endif
