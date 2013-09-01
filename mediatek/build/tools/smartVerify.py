#! /usr/bin/python

import xml.dom.minidom as xdom
from optparse import OptionParser
import shutil
import sys
import os
import re
import zipfile

parser = OptionParser(usage="usage: %prog [options] releaseSrc project releasePolicyXml",version="%prog 1.0")
parser.add_option("-d","--dump",action="store_true",help="dump the binary modules dependency information")
(options,args) = parser.parse_args()
if len(args) != 3:
    parser.print_help()
    sys.exit(1)

class Arguments(object):pass
ARGUMENTS = Arguments()
ARGUMENTS.releaseSrc = os.path.abspath(args[0])
ARGUMENTS.project = args[1]
ARGUMENTS.xml = os.path.abspath(args[2])

# check the arguments correctness
def checkArgument(argu):
    """ check the argument """
    if not os.path.exists(argu.releaseSrc):
        print >> sys.stderr,"the input releaseSrc '%s' does not exist!" % argu.releaseSrc
        sys.exit(1)
    if not os.path.exists(argu.xml):
        print >> sys.stderr,"the input xml '%s' does not exist!" % argu.xml
        sys.exit(1)
    projTargetFolder = "%s/out/target/product/%s" % (argu.releaseSrc,argu.project)
    if not os.path.exists(projTargetFolder):
        print >> sys.stderr,"the input project '%s' is illegal or the codebase '%s' had not full built yet!please check it!" % (argu.project,argu.releaseSrc)
        sys.exit(1)

checkArgument(ARGUMENTS)

# define our own  XML-DOM class for custom release policy
class XmlDom(object):
    def __init__(self,xml):
        self.xmlDom = xdom.parse(xml)

    def getRoot(self):
        return self.xmlDom.documentElement

    def getDirList(self):
        root = self.getRoot()
        dirElement = root.getElementsByTagName("DirList")[0].getElementsByTagName("ReleaseDirList")[0].getElementsByTagName("Dir")
        dirList = map(str,[item.firstChild.nodeValue for item in dirElement if item.firstChild is not None])
        return dirList

    def getUnreleaseDirList(self):
        root = self.getRoot()
        dirElement = root.getElementsByTagName("DirList")[0].getElementsByTagName("UnReleaseDirList")[0].getElementsByTagName("Dir")
        dirList = map(str,[item.firstChild.nodeValue for item in dirElement if item.firstChild is not None])
        return dirList

    def getFileList(self):
        root = self.getRoot()
        fileElement = root.getElementsByTagName("FileList")[0].getElementsByTagName("ReleaseFileList")[0].getElementsByTagName("File")
        fileList = map(str,[item.firstChild.nodeValue for item in fileElement if item.firstChild is not None])
        return fileList

    def getUnreleaseFileList(self):
        root = self.getRoot()
        fileElement = root.getElementsByTagName("FileList")[0].getElementsByTagName("UnReleaseFileList")[0].getElementsByTagName("File")
        fileList = map(str,[item.firstChild.nodeValue for item in fileElement if item.firstChild is not None])
        return fileList

    def getKernelSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("KernelRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getKernelBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("KernelRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getAppSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("APPRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getAppBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("APPRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getAndroidSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("AndroidRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getAndroidBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("AndroidRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getFrameworkSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getFrameworkBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getFrameworkPartialList(self):
        root = self.getRoot()
        partialElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("PartialSourceList")[0].getElementsByTagName("PartialSource")
        frameworkDict = {}
        for x in partialElement:
            module = str(x.getAttribute("module"))
            base = str(x.getAttribute("base"))
            binaryList = map(str,[item.firstChild.nodeValue for item in x.getElementsByTagName("Binary") if item.firstChild is not None])
            d = {}
            d["base"] = base
            d["binary_list"] = binaryList
            frameworkDict.setdefault(module,[]).append(d) 
        return frameworkDict

#end XmlDom

# create custom release policy DOM
dom = XmlDom(ARGUMENTS.xml)

###################################################
#       release relative path definition
###################################################

out = "out"
outTarget = os.path.join(out,"target")
outProduct = os.path.join(outTarget,"product",ARGUMENTS.project)
outSystem = os.path.join(outProduct,"system")
outCommon = os.path.join(outTarget,"common")
outIntermediate = os.path.join(outProduct,"obj")
outCommonIntermediate = os.path.join(outCommon,"obj")
outHost = os.path.join(out,"host","linux-x86")
outHostBin = os.path.join(outHost, "bin")
outHostIntermediate = os.path.join(outHost,"obj")

vendor = "vendor/mediatek/%s/artifacts" % ARGUMENTS.project

# the target module release path
archiveFolder = os.path.join(ARGUMENTS.releaseSrc,outIntermediate,"STATIC_LIBRARIES")
sharelibFolder = os.path.join(ARGUMENTS.releaseSrc,outSystem,"lib")
executeBinFolder = os.path.join(ARGUMENTS.releaseSrc,outSystem,"bin")
executeXbinFolder = os.path.join(ARGUMENTS.releaseSrc,outSystem,"xbin")
appSrcSystem = os.path.join(ARGUMENTS.releaseSrc,outSystem,"app")
frameworkFolder = os.path.join(ARGUMENTS.releaseSrc,outSystem,"framework")

sharelibIntermediate = os.path.join(ARGUMENTS.releaseSrc,outIntermediate,"lib")
frameworkIntermediate = os.path.join(ARGUMENTS.releaseSrc,outCommonIntermediate,"JAVA_LIBRARIES")
appIntermediate = os.path.join(ARGUMENTS.releaseSrc,outCommonIntermediate,"APPS")

# the host module release path
archiveHostFolder = os.path.join(ARGUMENTS.releaseSrc,outHostIntermediate,"STATIC_LIBRARIES")
sharelibHostFolder = os.path.join(ARGUMENTS.releaseSrc,outHost,"lib")
sharelibHostIntermediate = os.path.join(ARGUMENTS.releaseSrc,outHostIntermediate,"lib")
executeHostFolder = os.path.join(ARGUMENTS.releaseSrc,outHostBin)
frameworkHostFolder = os.path.join(ARGUMENTS.releaseSrc,outHost,"framework")

# the vendor relative paths that save the bianry release module
archiveVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outIntermediate,"STATIC_LIBRARIES")
sharelibVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outSystem,"lib")
sharelibVendorIntermediate = os.path.join(ARGUMENTS.releaseSrc,vendor,outIntermediate,"lib")
executeBinVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outSystem,"bin")
executeXbinVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outSystem,"xbin")
frameworkVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outSystem,"framework")
frameworkVendorIntermediate = os.path.join(ARGUMENTS.releaseSrc,vendor,outCommonIntermediate,"JAVA_LIBRARIES")
appDestVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outSystem,"app")
kernelDestVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,"kernel")

archiveHostVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outHostIntermediate,"STATIC_LIBRARIES")
sharelibHostVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outHost,"lib")
sharelibHostVendorIntermediate = os.path.join(ARGUMENTS.releaseSrc,vendor,outHostIntermediate,"lib")
executeHostVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outHostBin)
frameworkHostVendor = os.path.join(ARGUMENTS.releaseSrc,vendor,outHost,"framework")
destCls = os.path.join(ARGUMENTS.releaseSrc,vendor,"cls")
destJar = os.path.join(ARGUMENTS.releaseSrc,vendor,"jar")

# the LOCAL_PATH -> module_id record file path
releasePath = os.path.join(ARGUMENTS.releaseSrc,outProduct,"release")
releasePathHash = os.path.join(releasePath,"path_module_maptable")
installModulePathHash = os.path.join(releasePath,"module_installedpath_maptable")
# define a module_id list which save all the binary release modules' module_id
# this list is used to generate the module dependency for MP release
binModules = []

##################################################
# get the LOCAL_PATH -> module_id mapping table
##################################################

def getReleasePathTable():
    if not os.path.exists(releasePathHash):
        print >> sys.stderr,"Error!the path map file '%s' does not exist!" % releasePathHash
        sys.exit(1)
    pathTable = {}
    pattern = re.compile("(\S+)\s*->\s*(\S+)")
    output = open(releasePathHash,"r")
    for oput in output.readlines():
        match = pattern.match(oput)
        if match:
            key = match.group(1)
            key = re.sub("//", "/", key)
            value = match.group(2)
            value = re.sub("//", "/", value)
            pathTable.setdefault(key,[]).append(value)
    output.close()
    return pathTable

def getReleaseModuleTable():
    if not os.path.exists(releasePathHash):
        print >> sys.stderr,"Error!the path map file '%s' does not exist!" % releasePathHash
        sys.exit(1)
    moduleTable = {}
    pattern = re.compile("(\S+)\s*->\s*(\S+)")
    output = open(releasePathHash,"r")
    for oput in output.readlines():
        match = pattern.match(oput)
        if match:
            key = match.group(2)
            key = re.sub("//", "/", key)
            value = match.group(1)
            value = re.sub("//", "/", value)
            moduleTable[key] = value
    output.close()
    return moduleTable

def getInstalledPathTable():
    if not os.path.exists(installModulePathHash):
        print >> sys.stderr,"Error!the path map file '%s' does not exist!" % installModulePathHash
        sys.exit(1)
    moduleTable = {}
    pattern = re.compile("(\S+)\s*->\s*(\S+)")
    output = open(installModulePathHash,"r")
    for oput in output.readlines():
        match = pattern.match(oput)
        if match:
            key = match.group(1)
            key = re.sub("//", "/", key)
            value = match.group(2)
            value = re.sub("//", "/", value)
            moduleTable[key] = value
    output.close()
    return moduleTable

pathModuleIDTable = getReleasePathTable()
moduleIDPathTable = getReleaseModuleTable()
moduleIDInstalledPathTable = getInstalledPathTable()

###################################################
#    all class definitions of the ReleaseType
###################################################

