# file: ecbufr.pyx

from ecbufr.ctypes cimport uint64_t
from ecbufr cimport cmessage
from ecbufr cimport cdataset
from libc.stdio cimport FILE, fopen, fclose
from cpython.pycapsule cimport *

from ecbufr cimport  csection1
from ecbufr cimport  ctables
#from ecbufr.csection1 cimport  BufrSection1
from ecbufr.section1 import BufrSection1
from ecbufr.tables import BUFR_Tables
from ecbufr.dataset import BUFR_Dataset


cdef class BUFR_Message:
   cdef cmessage.BUFR_Message* _this
   cdef object    obj

   def __cinit__(self):
      self._this=NULL

# let capsule destructor do deallocation of obj
   def __dealloc__(self):
      self._this=NULL

   def allocate(self,int edition):
      if self._this is not NULL:
         return
      self._this = cmessage.bufr_create_message(edition)
      if self._this is NULL:
         raise MemoryError()
      self.obj = PyCapsule_New(<void *>self._this,"BUFR_Message",<PyCapsule_Destructor>BUFR_Message.del_BUFR_Message)

   @staticmethod
   cdef del_BUFR_Message(object obj):
      cdef cmessage.BUFR_Message* ptr

      ptr = <cmessage.BUFR_Message *>PyCapsule_GetPointer(obj,"BUFR_Message")
      if ptr is not NULL:
         cmessage.bufr_free_message(ptr)

   def write(self, BUFR_File bfile):
      c = cmessage.bufr_write_message(bfile._this, self._this)
      if c < 0 :
         raise IOError

   @property
   def edition(self):
      return self._this.edition

   @edition.setter
   def edition(self,value):
      self._this.edition=value

   @property
   def enforce(self):
      return self._this.enforce

   @enforce.setter
   def enforce(self,value):
      self._this.enforce=value
      
   def get_header(self):
      if self._this.header_string==NULL:
         return ""
      else:
         return self._this.header_string

   def get_obj(self):
      return self.obj

   def get_section1(self):
      s1=BufrSection1( <uint64_t>&self._this.s1 )
      return s1

      
cdef class BUFR_File:
   cdef FILE*  _this

   def __cinit__(self):
      self._this=NULL

   def __dealloc__(self):
      self.close()

   def open (self,basestring filename,basestring mode):
      """method use for open a BUFR file"""
      self._this = fopen( filename.encode(), mode.encode() )
      if self._this is NULL:
         raise IOError()

   def close(self):
      """method use for close a BUFR file"""
      if self._this is not NULL:
         fclose(self._this)
         self._this=NULL

   def  read(self):
      """method to use for reading a BUFR file, it returns the next BUFR_Message, or None at end of file"""
      cdef BUFR_Message _msg
      if self._this is not NULL:
         _msg=BUFR_Message()
         if (cmessage.bufr_read_message(self._this,&_msg._this) > 0):
            _msg.obj = PyCapsule_New(<void *>_msg._this,"BUFR_Message",<PyCapsule_Destructor>BUFR_Message.del_BUFR_Message)
            return _msg
      return None
