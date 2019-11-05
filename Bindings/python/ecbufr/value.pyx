# file: ecbufr.pyx

from libc.stdlib cimport malloc, free

from ecbufr cimport cvalue
from ecbufr.cvalue cimport ValueType
from ecbufr.ctypes cimport *

cdef enum:
   INT8=1
   INT32=2
   INT64=3
   FLT32=4
   FLT64=5
   STRING=6

cdef class BufrValue:
   cdef cvalue.BufrValue* _this_ptr
   cdef cvalue.BufrValue* _this

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          cvalue.bufr_free_value(self._this_ptr)

   def allocate(self,ValueType vtype):
      if self._this_ptr is not NULL:
         return
      self._this_ptr=cvalue.bufr_create_value( vtype )
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this=self._this_ptr

   def assign(self,uint64_t ptr):
      self._this=<cvalue.BufrValue*>ptr

   def get_value(self):
      cdef int len
      cdef int vtype

      if self._this is NULL:
         return ""
      vtype=self._this.type
      if vtype==FLT64:
         return cvalue.bufr_value_get_double(self._this)
      if vtype==FLT32:
         return cvalue.bufr_value_get_float(self._this)
      if vtype==STRING:
         string=cvalue.bufr_value_get_string(self._this,&len)
         return string
      if vtype==INT64:
         return cvalue.bufr_value_get_int64(self._this)
      return cvalue.bufr_value_get_int32(self._this)
      

   def set_value(self,val):
      cdef int vtype

      if self._this is NULL:
         return
      vtype=self._this.type
      if vtype==STRING:
         l=len(val)
         cvalue.bufr_value_set_string(self._this,val,l)
         return
      if vtype==FLT64:
         cvalue.bufr_value_set_double(self._this,val)
         return
      if vtype==FLT32:
         cvalue.bufr_value_set_float(self._this,val)
         return
      cvalue.bufr_value_set_int64(self._this,val)

   def get_type(self):
      if self._this is NULL:
         return -1
      return self._this.type

   def is_missing(self):
      cdef int msng

      if self._this is NULL:
         return True
      msng=cvalue.bufr_value_is_missing(self._this)
      if msng==1:
         return True
      else:
         return False
