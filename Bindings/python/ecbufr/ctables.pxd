# file: ctables.pxd

cdef extern from "bufr_tables.h":
   ctypedef struct BufrTablesSet:
      int version
   ctypedef struct BUFR_Tables:
      BUFR_Tables* _this
      BufrTablesSet master

   ctypedef enum BufrDataType:
      pass

   ctypedef struct BufrValueEncoding:
      BufrDataType    type
      int             scale
      int             reference
      int             nbits
      unsigned char   af_nbits

   ctypedef struct EntryTableB:
      int descriptor
      char *description
      char *unit
      BufrValueEncoding  encoding

   ctypedef struct EntryTableD:
      int descriptor
      char *description
      int count
      int *descriptors


   BUFR_Tables*    bufr_create_tables()
   void            bufr_free_tables( BUFR_Tables * )

   int bufr_load_l_tableB ( BUFR_Tables *, char *filename )
   int bufr_load_l_tableD ( BUFR_Tables *, char *filename )
   int bufr_load_m_tableB ( BUFR_Tables *, char *filename )
   int bufr_load_m_tableD ( BUFR_Tables *, char *filename )

   EntryTableB* bufr_fetch_tableB           ( BUFR_Tables *, int desc )
   EntryTableD* bufr_fetch_tableD           ( BUFR_Tables *, int desc )
   EntryTableD* bufr_match_tableD_sequence  ( BUFR_Tables *, int ndesc, int desc[] )

   EntryTableD* bufr_new_EntryTableD  ( int desc, const char *name, int len, int *descs, int count )
   void         bufr_free_EntryTableD ( EntryTableD * )
   EntryTableB* bufr_new_EntryTableB  ()
   void         bufr_free_EntryTableB ( EntryTableB * )
   void         bufr_copy_EntryTableB ( EntryTableB *dest, EntryTableB *src )
