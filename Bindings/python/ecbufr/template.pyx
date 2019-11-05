# file: ecbufr.pyx

from ecbufr.ctables cimport BUFR_Tables
from cpython.pycapsule cimport *

from ecbufr.tables import EntryTableB
from ecbufr.tables import EntryTableD

from ecbufr cimport ctemplate
from libc.stdlib cimport malloc, free

cdef class BUFR_Template:
   cdef ctemplate.BUFR_Template* _this_ptr
   cdef ctemplate.BUFR_Template* _this

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          ctemplate.bufr_free_template(self._this_ptr)

   cdef load(self,basestring filename, tbl):
      cdef BUFR_Tables  *tbl_ptr

      tobj=tbl.get_obj()
      tbl_ptr = <BUFR_Tables *>PyCapsule_GetPointer(tobj,"BUFR_Tables")
      self._this_ptr = ctemplate.bufr_load_template( filename.encode(), tbl_ptr )

   def save(self,basestring filename):
      ctemplate.bufr_save_template( filename.encode(), self._this )

   def allocate(self,BufrDescValue dv, tbl, int edition):
      cdef BUFR_Tables  *tbl_ptr

      nb=len(dv)
      print "nb=", nb
      print "edition=", edition
      tobj=tbl.get_obj()
      tbl_ptr = <BUFR_Tables *>PyCapsule_GetPointer(tobj,"BUFR_Tables")
      self._this_ptr = ctemplate.bufr_create_template(dv._this,nb,tbl_ptr,edition)
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this=self._this_ptr

   def compare(self, BUFR_Template other):
      return ctemplate.bufr_compare_template( self._this, other._this )

   def copy(self):
      bt=BUFR_Template()
      bt._this_ptr = ctemplate.bufr_copy_template( self._this )
      bt._this = bt._this_ptr
      return bt

   def add(self,BufrDescValue dv,tbl):
      cdef ctemplate.BufrDescValue *descs

      descs = dv._this
      nb=len(dv)
      for i in range(nb):
         dsc=descs[i].descriptor
         tbe=tbl.fetch_tableB( dsc )
         if not isinstance(tbe,EntryTableB):
            tde=tbl.fetch_tableD( dsc )
            if not isinstance(tbe,EntryTableD):
               print "Error Not a Descriptor:",dsc
               raise ValueError()

      ctemplate.bufr_template_add_DescValue( self._this, dv._this, len(dv) )

   def finalize(self):
      ctemplate.bufr_finalize_template(self._this)

cdef class BufrDescValue:
   cdef ctemplate.BufrDescValue*  _this_ptr
   cdef ctemplate.BufrDescValue*  _this
   cdef int                       nb

   def __cinit__(self,int nb):
      self._this_ptr=NULL
      self._this=self._this_ptr

      cdef ctemplate.BufrDescValue *descs

      descs = <ctemplate.BufrDescValue *>malloc(nb*sizeof(BufrDescValue))
      if descs is NULL:
         raise MemoryError()
      self.nb = nb
      self._this_ptr=descs
      self._this=self._this_ptr
      for i in range(nb):
         ctemplate.bufr_init_DescValue ( &descs[i] )


   def __dealloc__(self):
      if self._this_ptr is not NULL:
          free(self._this_ptr)

   def __len__(self):
      return self.size()

   def size(self):
      return self.nb

   def set_desc(self,pos,desc):
      cdef ctemplate.BufrDescValue *descs
      descs = self._this
      descs[pos].descriptor=desc

   def get_desc(self,pos):
      cdef ctemplate.BufrDescValue *descs
      descs = self._this
      return descs[pos].descriptor
