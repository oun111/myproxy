#!/usr/bin/python3 -B


from sys import argv
import os
import re


def search_keyword(ptn,__path):

  res = []

  for __path,__subPaths,__files in os.walk(__path):

    #scanning file under __path
    for f in __files:

      # filter only myp*.log files
      if bool(re.match('myp.*\.log.*',f))!=True :
        continue

      #print("file ",f)

      # read whole file
      m_fd = open(__path+f)
      contents = m_fd.read()
      m_fd.close()

      tmp = re.findall(ptn,contents)

      res += tmp ;

  return res


def __main__():

  if len(argv)!=2:
    print("needs the myproxy log path")
    exit(0)

  script,p = argv

  # find clients that have acquired xa items
  inres = search_keyword(r"acquired xa \d+ cid (\d+)",p)
  #print("in: ",inres)

  # find clients that released their xa items
  outres = search_keyword(r"release xaid \d+\(\w+\) cid (\d+)",p)
  #print("out: ",outres)

  """
  if len(inres) < len(outres):
    print("fatal: abnormal xa release")
  """

  # find differences between xa acquisitions and release
  diff = set(inres).difference(set(outres))
  if len(diff)>0:
    print("client(s) that has not release xa: {0}".format(diff))
  else: 
    print("great! all XAs are released!")


if __name__=="__main__":
  __main__()


