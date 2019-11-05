# file: cvalue.pxd

from ecbufr.caf cimport BufrAF

cdef extern from "bufr_value.h":

   ctypedef enum   ValueType:
      pass

   ctypedef struct BufrValue:
      ValueType      type
      BufrAF         *af

   BufrValue     *bufr_create_value         ( ValueType type )
   void           bufr_free_value           ( BufrValue * )

   BufrValue     *bufr_create_value         ( ValueType type )
   BufrValue     *bufr_duplicate_value      ( const BufrValue *dup )
   void           bufr_free_value           ( BufrValue * )
   void           bufr_copy_value           ( BufrValue *dest, const BufrValue *src )
   int            bufr_compare_value        ( const BufrValue *bv1, const BufrValue *bv2, double epsilon )
   int            bufr_between_values       ( const BufrValue *bv1, const BufrValue *bv, const BufrValue *bv2 )

   int            bufr_value_set_string     ( BufrValue *bv, const char *str, int len )
   int            bufr_value_set_double     ( BufrValue *bv, double val )
   int            bufr_value_set_float      ( BufrValue *bv, float val )
   int            bufr_value_set_int32      ( BufrValue *bv, int  val )
   int            bufr_value_set_int64      ( BufrValue *bv, long  val )

   const char    *bufr_value_get_string     ( const BufrValue *bv, int *len )
   double         bufr_value_get_double     ( const BufrValue *bv )
   long           bufr_value_get_int64      ( const BufrValue *bv )
   float          bufr_value_get_float      ( const BufrValue *bv )
   int            bufr_value_get_int32      ( const BufrValue *bv )

   int            bufr_print_value          ( char *str, const BufrValue * )
   int            bufr_print_scaled_value   ( char *outstr, const BufrValue *bv, int scale )

   unsigned long  bufr_missing_ivalue       ( int nbits )
   unsigned long  bufr_negative_ivalue      ( long value, int nbits )
   long           bufr_cvt_ivalue           ( unsigned long value, int nbits )

   int            bufr_value_is_missing     ( BufrValue *bv )
   int            bufr_is_missing_float     ( float )
   int            bufr_is_missing_double    ( double )
   int            bufr_is_missing_int       ( int i )
   int            bufr_is_missing_string    ( const char *, int )
   double         bufr_missing_double       ( )
   float          bufr_missing_float        ( )
   int            bufr_missing_int          ( )
   void           bufr_missing_string       ( char *, int )

   double         bufr_get_max_double       ( )
   float          bufr_get_max_float        ( )
