#!/usr/bin/python3 -B


"""

  HOWTO create the root filesystem:

  1. Create an empty file:

     dd if=/dev/zero of=rootfs.ext4 bs=1024 count=50000


  2. Format the file with ext4 :

     mkfs.ext4 rootfs.ext4


  3. Mount the empty FS:
     
     mount -o loop rootfs.ext4 /mnt/fsXXX


  4. Copy files in it:

     cp -r ./fsNNN/* /mnt/fsXXX


  5. Save it:
  
     umount /mnt/fsXXX


"""


import random
import os
import re
from sys import argv


class Global(object):
  MAX_NO = 65536
  vethDict = {}
  conDict = {}
  CONTAINER_PATH = "/var/run/netns/"
  BR_NAME = "br0"
  ADDR_BASE = "138.1.123."
  ADDR_INDEX = 1
  VETH_CACHE = "./list.txt"
  FS_NAME = "rootfs.ext4"


def fetchName(hdr,chk_set):
  name = ""

  while True:
    name = hdr+str(random.randint(1,Global.MAX_NO))

    if bool(set(chk_set.keys()) & set(name)) == False and \
        bool(set(chk_set.values()) & set(name)) == False:
      return name 

  return None
    


def createVethPairs():

  veth0 = fetchName("veth_",Global.vethDict)
  if veth0==None:
    print("can't fetch name for veth0")
    exit(-1)

  veth1 = fetchName("veth_",Global.vethDict)
  if veth1==None:
    print("can't fetch name for veth1")
    exit(-1)

  # create the vethx pair
  print("creating veth pair {0} - {1}".format(veth0,veth1))
  os.system("ip link add "+veth0+" type veth peer name "+veth1)

  Global.vethDict[veth0] = veth1

  return (veth0,veth1)



def clearVeths():

  for k in list(Global.vethDict.keys()):
    os.system("ip link delete "+k)
    Global.vethDict.pop(k)



def createContainer():
  c = fetchName("con_",Global.conDict)
  if c==None:
    print("can't fetch name for new container")
    exit(-1)
  else:
    Global.conDict[c] = ''

  # mount filesystem
  mountFS(c)

  cp = Global.CONTAINER_PATH+c
  print("Creating container '{0}' with persistent file '{1}'".format(c,cp))
  print(
"""


xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  REMEMBER to run /bashrc1 MANUALLY  !!! enjoy !

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


"""
)

  # create persistent container 
  os.system("touch "+cp)
  os.system("unshare -pu --fork --net={0} chroot /mnt/{1} /bin/sh".format(cp,c))

  # un-mount filesystem
  unmountFS(c)

  return c



def clearContainers():

  for c in Global.conDict.keys():
    dropContainer(c)



def dropContainer(c):
  cp = Global.CONTAINER_PATH+c
  os.system("umount " + cp)
  os.system("rm " + cp)



def attachVeth():

  Global.ADDR_BASE = re.findall(r"(\w+\.\w+\.\w+)",Global.ADDR_BASE)[0] + "."

  for c in Global.conDict.keys():

    #print("dealing with container '{0}'".format(c))

    # create new veth pair
    veths = createVethPairs()
    veth1 = veths[1]

    # attach veth1 into container
    cp = Global.CONTAINER_PATH+c
    print("attaching '{0}' into container '{1}'".format(veth1,cp))
    os.system("ip link set "+veth1+" netns "+cp)

    # find pid related to container 
    # res= str(os.popen("ps aux|grep unshare|grep -v \"sh -c\"|grep "+cp+" |awk '{print $2}' ").readlines())
    # pid = re.findall(r"\['(\d+).*",res)[0]
    # print ("pid: ", pid)

    # config the network interface in container
    assignAddr(c,veth1,Global.ADDR_BASE+str(Global.ADDR_INDEX))
    Global.ADDR_INDEX += 1

    attachBridgeIf(veths[0])

    # activate the veth0 
    os.system("ifconfig {0} up".format(veths[0]))



def createBridge():
  os.system("brctl addbr " + Global.BR_NAME)
  os.system("ifconfig {0} up".format(Global.BR_NAME))


