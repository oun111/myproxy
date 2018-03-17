#!/usr/bin/python3 -B

import random
import os
import re
from sys import argv


# TODO: use file to record these sets
vethList = set()
conList = set()

vethDict = dict()

MAX_NO = 65536
CONTAINER_PATH = "/var/run/netns/"


def fetchName(hdr,chk_set):
  name = ""

  while True:
    name = hdr+str(random.randint(1,MAX_NO))

    if bool(chk_set & set(name)) == False :
      chk_set.add(name)
      break 

  return name
    


def createVethPairs():

  veth0 = fetchName("veth_",vethList)
  veth1 = fetchName("veth_",vethList)

  # create the vethx pair
  print("creating veth pair {0} - {1}".format(veth0,veth1))
  os.system("ip link add "+veth0+" type veth peer name "+veth1)

  vethDict[veth0] = veth1

  return (veth0,veth1)



def clearVeths():

  for k in vethDict.keys():
    print("veth0: ",k)
    os.system("ip link delete "+k)



def createContainer():
  c = fetchName("con_",conList)
  cp = CONTAINER_PATH+c

  print("Creating container '{0}' with persistent file '{1}'".format(c,cp))
  # create persistent container 
  os.system("touch "+cp)
  os.system("unshare -pu --fork --net={0}".format(cp))



def clearContainers():

  for c in list(conList):
    cp = CONTAINER_PATH+c
    os.system("umount " + cp)
    os.system("rm " + cp)



def attachVeth():

  conItems = detectContainers()

  for c in conItems:

    print("dealing with container '{0}'".format(c))

    # create new veth pair
    veths = createVethPairs()
    veth1 = veths[1]

    # attach veth1 into container
    cp = CONTAINER_PATH+c
    print("attaching '{0}' into container '{1}'".format(veth1,cp))
    os.system("ip link set "+veth1+" netns "+cp)

    # find pid related to container 
    # res= str(os.popen("ps aux|grep unshare|grep -v \"sh -c\"|grep "+cp+" |awk '{print $2}' ").readlines())
    # pid = re.findall(r"\['(\d+).*",res)[0]
    # print ("pid: ", pid)

    # config the network interface in container
    os.system("ip netns exec {0} ifconfig {1}".format(c,veth1))



def createBridge():

  os.system("brctl addbr br0")



def dropBridge():

  os.system("brctl delbr br0")



def assignIpAddress():
  # TODO 
  exit(0)



def detectContainers():

  st = set()

  for __path,__subPaths,__files in os.walk(CONTAINER_PATH):
    for f in __files:
      st.add(f)

  return st



def loadVethList():

  with open("list.txt","a+") as mf:

    contents = mf.read()

    pos = contents.find("vlist:")

    if pos>=0:
      pos = contents.find(":",pos)+1
      ptn = re.compile("(\w+),(\w+)")
      res = ptn.findall(contents,pos)

      for n in res :
        vethList.add(n[0])
        vethList.add(n[1])
        vethDict[n[0]] = n[1]



def flushVethList():
  
  with open("list.txt","w+") as mf:

    mf.truncate()

    mf.write("vlist:")

    for n in vethDict:
      mf.write("({0},{1})".format(n,vethDict[n]))


def init():
  os.system("mkdir -p /var/run/netns/")

  loadVethList()


def release():
  flushVethList()
  #clearVeths()
  clearContainers()
  #dropBridge()


def help():
  print("help message: ")
  exit(-1)


def __main__():

  init()

  if len(argv)!=2:
    help()

  script,method = argv
  
  if method=='-a':
    attachVeth()

  elif method=='-c':
    createContainer()

  else:
    print("wrong argument '{0}'".format(method))
    help()

  #print(vethList)
  #print(vethDict)

  release()


if __name__=="__main__":
  __main__()


