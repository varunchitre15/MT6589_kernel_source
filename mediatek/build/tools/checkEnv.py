#! /usr/bin/python

from optparse import OptionParser
import sys
import os
import re
import commands

parser = OptionParser(usage="usage: %prog [options]",version="%prog 1.3")
parser.add_option("-a","--all",action="store_true",help="check all the build environment")
parser.add_option("-o","--os",action="store_true",help="check requirement for operating system")
parser.add_option("-M","--memory",action="store_true",help="check requirement for momery size")
parser.add_option("-m","--make",action="store_true",help="check requirement for GNU make")
parser.add_option("-P","--perl",action="store_true",help="check requirement for perl interpreter")
parser.add_option("-p","--python",action="store_true",help="check requirement for python interpreter")
parser.add_option("-e","--eabi",action="store_true",help="check requirement for arm-eabi compilation tool-chain")
parser.add_option("-g","--gcc",action="store_true",help="check requirement of gcc compilation tool-chain")
parser.add_option("-j","--jdk",action="store_true",help="check requirement for jdk")
parser.add_option("-t","--toolchain",action="store_true",help="check requirement for all compilation toolchains")
parser.add_option("-b","--bison",action="store_true",help="check requirement for bison")
parser.add_option("-f","--flex",action="store_true",help="check requirement for flex")
parser.add_option("-G","--gperf",action="store_true",help="check requirement for gperf")
parser.add_option("-w","--mingw",action="store_true",help="check requirement for mingw")
parser.add_option("-u","--unix2dos",action="store_true",help="check requirement for unix2dos/tofrodos command")

(options,args) = parser.parse_args()
if len(args) != 0:
    parser.print_help()
    sys.exit(1)

optionsList = []

if options.all == True:
    options.os = True
    options.make = True
    options.memory = True
    options.perl = True
    options.python = True
    options.eabi = True
    options.gcc = True
    options.jdk = True
    options.toolchain = True
    options.bison = True
    options.flex = True
    options.gperf = True
    options.mingw = True
    options.unix2dos = True

if options.toolchain == True:
    options.eabi = True
    options.gcc = True
    options.jdk = True

# use the List to save the items that we want to check
optionsList.append({"os":options.os})
optionsList.append({"memory":options.memory})
optionsList.append({"make":options.make})
optionsList.append({"perl":options.perl})
optionsList.append({"python":options.python})
optionsList.append({"eabi":options.eabi})
optionsList.append({"gcc":options.gcc})
optionsList.append({"jdk":options.jdk})
optionsList.append({"bison":options.bison})
optionsList.append({"flex":options.flex})
optionsList.append({"gperf":options.gperf})
optionsList.append({"mingw":options.mingw})
optionsList.append({"unix2dos":options.unix2dos})

def result(category,version,bit,flag,info):
    if bit != "" and version !="":
        if info != "":
            resultInfo = "[%s]: %s (%s-bit) [%s]\n%s\n" % (category,version,bit,flag,info)
        else:
            resultInfo = "[%s]: %s (%s-bit) [%s]" % (category,version,bit,flag)
        print >> sys.stdout,resultInfo
    elif version != "":
        if info != "":
            resultInfo = "[%s]: %s [%s]\n%s\n" % (category,version,flag,info)
        else:
            resultInfo = "[%s]: %s [%s]" % (category,version,flag)
        print >> sys.stdout,resultInfo
    elif bit != "":
        if info != "":
            resultInfo = "[%s]: (%s-bit) [%s]\n%s\n" % (category,bit,flag,info)
        else:
            resultInfo = "[%s]: (%s-bit) [%s]" % (category,bit,flag)
        print >> sys.stdout,resultInfo
    else:
        if info != "":
            resultInfo = ("[%s]: [%s]\n" + "%s\n") % (category,flag,info)
        else:
            resultInfo = "[%s]: [%s]" % (category,flag)
        print >> sys.stdout,resultInfo

# end result

global checkResult
checkResult  = 0

