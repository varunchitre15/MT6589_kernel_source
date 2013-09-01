#!/bin/bash
# ##########################################################
# ALPS(Android4.1 based) build environment profile setting
# ##########################################################
# Overwrite JAVA_HOME environment variable setting if already exists
JAVA_HOME=/mtkoss/jdk/jdk1.6.0_23
export JAVA_HOME

# Overwrite ANDROID_JAVA_HOME environment variable setting if already exists
ANDROID_JAVA_HOME=/mtkoss/jdk/jdk1.6.0_23
export ANDROID_JAVA_HOME

# Overwrite PATH environment setting for JDK & arm-eabi if already exists
PATH=/mtkoss/jdk/jdk1.6.0_23/bin:$PWD/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin:$PATH
export PATH

# Add MediaTek developed Python libraries path into PYTHONPATH
if [ -z "$PYTHONPATH" ]; then
  PYTHONPATH=$PWD/mediatek/build/tools
else
  PYTHONPATH=$PWD/mediatek/build/tools:$PYTHONPATH
fi
export PYTHONPATH

#<//2013/04/03-23518-tonypeng, Follow SoMC CM team request to change mode of file makeMtk.ini.
chmod 777 makeMtk.ini
#>//2013/04/03-23518-tonypeng

