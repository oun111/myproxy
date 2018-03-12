#!/usr/bin/python3

import os
from sys import argv

if len(argv)!=2:
  print("needs 1 argument")
  exit(-1)
  
script,opt = argv

if opt=="0" or opt=="1":
  os.system("rm -f /tmp/myproxy*")
  os.system("rm -f ./__db*")
  os.system("killall myproxy_app")

  if opt=="1":
     os.system("sudo echo \"20000\" > /proc/sys/net/core/somaxconn")
     os.system("sudo echo \"20000\" > /proc/sys/net/ipv4/tcp_max_syn_backlog")
     os.system("ulimit -n 20000")

     os.system("./src/myproxy_app -c ./conf/configure_pressure_test.json &  ")
#    os.system("./src/myproxy_app -c ./conf/configure.json &  ")

