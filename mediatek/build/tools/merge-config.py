#!/usr/bin/env python
'''Usages:
    %s option codebase project
    options:
    -m    merge the feature options 
    -r    refactor feature options
    -n    transform a flavor project to a base project
'''
import os, sys, ConfigParser, pprint

class MyConfigParser(ConfigParser.RawConfigParser):
    def optionxform(self, optionstr):
        return optionstr
            
def remove_trail(s):
    n = len(s)
    for i in range(n,0,-1):
        if s[i-1] > ' ':
            n = i; break
    return s[:n]

def parse_mk(f, m):
    global mGlobal
    f = open(f)
    while 1:
        l = f.readline()
        if l:
            if l[0] == '#': continue
            a = l.split('=')
            if len(a) == 2:
                k = a[0].replace(':', '')
                k = k.strip()
                v = remove_trail(a[1]).strip()
                if k[:15] == 'AUTO_ADD_GLOBAL':
                    mGlobal[k] = mGlobal.get(k, []) + v.split(' ')
                else: m[k] = v
        else: break
    f.close()

a = sys.argv
l = len(a)

if l < 4 or a[1][0] != '-':
    print __doc__ % a[0]
    sys.exit()

kOp = a.pop(1)[1]
    
# list hierachy
aConfig = a[2].split('[')
if len(aConfig) == 2: aConfig[1] = a[2]
aConfig.insert(0, 'common')

mGlobal = {}
m = {}
# determin which platform
pConfig = '%s/mediatek/config/%s/ProjectConfig.mk'
parse_mk(pConfig % (a[1], aConfig[1]), m)
#pprint.pprint(m)

strPlatform = m.get('MTK_PLATFORM', '')
if strPlatform:
    aConfig.insert(1, strPlatform.lower())
    print 'MTK_PLATFORM', strPlatform

# merge configures
m1 = {}
if kOp != 'm':
    if kOp == 'n' and len(aConfig) == 4: k = -2
    else: k = -1

    for i in aConfig[:k]:
        parse_mk(pConfig % (a[1], i), m1)
    aConfig = aConfig[k:]

for i in aConfig:
    parse_mk(pConfig % (a[1], i), m)

for k, v in mGlobal.items():
    mGlobal[k] = list( set(v) )

if m1:
    m = m.items()
    m1 = m1.items()
    m = list( set(m) - set(m1) )
else:
    for k, v in mGlobal.items():
        m[k] = ' '.join(v)
    m = m.items()

fFeature = a[1] + '/mediatek/config/feature_option_info.ini'
mFeature = {}
if os.path.isfile(fFeature):
    ini = MyConfigParser()
    ini.read(fFeature)
    for k in ini.sections():
        mFeature[k] = [ini.get(k, 'class'), ini.get(k, 'description')]

def sort_feature(a,b):
    r = cmp(a[0],b[0])
    if r == 0: r = cmp(a[2],b[2])
    return r

m = [ mFeature.get(i[0], ['','']) + list(i) for i in m]
m.sort(sort_feature)

# output a configure file
if kOp == 'n': strProject = ''
else: strProject = aConfig[-1]

f = open(pConfig % (a[1], strProject), 'w')
for v in m:
    if v[2] not in ['MTK_BUILD_VERNO']:
        if v[1]:
            f.write('\n')
            for i in v[1].split(';'):
                f.write('# ' + i + '\n')
        f.write( '%s=%s\n' % (v[2],v[3]) )
f.close()

f = pConfig % (a[1], 'common')
d = open(f).read()

for k, v in mGlobal.items():
    i = d.find(k)
    j = d.find('\n', i)
    d = d[:i] + k + '=' + ' '.join(v) + d[j:]

f = open(f, 'w')
f.write(d)
f.close()