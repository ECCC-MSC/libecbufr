# file: cmessage.pxd

#from ecbufr.cmessage cimport  BUFR_Message
from ecbufr.ctables cimport  BUFR_Tables
#from libc.stdio cimport FILE

cdef extern from "bufr_api.h":
   void    bufr_begin_api              ()

   int     bufr_load_cmc_tables        ( BUFR_Tables *tables )
