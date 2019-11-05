# file: cmessage.pxd

from libc.stdio cimport FILE
from ecbufr.csection1 cimport  BufrSection1

ctypedef enum BUFR_Enforcement :
   BUFR_LAX 
   BUFR_WARN_ALLOW 
   BUFR_STRICT

cdef extern from "bufr_message.h":

   ctypedef struct BUFR_Message:
      int               edition
      char             *header_string
      BufrSection1      s1
      BUFR_Enforcement  enforce

   BUFR_Message  *bufr_create_message  ( int edition )
   void           bufr_free_message    ( BUFR_Message *bufr )

   void           bufr_init_header     ( BUFR_Message *bufr, int edition )
   void           bufr_begin_message   ( BUFR_Message *bufr )
   void           bufr_end_message     ( BUFR_Message *bufr )

cdef extern from "bufr_api.h":
   int     bufr_read_message           ( FILE *fp, BUFR_Message **rtrn )
   int     bufr_write_message          ( FILE *fp, BUFR_Message *bufr )
