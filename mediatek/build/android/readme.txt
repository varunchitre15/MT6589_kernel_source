# kirby.wu@mediatek.inc (2009/10/22)

Procedure:

 1. if mediatek makefile extension is not patched into ABS :
    copy mediatek/build/android/patch/* to build/core
 2. build with
      TARGET_PRODUCT=yusu RELEASE_POLICY=other make
    then execute 
      python mediatek/build/android/tools/class.py
    (modify it if any policy related to framework is changed!)
    objects should be generated in vendor/mediate/yusu with
      cls, jar, copy.txt, target.txt, out
    you have to manually update objs, written in tools/moveobj.sh / objects.sh
    
