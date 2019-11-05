# file: ecbufr.pyx

from ecbufr cimport csection1
from ecbufr.ctypes cimport *
from libc.stdio cimport FILE, fopen, fclose

cdef class BufrSection1:
   cdef csection1.BufrSection1*  _this

   def __cinit__(self,uint64_t ptr):
      self._this=<csection1.BufrSection1*>ptr

   @property
   def bufr_master_table(self):
      return self._this.bufr_master_table

   @property
   def orig_centre(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.orig_centre

   @property 
   def orig_sub_centre(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.orig_sub_centre

   @property 
   def upd_seq_no(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.upd_seq_no

   @property 
   def flag(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.flag

   @property
   def msg_type(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.msg_type

   @property
   def msg_inter_subtype(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.msg_inter_subtype

   @property
   def msg_local_subtype(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.msg_local_subtype

   @property
   def master_table_version(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.master_table_version

   @property
   def local_table_version(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.local_table_version

   @property
   def year(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.year

   @property
   def month(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.month

   @property
   def day(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.day

   @property
   def hour(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.hour

   @property
   def minute(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.minute

   @property
   def second(self):
      if self._this is NULL:
         raise MemoryError()
      return self._this.second
