#! /usr/bin/python

import xml.dom.minidom as xdom
from optparse import OptionParser

# option parse
parser = OptionParser(usage="usage: %prog [options]")
parser.add_option("-l","--list",dest="autoGenList",metavar="FEATURELIST",help="get the user feature list")
(options,args) = parser.parse_args()
if len(args) != 0:
    parser.print_help()

try:
    autoList =  options.autoGenList.split()
except AttributeError:
    autoList = []

# through the logical we can set the we needed autogen xml list
# TODO:we use the autoList to save the xml list
# eg, if contidion:
#         autoList.append(xml)
#         ...

####################################################
#        uses-feature xml mapping table                 
####################################################

xmlHash = {};
xmlHash["android.hardware.bluetooth"] = ["android.hardware.bluetooth"]
xmlHash["android.hardware.camera"] = ["android.hardware.camera","android.hardware.camera.autofocus"]
xmlHash["android.hardware.camera.autofocus"] = ["android.hardware.camera","android.hardware.camera.autofocus"]
xmlHash["android.hardware.location"] = ["android.hardware.location","android.hardware.location.network"]
xmlHash["android.hardware.location.gps"] = ["android.hardware.location","android.hardware.location.network","android.hardware.location.gps"]
#xmlHash["android.hardware.location.network"] = []
xmlHash["android.hardware.microphone"] = ["android.hardware.microphone"]
xmlHash["android.hardware.sensor.compass"] = ["android.hardware.sensor.compass"]
xmlHash["android.hardware.sensor.accelerometer"] = ["android.hardware.sensor.accelerometer"]
xmlHash["android.hardware.sensor.light"] = ["android.hardware.sensor.light"]
xmlHash["android.hardware.sensor.gyroscope"] = ["android.hardware.sensor.gyroscope"]
xmlHash["android.hardware.sensor.barometer"] = ["android.hardware.sensor.barometer"]
xmlHash["android.hardware.sensor.proximity"] = ["android.hardware.sensor.proximity"]
#xmlHash["android.hardware.telephony"]
xmlHash["android.hardware.telephony.cdma"] = ["android.hardware.telephony","android.hardware.telephony.cdma"]
xmlHash["android.hardware.telephony.gsm"] = ["android.hardware.telephony","android.hardware.telephony.gsm"]
xmlHash["android.hardware.touchscreen"] = ["android.hardware.touchscreen"]
xmlHash["android.hardware.touchscreen.multitouch"] = ["android.hardware.touchscreen","android.hardware.touchscreen.multitouch"]
xmlHash["android.hardware.touchscreen.multitouch.distinct"] = ["android.hardware.touchscreen","android.hardware.touchscreen.multitouch","android.hardware.touchscreen.multitouch.distinct"]
xmlHash["android.hardware.touchscreen.multitouch.jazzhand"] = ["android.hardware.touchscreen","android.hardware.touchscreen.multitouch","android.hardware.touchscreen.multitouch.distinct","android.hardware.touchscreen.multitouch.jazzhand"]
xmlHash["android.software.live_wallpaper"] = ["android.software.live_wallpaper"]
xmlHash["android.hardware.wifi"] = ["android.hardware.wifi"]
xmlHash["android.software.sip"] = ["android.software.sip"]
xmlHash["android.software.sip.voip"] = ["android.software.sip","android.software.sip.voip"]

####################################################

def autoGen(xmlName,xmlContent):
    # create document
    doc = xdom.Document()
    # create permissions node
    permissions = doc.createElement("permissions")
    doc.appendChild(permissions)
    # create feature node
    for content in xmlContent:
        feature = doc.createElement("feature")
        feature.setAttribute("name",content)
        permissions.appendChild(feature)
    xmlfile = open("%s.xml"%xmlName,"w")
    xmlfile.write(doc.toprettyxml(indent = "    ",encoding="utf-8"))
    xmlfile.close()

for name in autoList:
    autoGen(name,xmlHash[name])


