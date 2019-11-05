# file: ecbufr.pyx

from libc.stdlib cimport malloc, free
from cpython.pycapsule cimport *

from ecbufr cimport cvalue
from ecbufr.cvalue cimport ValueType
from ecbufr cimport cdescriptor
from ecbufr.ctables cimport BUFR_Tables
from ecbufr cimport cdataset
from ecbufr.cdataset cimport DataSubset
from ecbufr.ctypes cimport *
from ecbufr.value import  BufrValue
from ecbufr.meta import  BufrRTMD

cdef enum:
   FLAG_CLASS31=0x1
   FLAG_EXPANDED=0x2
   FLAG_SKIPPED=0x4
   FLAG_CLASS33=0x8
   FLAG_IGNORED=0x10

cdef class BufrDescriptor:
   cdef cdescriptor.BufrDescriptor* _this_ptr
   cdef cdescriptor.BufrDescriptor* _this
   cdef object  value

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr
      self.value=None

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          cdescriptor.bufr_free_descriptor(self._this_ptr)

   def allocate(self,int desc,tbl):
      cdef BUFR_Tables  *tbl_ptr

      tobj=tbl.get_obj()
      tbl_ptr = <BUFR_Tables *>PyCapsule_GetPointer(tobj,"BUFR_Tables")
      self._this_ptr=cdescriptor.bufr_create_descriptor( tbl_ptr, desc )
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this=self._this_ptr

   def assign(self, uint64_t ptr):
      self._this=<cdescriptor.BufrDescriptor*>ptr
      if not isinstance(self.value,BufrValue):
         self.value = BufrValue()
      self.value.assign( <uint64_t>self._this.value )

   @property
   def descriptor(self):
      return self._this.descriptor

   property flags:
      def __get__(self):
         return self._this.flags
      def __set__(self,value):
         self._this.flags=value

   def get_value(self):
      cdef cdescriptor.BufrDescriptor* ptr
      if self._this is NULL:
         return None
      if self._this.value is NULL:
         return None

      if not isinstance(self.value,BufrValue):
         self.value = BufrValue()
         ptr = self._this
         self.value.assign( <uint64_t>ptr.value )

      return self.value.get_value()

   def get_rtmd(self):
      cdef cdescriptor.BufrDescriptor* ptr
      if self._this is NULL:
         return None
      if self._this.meta is NULL:
         return None

      v = BufrRTMD()
      ptr = self._this
      v.assign( <uint64_t>ptr.meta )
      return v


   def set_value(self,val):
      cdef cdescriptor.BufrDescriptor* ptr

      if self._this is NULL:
         return
      if self._this.value is NULL:
         return
      v = BufrValue()
      ptr = self._this
      v.assign( <uint64_t>ptr.value )
      v.set_value( val )

   def get_type(self):
      return self._this.value.type

   def BufrValue(self):
      cdef cdescriptor.BufrDescriptor* ptr
      if self._this is NULL:
         return None
      if self._this.value is NULL:
         return None

      v = BufrValue()
      ptr = self._this
      v.assign( <uint64_t>ptr.value )
      return v

   def is_missing(self):
      cdef cdescriptor.BufrDescriptor* ptr
      if self._this is NULL:
         return True
      if self._this.value is NULL:
         return True

      v = BufrValue()
      ptr = self._this
      v.assign( <uint64_t>ptr.value )
      return v.is_missing()

