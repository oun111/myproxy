#!/usr/bin/python3 -b

import ctypes
from ctypes import *

class zas(object):

  #m_dsn = None
  g_zasLib = None

  def __init__(self):
    so = ctypes.cdll.LoadLibrary

    #
    # TODO: set correct path of the cwpr library
    # 
    #self.g_zasLib = so("/mnt/sda7/work/zas/wrapper/libcwpr.0.0.1.so") 
    self.g_zasLib = so("/mnt/sda5/zyw/work/zas/wrapper/libcwpr.0.0.1.so") 


  def login(self,host,port,usr,pwd,db):
    """
    login to db
    """
    dsn = 'server={0};port={1};usr={2};pwd={3};' \
      'unix_socket=/tmp/mysql.sock;dbname={4};' \
      .format(host,port,usr,pwd,db)
    return self.g_zasLib.rlogon(c_char_p(bytes(dsn,encoding='utf-8')))


  def prepare(self,stmt):
    """
    prepare statement
    """
    return self.g_zasLib.prepare_stmt(c_char_p(bytes(stmt,encoding='utf-8')))


  ################################
  #
  # place holder insertions
  #
  ################################
  def insert_str(self,val):
    return self.g_zasLib.insert_str(c_char_p(bytes(val,encoding='utf-8')))


  def insert_const_str(self,val):
    return self.insert_str(val)


  def insert_bigint(self,val):
    return self.g_zasLib.insert_big_int(c_longlong(val))


  def insert_long(self,val):
    return self.g_zasLib.insert_long(c_long(val))


  def insert_int(self,val):
    return self.g_zasLib.insert_int(c_int(val))


  def insert_unsigned(self,val):
    return self.g_zasLib.insert_unsigned(c_int(val))


  def insert_short(self,val):
    return self.g_zasLib.insert_short(c_short(val))


  def insert_unsigned_short(self,val):
    return self.g_zasLib.insert_unsigned_short(c_ushort(val))


  def insert_double(self,val):
    return self.g_zasLib.insert_double(c_double(val))


  def insert_long_double(self,val):
    return self.insert_double(val)


  def insert_float(self,val):
    return self.g_zasLib.insert_float(c_float(val))


  ################################
  #
  # result set fetching
  #
  ################################

  def is_eof(self):
    return self.g_zasLib.is_eof()


  def fetch_str(self):
    val = ""
    cval = c_char_p(bytes(val,encoding='utf-8'))
    self.g_zasLib.fetch_str(cval)
    return str(cval.value, encoding="utf-8") 


  def fetch_int(self):
    cval = c_int()
    self.g_zasLib.fetch_int(byref(cval))
    return cval.value


  def fetch_bigint(self):
    cval = c_longlong()
    self.g_zasLib.fetch_bigint(byref(cval))
    return cval.value


  def fetch_long(self):
    cval = c_long()
    self.g_zasLib.fetch_long(byref(cval))
    return cval.value


  def fetch_unsigned(self):
    cval = c_uint()
    self.g_zasLib.fetch_unsigned(byref(cval))
    return cval.value


  def fetch_short(self):
    cval = c_short()
    self.g_zasLib.fetch_short(byref(cval))
    return cval.value


  def fetch_unsigned_short(self):
    cval = c_ushort()
    self.g_zasLib.fetch_unsigned_short(byref(cval))
    return cval.value


  def fetch_double(self):
    cval = c_double()
    self.g_zasLib.fetch_double(byref(cval))
    return cval.value


  def fetch_long_double(self):
    return self.fetch_double()


  def fetch_float(self):
    cval = c_float()
    self.g_zasLib.fetch_float(byref(cval))
    return cval.value


"""
def main():

  mz = zas()

  if mz.login("localhost",3306,"root","123","")!=0 :
    print("login fail\n")
    exit(-1)

  if mz.prepare('select *from test_db.test_tbl where id <:f1<int>')!=0 :
    print("prepare fail\n")
    exit(-1)

  if mz.insert_int(10)!=0 :
    exit(-1)

  while mz.is_eof()==0 :
    id = mz.fetch_int()
    name = mz.fetch_str()
    price = mz.fetch_double()
    size = mz.fetch_long()
    print("{0}: name: {1}, point: {2}, size: {3}\n".format(id,name,price,size))

  print("ok!\n")



if __name__ == "__main__":
  main()
"""