class KernelRelease(object):
    def __init__(self):
        """ kernel release initialization """
        self.sources = dom.getKernelSourceList()
        self.binarys = dom.getKernelBinaryList()
        self.src = ARGUMENTS.releaseSrc
        self.kernelDestVendor = kernelDestVendor

    def getKernelSourceList(self):
        return self.sources

    def getKernelBinaryList(self):
        return self.binarys

    def binaryRelease(self):
        for kb in self.binarys:
            makefile = os.path.join(self.src,kb,"Makefile")
            if os.path.exists(makefile):
                destDir = os.path.join(self.src,kb)
                self.releaseKo(kb)
                if os.path.exists(destDir):
                    os.system("find %s -type f -name '*.c' -exec rm -f {} \;" % destDir)
                destMakefile = os.path.join(destDir,"Makefile")
                if not os.path.exists(os.path.join(self.src,kb)):
                    os.makedirs(os.path.join(self.src,kb))
                os.system("touch %s;echo 'obj-  := dummy.o' > %s" % (destMakefile,destMakefile))
            else :
                destDir = os.path.join(self.src,kb)
                if os.path.exists(destDir):
                    os.system("find %s -type f -name '*.c' -exec rm -f {} \;" % destDir)
                self.releaseObj(kb)
                self.releaseMak(kb)
 
    def release(self):
        self.binaryRelease()
    
    def releaseMak(self,kbFolder):
        srcDir = os.path.join(self.src,kbFolder)
        srcObjs = map(lambda x:x.rstrip(),list(os.popen("find %s -name '*.o' -exec basename {} \\;" % srcDir)))
        pathList = kbFolder.split('/')
        for i in range(len(pathList)-1,0,-1):
            makDir = '/'.join(pathList[:i])
            makefile = os.path.join(os.path.join(self.src, makDir), "Makefile")
            if os.path.exists(makefile):
                writefile = []
                makefileOutput = open(makefile,"r")
                for line in makefileOutput.readlines():
                    for pattern in srcObjs:
                        matchObj = re.match(r'^\s*obj-', line, re.M)
                        if matchObj:
                            break;
                        line = re.sub(pattern+'\\b',os.path.splitext(pattern)[0]+'.module',line) 
                        line = re.sub(os.path.splitext(pattern)[0]+'.module.new'+'\\b',pattern+'.new',line)
                    writefile.append(line)
                makefileOutput.close()
                makefileInput = open(makefile, "w")
                makefileInput.writelines(writefile)
                makefileInput.close()

    def releaseObj(self,kbFolder):
        srcDir = os.path.join(self.src,kbFolder)
        srcObjs = map(lambda x:x.rstrip(),list(os.popen("find %s -name '*.o'" % srcDir)))
        for obj in srcObjs:
            obj_module = "%s.module" % os.path.splitext(os.path.basename(obj))[0]
            destDir = os.path.join(self.kernelDestVendor,os.path.dirname(obj[len(self.src)+1:]))
            if not os.path.exists(destDir):
                os.makedirs(destDir)
            print >> sys.stdout,"Kernel Binary(.o) Release:release %s ..." % obj
            os.system("rsync -a %s %s/%s" % (obj,destDir,obj_module))

    def releaseKo(self,kbFolder):
        koDir = os.path.join(sharelibVendor,"modules")
        if not os.path.exists(koDir):
            os.system("mkdir -p %s" % koDir)
        srcDir = os.path.join(self.src,kbFolder)
        srcKos = list(os.popen("find %s -name '*.ko'" % srcDir))
        for ko in srcKos:
            ko = ko.rstrip()
            print >> sys.stdout,"Kernel Binary(.ko) Release:release %s ..." % ko
            os.system("cp -a %s %s" % (os.path.join(sharelibFolder,"modules",os.path.basename(ko)),koDir))

#end KernelRelease

