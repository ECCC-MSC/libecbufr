"""This module is a Python wrapper for libECBUFR"""
# file: ecbufr.pyx

from ecbufr cimport cbufr_api
from libc.stdio cimport FILE, fopen, fclose

def initialize():
   print "Initializing libECBUFR Python bindings"
   cbufr_api.bufr_begin_api()
