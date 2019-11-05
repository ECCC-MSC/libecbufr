#!/usr/bin/python

import sys

from ecbufr.template import BUFR_Template as BT
from ecbufr.template import BufrDescValue
from ecbufr.tables import BUFR_Tables as Tables
from ecbufr.tables import EntryTableB as TB
from ecbufr.message import BUFR_Message as BM
from ecbufr.message import BUFR_File as BF
from ecbufr.dataset import BUFR_Dataset
from ecbufr.section1 import BufrSection1
from ecbufr.meta import BufrRTMD

import ecbufr

ecbufr.__doc__

if len(sys.argv)!=2:
   print "Usage:",sys.argv[0]," input_bufr_file"
   in_bufr=""
else:
   in_bufr=str(sys.argv[1])

print "Input BUFR:", in_bufr

def choose_bufrtable(tlist,version):
   nb=len(tlist)
   for i in range(nb):
      t=tlist[i]
      tmv=t.get_master_version()
      if tmv==version:
         print "Using Table version:",tmv
         return t
   print "Using Default Table version:",tlist[0].get_master_version()
   return tlist[0]

def try_bufrtable(t):
   b=t.fetch_tableB( 10001 )
   print b.descriptor,b.description
   del b
   d=t.fetch_tableD( 301025 )
   print d.descriptor,d.description
   print d.get_descriptors()
   del d


t_13=Tables()
t_13.load_TableB ( "../Tables/table_b_bufr-13", 0 )
t_13.load_TableD ( "../Tables/table_d_bufr-13", 0 )
print "Loaded Table master version:", t_13.get_master_version()

t_cmc=Tables()
t_cmc.load_cmc_tables()
print "Loaded CMC Table master version:", t_cmc.get_master_version()

try_bufrtable(t_cmc)

tables_list = []
tables_list.append ( t_cmc )
tables_list.append ( t_13 )

bdv=BufrDescValue(2)
bdv.set_desc( 0, 8034 )
bdv.set_desc( 1, 10034 )

bdv2=BufrDescValue(2)
bdv2.set_desc( 0, 8033 )
bdv2.set_desc( 1, 10034 )

print bdv.get_desc(0)
print bdv.get_desc(1)
print  len(bdv)
bt=BT()
bt.allocate(bdv,t_cmc,4)

bdv0=BufrDescValue(0)
bt2=BT()
bt2.allocate(bdv0,t_cmc,4)
bt2.add( bdv2, t_cmc )
bt2.finalize()
print "Template compare:", bt.compare( bt2 )

#bt2.save( "t2.template" )

try:
   bf=BF()
   print bf.open.__doc__
   print bf.read.__doc__
   bf.open( in_bufr, "rb" )
except Exception as inst:
   print "Cannot open file:",type(inst)
else:
   msg=bf.read()
   while isinstance(msg,BM):
      s1=msg.get_section1()
      msg_version=s1.master_table_version
      print "Message Table version:",msg_version
      print "Message edition:",msg.edition
      print "Orig Center:",s1.orig_centre
      print "bufr_master_table:",s1.bufr_master_table
      table=choose_bufrtable(tables_list,msg_version)
      dts=table.decode(msg)
      print "Subsets count:", dts.size()
      for i in range(len(dts)):
         print "subset:", i
         dss=dts.get_subset(i)
         print "Descriptors count:", dss.size()
         pos = dss.find( 4024, 0 )
         print "found 4024 at ", pos
         for j in range(len(dss)):
            dsc=dss.get_descriptor(j)
            print "descriptor:",j
            meta=dsc.get_rtmd()
            if isinstance(meta,BufrRTMD):
               print "{",
               nb=meta.len_nesting()
               for n in range(nb):
                  print meta.get_nesting(n),
               print "}",
            del meta
            print " ", dsc.descriptor, " ", dsc.get_value()
#            print "descriptor:",j," ", dsc.descriptor, " ", dsc.get_value(), " flags=",dsc.flags

      del dts
      del msg
      msg=bf.read()
   bf.close()
   del bf
