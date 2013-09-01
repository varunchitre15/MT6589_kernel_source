#! /usr/bin/python 

import os
import sys
import re

def getFeatureConfig(path):
   """
      give the ProjectConfig.mk's path, return
      the dictionary of all featrue option!
   """
   configPath = os.path.normpath(path)
   if not os.path.exists(configPath):
      print >> sys.stderr,"the given path %s does not exist!" % configPath
      sys.exit(1)
   featrueDict = {}
   pattern = re.compile("(\S+)\s*=\s*(\S+)")
   fileInput = open(configPath,"r")
   fileInputLines = fileInput.readlines()
   fileInput.close()
   for line in fileInputLines:
      if line.startswith("#"):
         continue
      m = pattern.match(line)
      if m:
         featrueDict[m.group(1)] = m.group(2)
   return featrueDict   

#if __name__ == "__main__":
#   d = getFeatrueConfig("../../../config/oppo/ProjectConfig.mk")
#   for i in d.iteritems():
#      print "key=%s,v=%s" % (i[0],i[1])  