class OsCheck(object):
    """ check the requirement for os """
    def __init__(self):
        self.platform = sys.platform
        self.flag = "FAIL" 
        self.tag = True
        self.info = ""

    def checkEnv(self):
        self.checkPlatform()
        if self.tag:
            self.checkOsBit()
   
    def checkPlatform(self):
        global checkResult
        if self.platform == "linux2":
            self.checkLinuxVersion()
        elif self.platform == "win32":
            self.info = "android build system don't support Windows os"
            result("OS","","unknown",self.flag,self.info)
            self.tag = False
            checkResult = 2
        elif self.platform == "darwin":
            self.info = "android build system don't support Mac os"
            result("OS","","unknown",self.flag,self.info)
            self.tag = False
            checkResult = 2

    def checkLinuxVersion(self):
        linuxVersion = commands.getoutput("lsb_release -d")
        pattern = re.compile("Description\s*:\s*(([\w\s]*?)([\d\.]+))")
        match = pattern.match(linuxVersion)
        if match:
            self.osVersion = match.group(1)
            distribution = match.group(2).strip()
            versionNo = match.group(3)
            if distribution.lower() == "ubuntu":
                number = versionNo.split(".")
                if int(number[0]) < 9 or (int(number[0]) == 9 and int(number[1]) == 4):
                    self.info = "your ubuntu os version is lower than recommendation"
                    self.tag = False
                    global checkResult
                    checkResult = 1
                elif int(number[0]) == 9 and int(number[1]) == 10:
                    self.flag = "OK"
                elif int(number[0]) == 10 and int(number[1]) == 4:
                    self.flag = "OK"
                else:
                    self.flag = "WARNING"
                    self.info = "your ubuntu os version is higher than recommendation"
            else:
                self.flag = "WARNING"
                self.info = "your Linux distribution is not Ubuntu which we recommendation"
   
    def checkOsBit(self):
        arch = commands.getoutput("uname -m")
        pattern = re.compile(".*?_(\d+)")
        match = pattern.match(arch)
        if match:
            self.osBit = int(match.group(1))
        else:
            self.osBit = 32
        if self.osBit != 64:
            self.flag = "FAIL"
            self.info = "android 2.3 only support 64 bits OS"
        result("OS",self.osVersion,self.osBit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1
 
# end OsCheck

class MemoryCheck(object):
    """check the requirement for physical memory size """
    def __init__(self):
        self.flag = "FAIL"
        self.info = ""

    def checkEnv(self):
        self.checkMemorySize()

    def checkMemorySize(self):
        freeMem = commands.getoutput("free -k")
        pattern = re.compile(".*Mem:\s*(\d+)",re.S)
        match = pattern.match(freeMem)
        if match:
            self.memSize = match.group(1)
            lowMem = 2.0 * 1024 * 1024
            highMem = 4.0 * 1024 * 1024
            if float(self.memSize) < lowMem:
                self.info = "it's too smaller than we recommendation"
                global checkResult
                checkResult = 1
            elif float(self.memSize) > lowMem and float(self.memSize) < highMem:
                self.flag = "WARNING"
                self.info = ("it's smaller than recommendation, "
                            + "may cause out-of-memory build error")
            else:
                self.flag = "OK"
        else:
            self.memSize = "unknown"

        if self.info != "":
            print >> sys.stdout,("[Physical Memory Size] : " 
                                + "%s K-Bytes [%s]\n%s\n") % (self.memSize,self.flag,self.info)
        else:
            print >> sys.stdout,("[Physical Memory Size] : " 
                                + "%s K-Bytes [%s]") % (self.memSize,self.flag)

#end MemoryCheck

class PerlCheck(object):
    """ check the requirement for perl interpreter """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkPerlVersion()

    def checkInstall(self):
        returnCode,self.perl = commands.getstatusoutput("which perl")
        if returnCode != 0:
            self.info = "you have not installed perl"
            self.tag = False
            result("perl","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkPerlVersion(self):
        perlVersion = commands.getoutput("%s -v" % self.perl)
        pattern = re.compile(".*?v([\d\.]+)\s*",re.S)
        match = pattern.match(perlVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 5 or (int(number[0]) == 5 and int(number[1]) < 10):
                self.info = "your perl version is lower than recommendation"
            elif int(number[0]) == 5 and int(number[1]) == 10:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your perl version is higher than recommendation"
        else: versionNo = "unknown"
        perlBit = commands.getoutput("file -bL %s" % self.perl)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(perlBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("perl",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end PerlCheck

class PythonCheck(object):
    """ check the requirement for python interpreter """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
  
    def checkEnv(self):
        # because we use python for environment checking, so we only check python's version
        self.checkPythonVersion()

    def checkPythonVersion(self):
        self.python = commands.getoutput("which python")
        pythonVersion = commands.getoutput("%s -V" % self.python)
        pattern = re.compile("Python\s*([\d\.]+)")
        match = pattern.match(pythonVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 1 or (int(number[0]) ==2 and int(number[1] < 6)):
               self.info = "your python version is lower than recommendation" 
            elif int(number[0]) == 2 and int(number[1]) == 6:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your python version is higher than recommendation"
        else: versionNo = "unknown"
        pythonBit = commands.getoutput("file -bL %s" % self.python)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(pythonBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("python",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1 

# end PythonCheck

class MakeCheck(object):
    """ check the requirement for GNU make """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkMakeVersion()

    def checkInstall(self):
        returnCode,self.make = commands.getstatusoutput("which make")
        if returnCode != 0:
            self.info = "you have not installed make"
            self.tag = False
            result("make","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkMakeVersion(self):
        makeVersion = commands.getoutput("%s -v" % self.make)
        pattern = re.compile("GNU\s*Make\s*([\d\.]+)\s*",re.S)
        match = pattern.match(makeVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 3 or (int(number[0]) == 3 and int(number[1]) < 81):
                self.info = "your make version is lower than recommendation"
            elif int(number[0]) == 3 and (int(number[1]) == 81 or int(number[1]) == 82):
               self.flag = "OK"
            else:
               self.info = "Android can only be built by versions 3.81 and 3.82."
        else: self.versionNo = "unknown"
        makeBit = commands.getoutput("file -bL %s" % self.make)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(makeBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("make",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end MakeCheck

class JavaCheck(object):
    """ check the requirement for java compiler/launcher """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkJavaVersion()

    def checkInstall(self):
        returnCode1,self.javac = commands.getstatusoutput("which javac")
        returnCode2,self.java = commands.getstatusoutput("which java")
        if (returnCode1 or returnCode2) != 0:
            self.info = "you have not installed jdk"
            self.tag = False
            result("jdk","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkJavaVersion(self):
        javaVersion = commands.getoutput("%s -version" % self.java)
        pattern = re.compile("java\s*version\s*\"([\d\._]+)",re.S)
        match = pattern.match(javaVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 1 or (int(number[0]) == 1 and int(number[1]) < 6):
                self.info = "your jdk version is lower than recommendation"
            elif int(number[0]) == 1 and int(number[1]) == 6:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your jdk version is higher than recommendation"
        else: self.versionNo = "unknown"
        pattern = re.compile("openjdk")
        match = pattern.match(javaVersion)
        if match:
            self.info = "openjdk is not supported"
            self.flag = "FAIL"
        jdkBit = commands.getoutput("file -bL %s" % self.java)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(jdkBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("jdk",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end JavaCheck

class GccCheck(object):
    """ check the requirement for gcc compiler """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkGccVersion()

    def checkInstall(self):
        returnCode,self.gcc = commands.getstatusoutput("which gcc")
        if returnCode != 0:
            self.info = "you have not installed gcc"
            self.tag = False
            result("gcc","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkGccVersion(self):
        gccVersion = commands.getoutput("%s --v" % self.gcc)
        pattern = re.compile(".*gcc\s*version\s*([\d\.]+)",re.S)
        match = pattern.match(gccVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 4 \
               or (int(number[0]) == 4 and int(number[1]) < 4) \
               or (int(number[0]) == 4 and int(number[1]) == 4 and int(number[2]) < 3):
                self.info = "your gcc version is lower than recommendation"
            elif int(number[0]) == 4 and int(number[1]) == 4 and int(number[2]) == 3:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your gcc version is higher than recommendation"
        else: self.versionNo = "unknown"
        gccBit = commands.getoutput("file -bL %s" % self.gcc)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(gccBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("gcc",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end GccCheck

class EabiCheck(object):
    """ check the requirement for arm-linux-androideabi-gcc compiler """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkEabiVersion()

    def checkInstall(self):
        returnCode,self.eabigcc = commands.getstatusoutput("which arm-linux-androideabi-gcc")
        if returnCode != 0:
            self.info = "you have not installed arm-linux-androideabi-gcc"
            self.tag = False
            result("arm-linux-androideabi-gcc","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkEabiVersion(self):
        eabiVersion = commands.getoutput("%s --version" % self.eabigcc)
        pattern = re.compile("arm-linux-androideabi-gcc.*?([\d\.]+)",re.S)
        match = pattern.match(eabiVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 4 \
               or (int(number[0]) == 4 and int(number[1]) != 6): 
                self.info = "your arm-linux-androideabi-gcc version is not recommendation"
            elif int(number[0]) == 4 and int(number[1]) == 6:
               self.flag = "OK"
        else:
            self.versionNo = "unknown version"
            self.info = "eabigcc: %s \n version info: %s \n" % (self.eabigcc,eabiVersion)
        eabiBit = commands.getoutput("file -bL %s" % self.eabigcc)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(eabiBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
            self.info += "Bit info:%s " % eabiBit
        result("arm-linux-androideabi-gcc",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end EabiCheck

class BisonCheck(object):
    """ check the requirement for bison """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkBisonVersion()

    def checkInstall(self):
        returnCode,self.bison = commands.getstatusoutput("which bison")
        if returnCode != 0:
            self.info = "you have not installed bison"
            self.tag = False
            result("bison","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkBisonVersion(self):
        bisonVersion = commands.getoutput("%s --version" % self.bison)
        pattern = re.compile("bison.*?([\d\.]+)",re.S)
        match = pattern.match(bisonVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 2 or (int(number[0]) == 2 and int(number[1]) < 4):
                self.info = "your bison version is lower than recommendation"
            elif int(number[0]) == 2 and int(number[1]) == 4:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your bison version is higher than recommendation"
        else: self.versionNo = "unknown"
        bisonBit = commands.getoutput("file -bL %s" % self.bison)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(bisonBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("bison",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end BisonCheck

class FlexCheck(object):
    """ check the requirement for flex """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkFlexVersion()

    def checkInstall(self):
        returnCode,self.flex = commands.getstatusoutput("which flex")
        if returnCode != 0:
            self.info = "you have not installed flex"
            self.tag = False
            result("flex","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkFlexVersion(self):
        flexVersion = commands.getoutput("%s --version" % self.flex)
        pattern = re.compile("flex\s*([\d\.]+)",re.S)
        match = pattern.match(flexVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 2 or (int(number[0]) == 2 and int(number[1]) < 5):
                self.info = "your flex version is lower than recommendation"
            elif int(number[0]) == 2 and int(number[1]) == 5:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your flex version is higher than recommendation"
        else: self.versionNo = "unknown"
        flexBit = commands.getoutput("file -bL %s" % self.flex)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(flexBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("flex",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end FlexCheck

class GperfCheck(object):
    """ check the requirement for gperf """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()
        if self.tag:
            self.checkGperfVersion()

    def checkInstall(self):
        returnCode,self.gperf = commands.getstatusoutput("which gperf")
        if returnCode != 0:
            self.info = "you have not installed gperf"
            self.tag = False
            result("gperf","","",self.flag,self.info)
            global checkResult
            checkResult = 1

    def checkGperfVersion(self):
        gperfVersion = commands.getoutput("%s --version" % self.gperf)
        pattern = re.compile("GNU\s*gperf\s*([\d\.]+)",re.S)
        match = pattern.match(gperfVersion)
        if match:
            self.versionNo = match.group(1)
            number = self.versionNo.split(".")
            if int(number[0]) < 3:
                self.info = "your gperf version is lower than recommendation"
            elif int(number[0]) == 3 and int(number[1]) == 0:
               self.flag = "OK"
            else:
               self.flag = "WARNING"
               self.info = "your gperf version is higher than recommendation"
        else: self.versionNo = "unknown"
        gperfBit = commands.getoutput("file -bL %s" % self.gperf)
        pattern = re.compile("ELF\s*(\d+)-bit\s*LSB\s*executable.*")
        match = pattern.match(gperfBit)
        if match:
            self.bit = match.group(1)
        else:
            self.bit = "unknown"
        result("gperf",self.versionNo,self.bit,self.flag,self.info)
        if self.flag == "FAIL":
            global checkResult
            checkResult = 1

# end GperfCheck

class MingwCheck(object):
    """ check the requirement for mingw """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()

    def checkInstall(self):
        returnCode,self.mingw = commands.getstatusoutput("which i586-mingw32msvc-gcc")
        if returnCode != 0:
            self.info = "you have not installed mingw32(i586-mingw32msvc-gcc is NOT in your path)"
            self.tag = False
            result("mingw","","",self.flag,self.info)
            global checkResult
            checkResult = 1
        else:
            self.flag = "OK"
            result("mingw","Installed","",self.flag,self.info)

# end MingwCheck

class Unix2DosCheck(object):
    """ check the requirement for unix2dos/tofrodos command """
    def __init__(self):
        self.flag = "FAIL"
        self.tag = True
        self.info = ""
    
    def checkEnv(self):
        self.checkInstall()

    def checkInstall(self):
        returnCode1,self.todos = commands.getstatusoutput("which todos")
        returnCode2,self.unix2dos = commands.getstatusoutput("which unix2dos")
        if returnCode1 and returnCode2 != 0:
            self.info = "you have not installed unix2dos/tofrodos command(unix2dos/todos is NOT in your path)"
            self.tag = False
            result("unix2dos/tofrodos","","",self.flag,self.info)
            global checkResult
            checkResult = 1
        else:
            self.flag = "OK"
            result("unix2dos/tofrodos","Installed","",self.flag,self.info)

# end MingwCheck

def suggest():
    suggestInfo = """
Build Environment Requirement
=============================================================
* ********* Suggested OS and Tool Chain to install *********
*
*       OS                         : Linux distribution Ubuntu 10.04
*       Memory Size                : 4G or above
*       make                       : GNU Make 3.81 or 3.82
*       perl                       : Version 5.10.X
*       python                     : Version 2.6.X
*       arm-linux-androideabi-gcc  : Version 4.6.X
*       gcc                        : Version 4.4.3
*       jdk                        : Version 1.6.X
*       bison                      : Version 2.4.X
*       flex                       : Version 2.5.X
*       gperf                      : Version 3.0.X
*       mingw                      : Installed
*       unix2dos/tofrodos          : Installed
* **********************************************************
=============================================================

"""
    print >> sys.stdout,suggestInfo

# end suggest

suggest()
print >> sys.stdout,"Build Environment Check Result Report"
print >> sys.stdout,"*************************************************************\n"
for item in optionsList:
    if item.has_key("os") and item.get("os") == True:
        o = OsCheck()
        o.checkEnv()
        if checkResult == 2:
           break 
    elif item.has_key("perl") and item.get("perl") == True:
        p = PerlCheck()
        p.checkEnv()
    elif item.has_key("python") and item.get("python") == True:
        p = PythonCheck()
        p.checkEnv()
    elif item.has_key("make") and item.get("make") == True:
        m = MakeCheck()
        m.checkEnv()
    elif item.has_key("jdk") and item.get("jdk") == True:
        j = JavaCheck()
        j.checkEnv()
    elif item.has_key("gcc") and item.get("gcc") == True:
        g = GccCheck()
        g.checkEnv()
    elif item.has_key("eabi") and item.get("eabi") == True:
        e = EabiCheck()
        e.checkEnv()
    elif item.has_key("bison") and item.get("bison") == True:
        b = BisonCheck()
        b.checkEnv()
    elif item.has_key("flex") and item.get("flex") == True:
        f = FlexCheck()
        f.checkEnv()
    elif item.has_key("gperf") and item.get("gperf") == True:
        g = GperfCheck()
        g.checkEnv()
    elif item.has_key("mingw") and item.get("mingw") == True:
        m = MingwCheck()
        m.checkEnv()
    elif item.has_key("unix2dos") and item.get("unix2dos") == True:
        m = Unix2DosCheck()
        m.checkEnv()
    elif item.has_key("memory") and item.get("memory") == True:
        m = MemoryCheck()
        m.checkEnv()

print >> sys.stdout,"*************************************************************\n"
sys.exit(checkResult)
