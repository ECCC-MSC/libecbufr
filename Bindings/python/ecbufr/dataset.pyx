# file: ecbufr.pyx

from cpython.pycapsule cimport *
from ecbufr cimport cdataset
from ecbufr.cmessage  cimport  BUFR_Message
from ecbufr.ctables   cimport  BUFR_Tables
from ecbufr cimport  csection1
from ecbufr.section1 import BufrSection1
#from ecbufr.cdescriptor import  *
#from ecbufr cimport cdescriptor
from ecbufr.ctypes cimport *
from ecbufr.descriptor import  BufrDescriptor

#cdef del_BUFR_Dataset(object obj):
#   print "del_BUFR_Dataset called"


cdef class BUFR_Dataset:
   cdef cdataset.BUFR_Dataset* _this_ptr
   cdef cdataset.BUFR_Dataset* _this
#   cdef object  obj

   def __cinit__(self):
      self._this_ptr=NULL
      self._this=self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
          cdataset.bufr_free_dataset(self._this_ptr)

   def __len__(self):
      return self.size()

   def size(self):
      return cdataset.bufr_count_datasubset( self._this )

   def get_header(self):
      if self._this.header_string==NULL:
         return ""
      else:
         return self._this.header_string

   def get_s1(self):
      s1=BufrSection1( <uint64_t>&self._this.s1 )
      return s1

   @staticmethod
   def decode( msg, tbl ):
      cdef BUFR_Message *msg_ptr
      cdef BUFR_Tables  *tbl_ptr

      msg_ptr = <BUFR_Message *>PyCapsule_GetPointer(msg,"BUFR_Message")
      tbl_ptr = <BUFR_Tables *>PyCapsule_GetPointer(tbl,"BUFR_Tables")
      ds=BUFR_Dataset()
      ds._this_ptr = cdataset.bufr_decode_message( msg_ptr, tbl_ptr )
      if ds._this_ptr is NULL:
         raise RuntimeError("decoding error occurred")
      ds._this=ds._this_ptr
#      ds.obj = PyCapsule_New(<void *>ds._this,"BUFR_Dataset",<PyCapsule_Destructor>del_BUFR_Dataset)
      return ds

   def get_subset(self, int pos):
      dss = DataSubset()
      dss._this = cdataset.bufr_get_datasubset( self._this, pos )
      return dss

cdef class DataSubset:
   cdef cdataset.DataSubset* _this

   def __cinit__(self):
      self._this=NULL

   def __dealloc__(self):
      self._this=NULL

   def __len__(self):
      return self.size()

   def size(self):
      return cdataset.bufr_datasubset_count_descriptor( self._this )

   def get_descriptor(self, int pos):
      dsc = BufrDescriptor()
      dsc.assign( <uint64_t>cdataset.bufr_datasubset_get_descriptor( self._this, pos ) )
      return dsc

   def use_descriptor(self, int pos, object dsc):
      if not isinstance(dsc,BufrDescriptor):
         dsc = BufrDescriptor()
      dsc.assign( <uint64_t>cdataset.bufr_datasubset_get_descriptor( self._this, pos ) )
      return dsc

   def find(self, int dsc, int start ):
      pos = cdataset.bufr_subset_find_descriptor( self._this, dsc, start )
      return pos
