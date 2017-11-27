#!/usr/bin/python3 -b

import sys

# TODO: set correct path of the zas.py 
sys.path.append("/mnt/sda5/zyw/work/zas/wrapper")

from python.zas import *

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

