# file: cvalue.pxd

from ecbufr.caf cimport BufrAF
from ecbufr.caf cimport BufrAFD
from ecbufr.cmeta cimport BufrRTMD
from ecbufr.cvalue cimport *
from ecbufr.ctables cimport BufrValueEncoding
from ecbufr.ctables cimport BUFR_Tables
from ecbufr.ctables cimport BufrDataType

from ecbufr.ctypes cimport *

cdef extern from "bufr_desc.h":

   ctypedef struct BufrDescriptor:
      int                  descriptor
      BufrValue           *value
      BufrValueEncoding    encoding
      unsigned char        flags
      BufrAFD             *afd
      BufrRTMD            *meta
      ValueType            type
      BufrAF              *af

   BufrDescriptor *bufr_create_descriptor         ( BUFR_Tables *tbls, int desc )
   BufrDescriptor *bufr_dupl_descriptor           ( BufrDescriptor *dup )
   void            bufr_free_descriptor           ( BufrDescriptor * )
   void            bufr_copy_descriptor           ( BufrDescriptor *dest, BufrDescriptor *src )
   BufrValue      *bufr_mkval_for_descriptor      ( BufrDescriptor * )
   void            bufr_print_descriptor          ( char *str, BufrDescriptor *bdsc )
   int             bufr_print_dscptr_value        ( char *outstr, BufrDescriptor *cb )
   ValueType       bufr_encoding_to_valtype       ( BufrValueEncoding * )
   ValueType       bufr_datatype_to_valtype       ( BufrDataType type, int nbits, int scale )

   int             bufr_descriptor_set_fvalue     ( BufrDescriptor *bdsc,  float  val )
   int             bufr_descriptor_set_dvalue     ( BufrDescriptor *bdsc,  double val )
   int             bufr_descriptor_set_ivalue     ( BufrDescriptor *bdsc,  int    val )
   int             bufr_descriptor_set_svalue     ( BufrDescriptor *bdsc,  const char  *val )
   int             bufr_descriptor_set_bitsvalue  ( BufrDescriptor *bdsc,  unsigned long ival )

   float           bufr_descriptor_get_fvalue     ( BufrDescriptor *bdsc )
   double          bufr_descriptor_get_dvalue     ( BufrDescriptor *bdsc )
   int             bufr_descriptor_get_ivalue     ( BufrDescriptor *bdsc )
   char           *bufr_descriptor_get_svalue     ( BufrDescriptor *bdsc, int *len )
