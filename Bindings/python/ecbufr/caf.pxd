# file: caf.pxd

cdef extern from "bufr_afd.h":
   ctypedef struct AF_Definition:
      int              nbits
      char            *sig

   ctypedef struct BufrAFD:
      int            count
      AF_Definition *defs

   BufrAFD          *bufr_create_afd     ( const int *bdefs, int count )
   BufrAFD          *bufr_duplicate_afd  ( const BufrAFD * )
   void              bufr_free_afd       ( BufrAFD * )

cdef extern from "bufr_af.h":
   ctypedef struct AF_Field:
      unsigned char  len
      unsigned char  shift
      short          signify

   ctypedef struct BufrAF:
      unsigned long  bits
      unsigned short nbits
      unsigned short count

   BufrAF           *bufr_create_af     ( const int *bdefs, int count )
   BufrAF           *bufr_duplicate_af  ( const BufrAF * )
   void              bufr_free_af       ( BufrAF * )
   void              bufr_af_set_value  ( BufrAF *, int pos, int val )
   int               bufr_af_get_value  ( const BufrAF *, int pos )
   void              bufr_af_set_sig    ( BufrAF *, int pos, int val )
   int               bufr_af_get_sig    ( const BufrAF *, int pos )
   int               bufr_print_af      ( char *outstr, const BufrAF *af )
