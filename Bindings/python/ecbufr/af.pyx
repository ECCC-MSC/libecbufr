# file: ecbufr.pyx

from libc.stdlib cimport malloc, free

from ecbufr cimport caf

cdef class BufrAF:
   cdef caf.BufrAF* _this_ptr
   cdef caf.BufrAF* _this

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          caf.bufr_free_af(self._this_ptr)

   def allocate(self,blens):
      cdef int *my_ints

      dlen = len(blens)
      my_ints = <int *>malloc(dlen*4)
      if my_ints is NULL:
         raise MemoryError()
      for i in xrange(dlen):
         my_ints[i] = blens[i]

      self._this_ptr=caf.bufr_create_af( my_ints, dlen )
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this=self._this_ptr
      free(my_ints)

   def get_value(self,int pos):
      return caf.bufr_af_get_value(self._this,pos)

   def get_sig(self, int pos):
      return caf.bufr_af_get_sig(self._this,pos)

   def set_value(self,int pos,int val):
      return caf.bufr_af_set_value(self._this,pos,val)

   def set_sig(self,int pos,int sig):
      return caf.bufr_af_set_sig(self._this,pos,sig)

   def sprint(self):
      cdef  char string[2048]
      cdef  char *s

      caf.bufr_print_af(string,self._this)
      ps = string
      return  ps
