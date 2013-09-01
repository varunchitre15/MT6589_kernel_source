#!/usr/bin/python
import sys,os
import re
import getopt
import subprocess
from subprocess import *
from optparse import OptionParser
from os.path import join, getsize

def build_search_list(folder):
    list = []
    for src in os.listdir(folder):
        srcpath = os.path.join(folder,src)
        for root, dirs, files in os.walk(srcpath):
            for file in files:
                if re.search('png',file):
                    list.append(file)
    temp = set(list)
    list_temp=[]
    for i in temp:
        list_temp.append(i)
    return list_temp

def update_pattern(list):
    pattern_line=''
    for li in list:
        if re.search('\.9\.png',li):
            if len(pattern_line)>0:
                pattern_line = pattern_line + '|' + re.split('\.9\.png',li)[0]
            else:
                pattern_line =  re.split('\.9\.png',li)[0]
        elif re.search('\.png',li):
            if len(pattern_line)>0:
                pattern_line = pattern_line + '|' + re.split('\.png',li)[0]
            else:
                pattern_line = re.split('\.png',li)[0]
        elif re.search('\.jpg',li):
            if len(pattern_line)>0:
                pattern_line = pattern_line + '|' + re.split('\.jpg',li)[0]
            else:
                pattern_line = re.split('\.jpg',li)[0]
    pattern = re.compile(pattern_line)
    return pattern

def remove_pattern(match,list):
    for li in list:
        if re.search(match,li):
            list.remove(li)

def file_check(list,file):
    f_input = open(file)
    pattern = update_pattern(list)
    for line in f_input:
        if pattern.search(line):
            #match = pattern.search(line).group(0)
            m = pattern.findall(line)
            for n in range(0,len(m)):
            	remove_pattern(m[n],list)
    f_input.close()

def check_exist(list, folder):
    for root, dirs, files in os.walk(folder):
        for file in files:
            if re.search('\.xml|\.java',file):
                file_check(list,join(root,file))

def check_local_module_package(file):
    f_input=open(file)
    for line in f_input:
        if re.search('LOCAL_PACKAGE_NAME',line):
            f_input.close()
            return True
    f_input.close()
    return False

def find_dummy_res(folder):
    list = build_search_list(folder)
    if len(list)>0:
        package_name = find_process_folder_packagename(folder)
        total = len(list)
        check_exist(list,folder)
        if len(list) > 0:
            print '[Check Res][Unused Res] PackageName ' + ' FileName' + ' Size '
            dummy_file_size = 0
            for li in list:
                cmd = 'find %s -name %s'%(folder,li)
                find_out = subprocess.Popen(cmd,stdout=PIPE,shell=True).communicate()[0]
                find_out = re.split('\\n',find_out);
                size = 0
                for file in find_out:
                    if re.search(folder,file):
                        size_temp = os.path.getsize(file)
                        size = size + size_temp  
#                        print os.getcwd()
#                        print '[check res][Unused res] ' + package_name + ' ' + folder + li + ' ' +str(size)
                        print '[Check Res][Unused Res] ' \
                            + package_name \
                            + ' ' +file.split(os.getcwd(),1)[0]+ ' ' \
                            + str(size) + ('bytes' if str(size)>1 else 'byte')
                        dummy_file_size = dummy_file_size + size
            print '[Check Res][Unused Res Sum] ' \
                + package_name \
                + ' %d'%(len(list)) + ('Files' if len(list)>1 else 'File') + '(%d'%(len(list)*100/total if len(list)*100/total>0 else 1)+'%) ' \
                + str(dummy_file_size) + ('bytes' if str(dummy_file_size)>1 else 'byte')
    
def find_process_folder(dir):
    folder_list=[]
    for root, dirs, files in os.walk(dir):
        for file in files:
            if re.search('Android\.mk',file):
                if check_local_module_package(join(root,file)):
                    folder_list.append(root)
    return folder_list
    
def main():
    parser = OptionParser()
    parser.add_option("-d","--dir",action = "store",type = "string",dest = "dir")
    
    (options, args) = parser.parse_args()
    if options.dir:
        folder_list = find_process_folder(options.dir)
        for folder in folder_list:
            #print 'Process folder : ' + li
            find_dummy_res(folder)
    else:
        print "the process directory not assign !!!"

def find_process_folder_packagename(dir):
    folder_list=[]
    for root, dirs, files in os.walk(dir):
        for file in files:
            if re.search('Android\.mk',file):
                return check_local_module_package_packagename(join(root,file))
  #  return folder_list

def check_local_module_package_packagename(file):
    f_input=open(file)
    for line in f_input:
        if re.search('LOCAL_PACKAGE_NAME',line):
            f_input.close()
            return line[22:].rstrip('\n')
    f_input.close()



if __name__ == "__main__":
    main()