class AndroidRelease(object):
    def __init__(self):
        """ android release initialization """
        self.sources = dom.getAndroidSourceList()
        self.binarys = dom.getAndroidBinaryList()
        self.src = ARGUMENTS.releaseSrc
        self.archiveFolder = archiveFolder 
        self.sharelibFolder = sharelibFolder
        self.executeBinFolder = executeBinFolder
        self.executeXbinFolder = executeXbinFolder 
        self.sharelibIntermediate = sharelibIntermediate
        self.sharelibHostIntermediate = sharelibHostIntermediate

        self.frameworkFolder = frameworkFolder
        self.frameworkIntermediate = frameworkIntermediate
        self.frameworkHostFolder = frameworkHostFolder
        self.frameworkVendor = frameworkVendor
        self.frameworkVendorIntermediate = frameworkVendorIntermediate
        self.frameworkHostVendor = frameworkHostVendor

        self.archiveHostFolder = archiveHostFolder
        self.sharelibHostFolder = sharelibHostFolder
        self.executeHostFolder = executeHostFolder

        self.archiveVendor = archiveVendor
        self.sharelibVendor = sharelibVendor
        self.sharelibHostVendorIntermediate = sharelibHostVendorIntermediate
        self.executeBinVendor = executeBinVendor
        self.executeXbinVendor = executeXbinVendor
        self.sharelibVendorIntermediate = sharelibVendorIntermediate
        
        self.archiveHostVendor = archiveHostVendor
        self.sharelibHostVendor = sharelibHostVendor
        self.executeHostVendor = executeHostVendor

        self.releasePath = releasePath
        self.releasePathTable = pathModuleIDTable
        self.releaseModuleTable = moduleIDPathTable
        self.intallledModulePathTable = moduleIDInstalledPathTable

    def transformModuleId(self,moduleName):
        mdList = ["MODULE.TARGET.SHARED_LIBRARIES.%s" % moduleName,
                  "MODULE.TARGET.STATIC_LIBRARIES.%s" % moduleName,
                  "MODULE.TARGET.EXECUTABLES.%s" % moduleName,
                  "MODULE.TARGET.JAVA_LIBRARIES.%s" % moduleName,
                  "MODULE.HOST.SHARED_LIBRARIES.%s" % moduleName,
                  "MODULE.HOST.STATIC_LIBRARIES.%s" % moduleName,
                  "MODULE.HOST.EXECUTABLES.%s" % moduleName,
                  "MODULE.HOST.JAVA_LIBRARIES.%s" % moduleName]
        return mdList

    def binaryRelease(self):
        for dr in self.binarys:
            flag = True 
            if dr.find("/") == -1:
                mdIdList = self.transformModuleId(dr)
                for md in mdIdList:
                    if self.releaseModuleTable.has_key(md):
                        # -------------------------------------
                        # save the moduleIdLIst into binModules
                        binModules.extend(md)
                        # -------------------------------------
                        flag = False 
                        # remove the destination relative part
                        destPart = os.path.join(self.src,self.releaseModuleTable.get(md))
                        self.copyBinary(md)
                        if os.path.exists(destPart):
                            os.system("find %s -type f ! -name '*.h' -exec rm -f {} \;" % destPart)
                        self.releaseHeader(self.releaseModuleTable.get(md))
            if flag:    
                srcPart = os.path.join(self.src,dr)
                if not os.path.exists(srcPart):
                    print >> sys.stderr,"Error!Android Release Binary Directory '%s' does not exists!" % srcPart
                    sys.exit(6)
                # remove the destination relative part
                destPart = os.path.join(self.src,dr)
                self.getBinary(dr)
                if os.path.exists(destPart):
                    os.system("find %s -type f ! -name '*.h' -exec rm -f {} \;" % destPart)

    def getBinary(self,drFolder):
        drFolder = os.path.join(self.src,drFolder)
        # find all the binary release Android.mk under given folder
        androidMkList = map(lambda x:x.rstrip(),list(os.popen("find %s -name Android.mk" % drFolder)))
        androidMkPaths = map(lambda x:os.path.dirname(x),androidMkList)
        # release binaries through the file "path_module_maptable"
        for adrPath in androidMkPaths:
            # adrPath is an absolute path,so for using the LOCAL_PATH -> module_id mapping relationship,remove the prefix(self.src)
            adrPath = adrPath[len(self.src)+1:]
            # touch the destination Android.mk
            destAndroidMk = os.path.join(self.src,adrPath,"Android.mk")
            if not os.path.exists(os.path.join(self.src,adrPath)):
                os.makedirs(os.path.join(self.src,adrPath))
            os.system("touch %s;echo '#empty' > %s" % (destAndroidMk,destAndroidMk))
            if self.releasePathTable.has_key(adrPath):
                moduleIdList = self.releasePathTable.get(adrPath)
                for md in moduleIdList:
                    self.copyBinary(md)
                # -------------------------------------
                # save the moduleIdLIst into binModules
                binModules.extend(moduleIdList)
                # -------------------------------------

    def copyInstalledModule(self,moduleId):
        if self.intallledModulePathTable.has_key(moduleId):
            installedModule = self.intallledModulePathTable.get(moduleId)
            installedPath = os.path.dirname(installedModule)
            installedPath_dst = os.path.join(self.src, vendor, installedPath)
            installedModule_src = os.path.join(self.src, installedModule)
            if not os.path.exists(installedModule_src):
                print >> sys.stdout,"Android Binary Release: %s does not exist..." % installedModule_src
            else:
                if not os.path.exists(installedPath_dst):
                    os.system("mkdir -p %s" % installedPath_dst)
                print >> sys.stdout,"Android Binary Release:release %s ..." % installedModule
                os.system("cp -f %s %s" % (installedModule_src, installedPath_dst))

    def copyBinary(self,moduleId):
        self.copyInstalledModule(moduleId)
        moduleIdPattern = re.compile("MODULE\.(.*?)\.(.*?)\.(.*)")
        moduleMatch = moduleIdPattern.match(moduleId)
        if moduleMatch:
            moduleTarget = moduleMatch.group(1)
            moduleClass = moduleMatch.group(2)
            moduleName = moduleMatch.group(3)
            if moduleClass == "STATIC_LIBRARIES":
                if moduleTarget == "TARGET":
                    source = os.path.join(self.archiveFolder,"%s_intermediates/%s.a" % (moduleName,moduleName))
                    destination = os.path.join(self.archiveVendor,"%s_intermediates/%s.a" % (moduleName,moduleName))
                elif moduleTarget == "HOST":
                    source = os.path.join(self.archiveHostFolder,"%s_intermediates/%s.a" % (moduleName,moduleName))
                    destination = os.path.join(self.archiveHostVendor,"%s_intermediates/%s.a" % (moduleName,moduleName))
                if os.path.exists(source):
                    dirPath = os.path.dirname(destination)
                    if not os.path.exists(dirPath):
                        os.makedirs(dirPath)
                    print >> sys.stdout,"Android Binary Release:release %s ..." % source
                    os.system("rsync -a %s %s" % (source,destination))
            elif moduleClass == "SHARED_LIBRARIES":
                if moduleTarget  == "TARGET":
                    # copy the share library to TARGET_OUT_INTERMEDIATE_LIBRARIES for dependency
                    source = os.path.join(self.sharelibIntermediate,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibVendorIntermediate,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"Android Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))
                elif moduleTarget == "HOST":
                    source = os.path.join(self.sharelibHostIntermediate,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibHostVendorIntermediate,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"Android Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))
            elif moduleClass == "JAVA_LIBRARIES":
                if moduleTarget == "HOST":
                    source = os.path.join(self.frameworkHostFolder,"%s.jar" % moduleName)     
                    destination = os.path.join(self.frameworkHostVendor,"%s.jar" % moduleName)
                    if not os.path.exists(source):
                        print >> sys.stderr,"Error!Android Release Binary '%s' does not exists!" % source
                        sys.exit(7)
                    if not os.path.exists(destination):
                        dirPath = os.path.dirname(destination)
                        os.makedirs(dirPath)
                    print >> sys.stdout,"Android Binary Release:release %s ..." % source
                    os.system("rsync -a %s %s" % (source,destination))
                if moduleTarget == "TARGET":
                    sourceSysJar = os.path.join(self.frameworkFolder,"%s.jar" % moduleName)
                    destinationSysJar = os.path.join(self.frameworkVendor,"%s.jar" % moduleName)
                    sourceJavalibJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    destinationJavalibJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    sourceClassesJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    destinationClassesJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    if os.path.exists(sourceSysJar):
                        self.copyJar(sourceSysJar,destinationSysJar)
                    if os.path.exists(sourceJavalibJar):
                        self.copyJar(sourceJavalibJar,destinationJavalibJar)
                    if os.path.exists(sourceClassesJar):
                        self.copyJar(sourceClassesJar,destinationClassesJar)

    def copyJar(self,src,dest):
        dirPath = os.path.dirname(dest)
        if not os.path.exists(dirPath):
            os.makedirs(dirPath)
        print >> sys.stdout,"Android Binary Release:release %s ..." % src
        os.system("rsync -a %s %s" % (src,dest))

    def release(self):
        self.binaryRelease()

# end  AndroidRelease

class FrameworkRelease(object):
    def __init__(self):
        """ framework release initialization """
        self.sources = dom.getFrameworkSourceList()
        self.binarys = dom.getFrameworkBinaryList()
        self.partials = dom.getFrameworkPartialList()
        self.src = ARGUMENTS.releaseSrc
        self.frameworkFolder = frameworkFolder
        self.frameworkIntermediate = frameworkIntermediate
        self.frameworkHostFolder = frameworkHostFolder
        self.frameworkVendor = frameworkVendor
        self.frameworkVendorIntermediate = frameworkVendorIntermediate
        self.frameworkHostVendor = frameworkHostVendor
        self.appIntermediate = appIntermediate
        self.destCls = destCls
        self.destJar = destJar
        
        self.releasePathTable = pathModuleIDTable

    def binaryRelease(self):
        for fw in self.binarys:
            # remove the destination relative part
            destPart = os.path.join(self.src,fw)
            self.getBinary(fw)
            if os.path.exists(destPart):
                os.system("find %s -type f -name '*' -exec rm -f {} \;" % destPart)

    def getBinary(self,fwFolder):
        fwFolder = os.path.join(self.src,fwFolder)
        if not os.path.exists(fwFolder):
            print >> sys.stderr,"Error!Framework Release Source Directory/File %s does not exists!" % fwFolder 
            sys.exit(7)
        androidMkList = map(lambda x:x.rstrip(),list(os.popen("find %s -name Android.mk" % fwFolder)))
        androidMkPaths = map(lambda x:os.path.dirname(x),androidMkList)
        # release binaries through the file "path_module_maptable"
        for adrPath in androidMkPaths:
            # adrPath is an absolute path,so for using the LOCAL_PATH -> module_id mapping relationship,remove the prefix(self.src)
            adrPath = adrPath[len(self.src)+1:]
            if self.releasePathTable.has_key(adrPath):
                moduleIdList = self.releasePathTable.get(adrPath)
                for md in moduleIdList:
                    self.copyBinary(md)
                # -------------------------------------
                # save the moduleIdLIst into binModules
                binModules.extend(moduleIdList)
                # -------------------------------------

    def copyBinary(self,moduleId):
        moduleIdPattern = re.compile("MODULE\.(.*?)\.(.*?)\.(.*)")
        moduleMatch = moduleIdPattern.match(moduleId)
        if moduleMatch:
            moduleTarget = moduleMatch.group(1)
            moduleClass = moduleMatch.group(2)
            moduleName = moduleMatch.group(3)
            if moduleClass == "JAVA_LIBRARIES":
                if moduleTarget == "HOST":
                    source = os.path.join(self.frameworkHostFolder,"%s.jar" % moduleName)     
                    destination = os.path.join(self.frameworkHostVendor,"%s.jar" % moduleName)
                    if not os.path.exists(source):
                        print >> sys.stderr,"Error!Framework Release Binary '%s' does not exists!" % source
                        sys.exit(7)
                    if not os.path.exists(destination):
                        dirPath = os.path.dirname(destination)
                        os.makedirs(dirPath)
                    print >> sys.stdout,"Framework Binary Release:release %s ..." % source
                    os.system("rsync -a %s %s" % (source,destination))
                if moduleTarget == "TARGET":
                    sourceSysJar = os.path.join(self.frameworkFolder,"%s.jar" % moduleName)
                    destinationSysJar = os.path.join(self.frameworkVendor,"%s.jar" % moduleName)
                    sourceJavalibJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    destinationJavalibJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    sourceClassesJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    destinationClassesJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    if os.path.exists(sourceSysJar):
                        self.copyJar(sourceSysJar,destinationSysJar)
                    if os.path.exists(sourceJavalibJar):
                        self.copyJar(sourceJavalibJar,destinationJavalibJar)
                    if os.path.exists(sourceClassesJar):
                        self.copyJar(sourceClassesJar,destinationClassesJar)

    def copyJar(self,src,dest):
        dirPath = os.path.dirname(dest)
        if not os.path.exists(dirPath):
            os.makedirs(dirPath)
        print >> sys.stdout,"Framework Binary Release:release %s ..." % src
        os.system("rsync -a %s %s" % (src,dest))
        
    def partialRelease(self):
        for fw in self.partials:
            if fw == '':
                continue
            appFlag = False
            classesJar = os.path.join(self.frameworkIntermediate,"%s_intermediates/classes.jar" % fw)
            for element in self.partials[fw]: 
                base = element["base"]
                binary_list = element["binary_list"]
                if not os.path.exists(classesJar):
                    classesJar = os.path.join(self.appIntermediate,"%s_intermediates/classes.jar" % fw)
                    if not os.path.exists(classesJar):
                        print >> sys.stderr,"Error!Framework Partial Release Binary '%s' does not exists!" % classesJar
                        sys.exit(7)
                    else: appFlag = True
                zfile = zipfile.ZipFile(classesJar)
                if appFlag:
                    classes = os.path.join(self.appIntermediate,"%s_intermediates/classes" % fw)
                else:
                    classes = os.path.join(self.frameworkIntermediate,"%s_intermediates/classes" % fw)
                zfile.extractall(classes)
                if os.path.exists(".tmp"):
                    shutil.rmtree(".tmp")
                print >> sys.stdout,"Framework Partial Release ..."
                # release relative class files
                for bi in binary_list:
                    if not os.path.exists(os.path.join(self.src,base,bi)):
                        print >> sys.stderr,"Error!Framework Partial Release '%s' does not exists!" % os.path.join(self.src,base,bi)
                        sys.exit(7)
                    if os.path.isdir(os.path.join(self.src,base,bi)):
                        # remove the destination binary relative directory
                        os.system("find %s -type f -name '*' -exec rm -f {} \;" % os.path.join(self.src,base,bi))
                        os.system("mkdir -p %s" % os.path.join(".tmp",bi))
                        os.system("mkdir -p %s" % os.path.join(self.destCls,bi))
                        os.system("mkdir -p %s" % os.path.join(self.destJar,fw))
                        os.system("cp -a %s %s" % (os.path.join(classes,bi,"*"),os.path.join(".tmp",bi)))
                        os.system("cp -a %s %s" % (os.path.join(classes,bi,"*"),os.path.join(destCls,bi)))
                    elif os.path.isfile(os.path.join(self.src,base,bi)):
                        # remove the destination binary relative file
                        os.system("rm -rf %s" % os.path.join(self.src,base,bi))
                        biPath = os.path.dirname(bi)
                        biBaseName = os.path.splitext(bi)[0]
                        os.system("mkdir -p %s" % os.path.join(".tmp",biPath))
                        os.system("mkdir -p %s" % os.path.join(self.destCls,biPath))
                        os.system("mkdir -p %s" % os.path.join(self.destJar,fw))
                        os.system("cp -a %s* %s" % (os.path.join(classes,biBaseName),os.path.join(".tmp",biPath)))
                        os.system("cp -a %s* %s" % (os.path.join(classes,biBaseName),os.path.join(destCls,biPath)))
                if not os.path.exists(os.path.join(destJar,fw,"policy.jar")):
                    print >> sys.stdout,"Framework Partial Release:create the %s/policy.jar ..." % fw
                    os.system("jar -cf %s -C .tmp ." % os.path.join(destJar,fw,"policy.jar"))
                else:
                    print >> sys.stdout,"Framework Partial Release:add class files into %s/policy.jar ..." % fw
                    os.system("jar -uf %s -C .tmp ." % os.path.join(destJar,fw,"policy.jar"))
                # release relative aidl files
                if not os.path.exists(os.path.join(self.src,base)):
                    print >> sys.stderr,"Error!Framework Partial Release '%s' does not exists!" % os.path.join(self.src,base)
                    sys.exit(7)
                if not os.path.isdir(os.path.join(self.src,base)):
                    print >> sys.stderr,"Error!Framework Partial Release the base '%s' must be a folder!" % os.path.join(self.src,base)
                    sys.exit(7)
                aidlFiles = map(lambda x:x.rstrip(),list(os.popen("find %s -name '*.aidl'" % os.path.join(self.src,base))))
                for aidl in aidlFiles:
                    vendorAidl = os.path.join(self.destCls,aidl[len(os.path.join(self.src,base))+1:])
                    if not os.path.exists(os.path.dirname(vendorAidl)):
                        os.makedirs(os.path.dirname(vendorAidl))
                    print >> sys.stdout,"Framework Partial Release:release %s ..." % aidl
                    os.system("rsync -a %s %s" % (aidl,vendorAidl))
                if os.path.exists(".tmp"):
                    shutil.rmtree(".tmp") 
                # remove the generated classes folder
                if os.path.exists(classes):
                    shutil.rmtree(classes)

    def release(self):
        self.binaryRelease()
        self.partialRelease()

# end FrameworkRelease

class AppRelease(object):
    def __init__(self):
        """ app release initialization """
        self.sources = dom.getAppSourceList()
        self.binarys = dom.getAppBinaryList()
        self.src = ARGUMENTS.releaseSrc

        self.appSrcSystem = appSrcSystem
        self.appDestVendor = appDestVendor

        self.frameworkFolder = frameworkFolder
        self.frameworkIntermediate = frameworkIntermediate
        self.frameworkHostFolder = frameworkHostFolder
        self.frameworkVendor = frameworkVendor
        self.frameworkVendorIntermediate = frameworkVendorIntermediate
        self.frameworkHostVendor = frameworkHostVendor

        self.sharelibFolder = sharelibFolder
        self.sharelibHostFolder = sharelibHostFolder
        self.sharelibIntermediate = sharelibIntermediate
        self.sharelibHostIntermediate = sharelibHostIntermediate
        self.sharelibVendor = sharelibVendor
        self.sharelibHostVendor = sharelibHostVendor
        self.sharelibHostVendorIntermediate = sharelibHostVendorIntermediate
        self.sharelibVendorIntermediate = sharelibVendorIntermediate

        self.releasePathTable = pathModuleIDTable

    def binaryRelease(self):
        for ap in self.binarys:
            # remove the destination relative part
            destPart = os.path.join(self.src,ap)
            self.getBinary(ap)
            if os.path.exists(destPart):
                os.system("find %s -type f -name '*' -exec rm -f {} \;" % destPart)

    def getBinary(self,apFolder):
        apFolder = os.path.join(self.src,apFolder)
        androidMkList = map(lambda x:x.rstrip(),list(os.popen("find %s -name Android.mk" % apFolder)))
        androidMkPaths = map(lambda x:os.path.dirname(x),androidMkList)
        # release binaries through the file "path_module_maptable"
        for adrPath in androidMkPaths:
            # adrPath is an absolute path,so for using the LOCAL_PATH -> module_id mapping relationship,remove the prefix(self.src)
            adrPath = adrPath[len(self.src)+1:]
            if self.releasePathTable.has_key(adrPath):
                moduleIdList = self.releasePathTable.get(adrPath)
                for md in moduleIdList:
                    self.copyBinary(md)
                # -------------------------------------
                # save the moduleIdLIst into binModules
                binModules.extend(moduleIdList)
                # -------------------------------------

    def copyBinary(self,moduleId):
        moduleIdPattern = re.compile("MODULE\.(.*?)\.(.*?)\.(.*)")
        moduleMatch = moduleIdPattern.match(moduleId)
        if moduleMatch:
            moduleTarget = moduleMatch.group(1)
            moduleClass = moduleMatch.group(2)
            moduleName = moduleMatch.group(3)
            if moduleClass == "APPS":
                source = "%s/%s.apk" % (self.appSrcSystem,moduleName)
                destination = "%s/%s.apk" % (self.appDestVendor,moduleName)
                if not os.path.exists(source):
                    print >> sys.stderr,"Error!App Release Binary '%s' does not exists!" % source
                    sys.exit(5)
                dirPath = os.path.dirname(destination)
                if not os.path.exists(dirPath):
                    os.makedirs(dirPath)
                print >> sys.stdout,"App Binary Release:release %s ..." % source
                os.system("rsync -a %s %s" % (source,destination))
            elif moduleClass == "JAVA_LIBRARIES":
                if moduleTarget == "HOST":
                    source = os.path.join(self.frameworkHostFolder,"%s.jar" % moduleName)     
                    destination = os.path.join(self.frameworkHostVendor,"%s.jar" % moduleName)
                    if not os.path.exists(source):
                        print >> sys.stderr,"Error!APP Release Binary '%s' does not exists!" % source
                        sys.exit(7)
                    if not os.path.exists(destination):
                        dirPath = os.path.dirname(destination)
                        os.makedirs(dirPath)
                    print >> sys.stdout,"APP Binary Release:release %s ..." % source
                    os.system("rsync -a %s %s" % (source,destination))
                if moduleTarget == "TARGET":
                    sourceSysJar = os.path.join(self.frameworkFolder,"%s.jar" % moduleName)
                    destinationSysJar = os.path.join(self.frameworkVendor,"%s.jar" % moduleName)
                    sourceJavalibJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    destinationJavalibJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"javalib.jar")
                    sourceClassesJar = os.path.join(self.frameworkIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    destinationClassesJar = os.path.join(self.frameworkVendorIntermediate,"%s_intermediates" % moduleName,"classes.jar")
                    if os.path.exists(sourceSysJar):
                        self.copyJar(sourceSysJar,destinationSysJar)
                    if os.path.exists(sourceJavalibJar):
                        self.copyJar(sourceJavalibJar,destinationJavalibJar)
                    if os.path.exists(sourceClassesJar):
                        self.copyJar(sourceClassesJar,destinationClassesJar)
            elif moduleClass == "SHARED_LIBRARIES":
                # copy the share library to system folder for build system.img
                if moduleTarget  == "TARGET":
                    source = os.path.join(self.sharelibFolder,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibVendor,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"APP Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))
                    # copy the share library to TARGET_OUT_INTERMEDIATE_LIBRARIES for dependency
                    source = os.path.join(self.sharelibIntermediate,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibVendorIntermediate,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"APP Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))
                elif moduleTarget == "HOST":
                    source = os.path.join(self.sharelibHostFolder,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibHostVendor,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"APP Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))
                    source = os.path.join(self.sharelibHostIntermediate,"%s.so" % moduleName)
                    destination = os.path.join(self.sharelibHostVendorIntermediate,"%s.so" % moduleName)
                    if os.path.exists(source):
                        dirPath = os.path.dirname(destination)
                        if not os.path.exists(dirPath):
                            os.makedirs(dirPath)
                        print >> sys.stdout,"APP Binary Release:release %s ..." % source
                        os.system("rsync -a %s %s" % (source,destination))

    def copyJar(self,src,dest):
        dirPath = os.path.dirname(dest)
        if not os.path.exists(dirPath):
            os.makedirs(dirPath)
        print >> sys.stdout,"APP Binary Release:release %s ..." % src
        os.system("rsync -a %s %s" % (src,dest))

    def release(self):
        self.binaryRelease()

# end AppRelease

# this class is used for some misc release files/folders
# TODO:maybe we'll use hardcode here
class MiscRelease(object):
    def __init__(self):
        """ misc release initialization """
        self.src = ARGUMENTS.releaseSrc
        self.prj = ARGUMENTS.project
        self.unreleaseFolder = dom.getUnreleaseDirList()
        self.unreleaseFile = dom.getUnreleaseFileList()

    def release(self):
        self.releaseHeaderUnderOut()
        self.removeUnreleasePart()
        self.modifyKconfig()
 
    def releaseHeaderUnderOut(self):
        self.outProduct = outProduct
        self.releaseHeader(os.path.join(self.outProduct,"obj/include"))
        
    def releaseHeader(self,folder):
        srcDir = os.path.join(self.src,folder)
        srcHeader = list(os.popen("find %s -name *.h" % srcDir))
        for head in srcHeader:
            head = head.rstrip()
            destDir = os.path.join(self.src,vendor,os.path.dirname(head[len(self.src)+1:]))
            if not os.path.exists(destDir):
                os.makedirs(destDir)
            #print >> sys.stdout,"Misc Release:release %s ..." % head
            os.system("rsync -a %s %s" % (head,destDir))

    def removeUnreleasePart(self):
        for d in self.unreleaseFolder:
            print >> sys.stdout,"Misc Release:removing %s ..." % os.path.join(self.src,d)
            os.system("rm -rf %s" % os.path.join(self.src,d))
        for f in self.unreleaseFile:
            print >> sys.stdout,"Misc Release:removing %s ..." % os.path.join(self.src,f)
            os.system("rm -rf %s" % os.path.join(self.src,f))

    def modifyKconfig(self):
        kconfig = os.path.join(self.src,"mediatek/source/kernel/Kconfig")
        kconfigOutput = open(kconfig,"r")
        kconfigWrite = []
        flag = True
        for line in kconfigOutput.readlines():
            if line == "if ARCH_MT6516\n":
                flag = False
            elif line == "endif\n" and flag == False:
                flag = True
            elif flag:
                kconfigWrite.append(line)
        kconfigOutput.close()
        kconfigInput = open(kconfig,"w")
        kconfigInput.writelines(kconfigWrite)
        kconfigInput.close()


# end MiscRelease

# dump the dependency information for MP release
def dumpDep(binMods):
    print >> sys.stdout,"dump dependency information ..."
    binmds = binMods
    targetTxt = os.path.join(ARGUMENTS.releaseSrc,vendor,"target.txt")
    if os.path.exists(targetTxt):
        os.system("rm -rf %s" % targetTxt)
    os.system("mkdir -p %s" % os.path.dirname(targetTxt))
    os.system("touch %s" % targetTxt)
    targetPat = re.compile("LOCAL_BUILT_MODULE\s*=\s*(\S+)")
    dependPat = re.compile("LOCAL_DEP_BUILT_FILES\s*\+\s*=\s*(\S+)")
    for binmd in binmds:
        depFile = os.path.join(releasePath,"%s.dep" % binmd) 
        depFileOutput = open(depFile,"r")
        for line in depFileOutput.readlines():
            # handle
            targetMatch = targetPat.match(line)
            dependMatch = dependPat.match(line)
            if targetMatch:
                targetModule = targetMatch.group(1)
            if dependMatch:
                dependModule = dependMatch.group(1)
                os.system("echo %s:%s >> %s" % (targetModule,dependModule,targetTxt))
        depFileOutput.close()

###############################################
#            begin to release
###############################################

def mtkRelease():
    """ mediatek custom release """
    # initialzation
    kernel = KernelRelease()
    android = AndroidRelease()
    framework = FrameworkRelease()
    apps = AppRelease()
    misc = MiscRelease()
    # release steps
    kernel.release()
    android.release()
    apps.release()
    framework.release()
    misc.release()
    print >> sys.stdout,"custom release[done]!"

# end mtkRelease

### MTK RELEASE ENTRY ###
mtkRelease()
### MTK RELEASE ENTRY ###

# ---------------------------------------------#
#   give the -d and dump the dep information   #
# ---------------------------------------------#
if options.dump == True:
    dumpDep(binModules)
# ---------------------------------------------#
