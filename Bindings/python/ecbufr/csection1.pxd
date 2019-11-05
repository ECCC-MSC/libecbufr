# file: csection1.pxd

cdef extern from "bufr_message.h":

   ctypedef struct BufrSection1:
      short bufr_master_table;
      int   orig_centre
      short orig_sub_centre;
      short upd_seq_no;
      short flag;
      short msg_type;
      short msg_inter_subtype;
      short msg_local_subtype;
      short master_table_version;
      short local_table_version;
      short year;
      short month;
      short day;
      short hour;
      short minute;
      short second;
