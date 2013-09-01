#!/proj/map/bin/python/bin/python


import re,os,sys

files = [re.sub(r"[\r\n]","",x.strip()) for x in open("movelist","r").readlines()]

if len(sys.argv)>2:
  files = [sys.argv[2]]

for item in filter(lambda x:x,files):
  head,tail = os.path.split(item)
  if not head: head = "."
  #print ("mkdir -p ../0/%s"%head)
  #print ("mv %s ../0/%s"%(item,head))
  os.system("mkdir -p ../0/%s"%head)
  os.system("mkdir -p %s"%head)
  if len(sys.argv)>1 and sys.argv[1]=="back":
    os.system("mv ../0/%s %s"%(item,head))
  else: 
    os.system("mv %s ../0/%s"%(item,head))

