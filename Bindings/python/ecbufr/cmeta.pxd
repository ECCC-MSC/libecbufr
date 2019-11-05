# file: caf.pxd

from ecbufr.cdescriptor cimport BufrDescriptor

cdef extern from "bufr_meta.h":
   ctypedef struct LocationValue:
      int   descriptor
      float value

   ctypedef struct BufrRTMD:
      int           *nesting
      int            nb_nesting
      LocationValue *tlc
      int            nb_tlc
      int            pos_template
      int            len_expansion

      BufrDescriptor ** qualifiers
      int            nb_qualifiers


   BufrRTMD     *bufr_create_rtmd     ( int count )
   void          bufr_free_rtmd       ( BufrRTMD * )