def attachBridgeIf(ifname):
  os.system("brctl addif {0} {1} ".format(Global.BR_NAME,ifname))  



def dropBridge():

  os.system("ifconfig {0} down".format(Global.BR_NAME))
  os.system("brctl delbr " + Global.BR_NAME)



def assignAddr(c,veth,ip):
  os.system("ip netns exec {0} ifconfig {1} {2} up".format(c,veth,ip))
  os.system("ip netns exec {0} ifconfig lo up".format(c))


def modifyBridgeAddr():

  os.system("ifconfig {0} {1}".format(Global.BR_NAME, \
    Global.ADDR_BASE+str(Global.ADDR_INDEX)))
  Global.ADDR_INDEX += 1


def loadCache():

  os.system("touch "+Global.VETH_CACHE)

  with open(Global.VETH_CACHE,"r+") as mf:

    contents = mf.read()

    # read the 'veth' items
    pos = contents.find("vlist:")

    if pos>=0:
      pos = contents.find(":",pos)+1
      ptn = re.compile("(\w+),(\w+)")
      res = ptn.findall(contents,pos)

      for n in res :
        Global.vethDict[n[0]] = n[1]

    # read the 'ip address base index'
    """
    pos = contents.find("aindex:")

    if pos>=0:
      pos = contents.find(":",pos)+1
      ptn = re.compile("(\d+)")
      res = ptn.findall(contents,pos)

      if res!=None:
        Global.ADDR_INDEX = int(res[0])
    """



def flushCache():
  
  with open(Global.VETH_CACHE,"w+") as mf:

    mf.truncate()

    # the veth list
    mf.write("vlist:")
    for n in Global.vethDict:
      mf.write("({0},{1})".format(n,Global.vethDict[n]))

    # the ip address index
    mf.write("\naindex: {0}".format(Global.ADDR_INDEX))



def loadContainerList():
  os.system("mkdir -p "+Global.CONTAINER_PATH)

  for __path,__subPaths,__files in os.walk(Global.CONTAINER_PATH):
    for f in __files:
      Global.conDict[f] = ''


def mountFS(c):
  os.system("mkdir -p /mnt/"+c)
  os.system("mount -o loop,ro {0} /mnt/{1}".format(Global.FS_NAME,c))



def unmountFS(c):
  os.system("umount /mnt/{0}/tmp/*".format(c))
  os.system("umount /mnt/{0}/*".format(c))
  os.system("umount /mnt/"+c)
  os.system("rm -rf /mnt/"+c)



def release():
  clearVeths()
  dropBridge()
  clearContainers()
  Global.ADDR_INDEX = 1



def help(script):
  print(
"""
{0} <options>: initialize linux containers

options:
  -c: create a linux container
  -a: attach 'veth' device to all containers
  -r: release all
  -i <address base>: specify ip address base in all containers
  -e <cache>: the cache file
  -f <root fs>: root filesystem 

""".format(script)
  )
  exit(-1)



def parse_arguments():
  method = ""

  for n in range(len(argv)):
    if n==0:
      continue 

    ax = argv[n]
    if ax[0]=='-' and ax!='-i' and ax!='-e' and ax!="-f":
      method = ax
    # ip address base
    if ax=='-i':
      n += 1
      if n<len(argv):
        Global.ADDR_BASE = argv[n]
    # the cache file
    elif ax=='-e':
      n += 1
      if n<len(argv):
        Global.VETH_CACHE = argv[n]
    # root fs file
    elif ax=='-f':
      n += 1
      if n<len(argv):
        Global.FS_NAME = argv[n]


  return method


def __main__():

  method = parse_arguments()

  loadContainerList()
  
  if method=='-a':
    loadCache()
    createBridge()
    modifyBridgeAddr()
    attachVeth()
    flushCache()

  elif method=='-c':
    c = createContainer()
    # this will be invoked after the container has exited
    dropContainer(c)

  elif method=='-r':
    loadCache()
    release()
    flushCache()

  elif method=='-h':
    help(argv[0])

  else:
    print("wrong argument '{0}'".format(method))
    help(argv[0])


if __name__=="__main__":
  __main__()


