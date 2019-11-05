# file: ctemplate.pxd

from ecbufr.cvalue cimport BufrValue
from ecbufr.ctables cimport BUFR_Tables

cdef extern from "bufr_template.h":
   ctypedef struct BUFR_Template:
      pass
   ctypedef struct BufrDescValue:
      int          descriptor
      BufrValue  **values
      int          nbval

   BUFR_Template   *bufr_create_template        ( BufrDescValue *, int nb, BUFR_Tables *, int )
   void             bufr_template_add_DescValue ( BUFR_Template *, BufrDescValue *, int nb )
   int              bufr_finalize_template      ( BUFR_Template *tmplt )
   void             bufr_free_template          ( BUFR_Template * )
   BUFR_Template   *bufr_copy_template          ( BUFR_Template * )

   BUFR_Template   *bufr_load_template          ( const char *filename, BUFR_Tables *mtbls )
   int              bufr_save_template          ( const char *filename, BUFR_Template *tmplt )

   int              bufr_compare_template       ( BUFR_Template *, BUFR_Template * )

   void             bufr_init_DescValue         ( BufrDescValue *dscv )
   void             bufr_valloc_DescValue       ( BufrDescValue *dscv, int nb_values )
   int              bufr_vgrow_DescValue        ( BufrDescValue *dscv, int nb_values )
   void             bufr_vfree_DescValue        ( BufrDescValue *dscv )
