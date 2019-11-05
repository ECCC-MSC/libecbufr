# file: cmessage.pxd

from ecbufr.ctemplate cimport  BUFR_Template
from ecbufr.ctables   cimport  BUFR_Tables
from ecbufr.csection1 cimport  BufrSection1
from ecbufr.cmessage  cimport  BUFR_Message
from ecbufr.cdescriptor  cimport  BufrDescriptor
from ecbufr.ctemplate cimport  BufrDescValue
from ecbufr.cvalue cimport  BufrValue

from libc.stdio cimport FILE

cdef extern from "bufr_dataset.h":

   ctypedef struct BUFR_Dataset:
      char             *header_string
      BufrSection1      s1

   ctypedef struct DataSubset:
      pass

   BUFR_Dataset   *bufr_create_dataset    ( BUFR_Template *tmplt )
   void            bufr_free_dataset      ( BUFR_Dataset * )

   BUFR_Dataset   *bufr_decode_message    ( BUFR_Message *msg, BUFR_Tables *tables )
   int             bufr_count_datasubset  ( BUFR_Dataset *dts )

   DataSubset     *bufr_get_datasubset    ( BUFR_Dataset *dts, int pos )

   int             bufr_datasubset_count_descriptor  ( DataSubset *subset )
   BufrDescriptor *bufr_datasubset_get_descriptor    ( DataSubset *ss, int pos )


cdef extern from "bufr_api.h":
   int     bufr_subset_find_descriptor  ( DataSubset *subset, int descriptor, int startpos )
   int     bufr_subset_find_values      ( DataSubset *dts, BufrDescValue *descs, int nb, int startpos )
   void    bufr_set_key_location        ( BufrDescValue *cv, int descriptor, float value )
   void    bufr_set_key_qualifier       ( BufrDescValue *cv, int descriptor, const BufrValue* value )
   void    bufr_set_key_qualifier_int32 ( BufrDescValue *cv, int descriptor, int value)
   void    bufr_set_key_qualifier_flt32 ( BufrDescValue *cv, int descriptor, float value)
   void    bufr_set_key_flt32          ( BufrDescValue *cv, int descriptor, float *values, int nbval )
   void    bufr_set_key_int32          ( BufrDescValue *cv, int descriptor, int *values,   int nbval )
   void    bufr_set_key_string         ( BufrDescValue *cv, int descriptor, const char **values, int nbval )

