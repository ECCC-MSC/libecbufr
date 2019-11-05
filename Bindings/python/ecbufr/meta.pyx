# file: ecbufr.pyx

from libc.stdlib cimport malloc, free
from ecbufr.ctypes cimport *


from ecbufr cimport cmeta

cdef class BufrRTMD:
   cdef cmeta.BufrRTMD* _this_ptr
   cdef cmeta.BufrRTMD* _this

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          cmeta.bufr_free_rtmd(self._this_ptr)

   def allocate(self,count):
      self._this_ptr=cmeta.bufr_create_rtmd( count )
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this=self._this_ptr

   def assign(self,uint64_t ptr):
      self._this=<cmeta.BufrRTMD*>ptr

   def get_nesting(self,int pos):
      if pos < self._this.nb_nesting:
         return self._this.nesting[pos]
      return -1

   def len_nesting(self):
      return self._this.nb_nesting
