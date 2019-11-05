# file: tables.pyx

from libc.stdlib cimport malloc, free
from cpython.pycapsule cimport *
from ecbufr cimport ctables
from ecbufr cimport cbufr_api
from ecbufr.dataset import BUFR_Dataset
from ecbufr cimport cdataset
from ecbufr.cmessage cimport BUFR_Message

cdef del_BUFR_Tables(object obj):
   ptr = <ctables.BUFR_Tables *> PyCapsule_GetPointer(obj,"BUFR_Tables")
   if ptr is not NULL:
      ctables.bufr_free_tables(ptr)


cdef class BUFR_Tables:
   cdef ctables.BUFR_Tables* _this
   cdef object   obj

   def __cinit__(self):
      self._this = ctables.bufr_create_tables()
      if self._this is NULL:
            raise MemoryError()
      self.obj = PyCapsule_New(<void *>self._this,"BUFR_Tables",<PyCapsule_Destructor>del_BUFR_Tables)

   def __dealloc__(self):
      self._this=NULL

   def get_master_version(self):
      cdef ctables.BufrTablesSet this
      this=self._this.master
      return this.version

   def get_obj(self):
      return self.obj

   def load_TableB(self,basestring filename,int local):
      if local == 0:
         ctables.bufr_load_m_tableB(self._this,filename.encode())
      else:
         ctables.bufr_load_l_tableB(self._this,filename.encode())

   def load_TableD(self,basestring filename,int local):
      if local == 0:
         ctables.bufr_load_m_tableD(self._this,filename.encode())
      else:
         ctables.bufr_load_l_tableD(self._this,filename.encode())

   def  load_cmc_tables(self):
      cbufr_api.bufr_load_cmc_tables(self._this)

   def  fetch_tableB(self, int desc):
      ptr=ctables.bufr_fetch_tableB(self._this, desc)
      if ptr is not NULL:
         tb=EntryTableB()
         tb._this = ptr
         return tb
      return None

   def  fetch_tableD(self, int desc):
      this=ctables.bufr_fetch_tableD(self._this, desc)
      if this is not NULL:
         td=EntryTableD()
         td._this=this
         return td
      else:
         return None

   def decode(self, msg):
      mobj=msg.get_obj()
      tobj=self.get_obj()
      _dts = BUFR_Dataset.decode( mobj, tobj )
      return _dts

cdef class EntryTableB:
   cdef ctables.EntryTableB* _this_ptr
   cdef ctables.EntryTableB* _this

   def __cinit__(self):
      self._this_ptr = NULL
      self._this = self._this_ptr

   def __dealloc__(self):
       if self._this_ptr is not NULL:
          ctables.bufr_free_EntryTableB(self._this_ptr)

   def  allocate(self):
      if self._this_ptr is not NULL:
         self.deallocate(self)
      self._this_ptr = ctables.bufr_new_EntryTableB()
      if self._this_ptr is NULL:
            raise MemoryError()
      self._this = self._this_ptr

   def  copy(self,EntryTableB src):
      if self._this_ptr is NULL:
         raise MemoryError()
      ctables.bufr_copy_EntryTableB(self._this_ptr,src._this)

   @property
   def descriptor(self):
      return self._this.descriptor

   @property
   def description(self):
      return self._this.description

   @descriptor.setter
   def descriptor(self,value):
      self._this.descriptor=value


cdef class EntryTableD:
   cdef ctables.EntryTableD* _this_ptr
   cdef ctables.EntryTableD* _this

   def __cinit__(self):
      self._this_ptr = NULL
      self._this = self._this_ptr

   def __dealloc__(self):
      if self._this_ptr is not NULL:
         ctables.bufr_free_EntryTableD(self._this_ptr)

   def allocate(self,int desc,basestring name,descs):
      cdef int *my_ints

      dlen = len(descs)
      my_ints = <int *>malloc(dlen*4)
      if my_ints is NULL:
         raise MemoryError()

      for i in xrange(dlen):
         my_ints[i] = descs[i]
      self._this_ptr = ctables.bufr_new_EntryTableD(desc,name.encode(),len(name),my_ints,dlen)
      if self._this_ptr is NULL:
         raise MemoryError()
      self._this = self._this_ptr
      free(my_ints)

   @property
   def descriptor(self):
      return self._this.descriptor

   @property
   def description(self):
      return self._this.description

   def get_descriptors(self):
      cdef int d
      descs=[]
      for i in range(self._this.count):
         d=self._this.descriptors[i]
         descs.append(d)
      return descs

