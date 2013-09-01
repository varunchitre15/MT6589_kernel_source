

PRODUCT_OUT=$1
TARGET_PRODUCT=$2

if [[ ${PRODUCT_OUT} == "" ]]; then 
  echo "usage: make-platform.sh <PRODUCT_OUT> <TARGET_PRODUCT>"
  exit -1;
fi

if [[ ! -e ${PRODUCT_OUT}/root || ! -e ${PRODUCT_OUT}/system ]]; then
  echo "$TARGET_PRODUCT platform is not built"
  exit -1
fi

PACK_PLATFORM=n
PACK_RAMDISK=n

if [ "${TARGET_PRODUCT}" == "mt6575_fpga" ]; then 
	PACK_RAMDISK=y
	PACK_PLATFORM=y
fi

if [ ${PACK_PLATFORM} == "y" ]; then 

	PACK_OUT=${PRODUCT_OUT}/.pack
	ARCHIVE_NAME=platform.tgz
	FULL_ARCHIVE_NAME=${PRODUCT_OUT}/${ARCHIVE_NAME}
	
	SYSTEM_FOLDER=${PRODUCT_OUT}/system

	echo -n "Packing Android platform \"${FULL_ARCHIVE_NAME}\"..."
	if [[ -e ${PACK_OUT} ]]; then
	  rm -rf ${PACK_OUT};
	fi
	if [[ ! -e ${PACK_OUT} ]]; then
	  mkdir ${PACK_OUT};
	fi
	if [[ ! -e ${PACK_OUT} || ! -d ${PACK_OUT} ]]; then
	  echo "can't create dir .pack. please check file permission.";
	  exit -1;
	fi
	cp -R ${SYSTEM_FOLDER}/* ${PACK_OUT}
	cp -R ${PRODUCT_OUT}/data ${PACK_OUT}/data
	cd ${PACK_OUT};
	  chmod -R a+x bin/*;
	  chmod -R a+x xbin/*;
	  tar -zcf ../${ARCHIVE_NAME} *;
	cd - > /dev/null
	echo "Done"

fi

if [ ${PACK_RAMDISK} == "y" ]; then 
	echo "Creating ramdisk.gz..."

	RAMDISK=ramdisk
	RAMDISK_GZ=ramdisk.gz
	rm -f ${PRODUCT_OUT}/$RAMDISK
	rm -f ${PRODUCT_OUT}/$RAMDISK.gz

	cd ${PRODUCT_OUT}/root
	rm -rf data
	find . -print | cpio -H newc -o > ../${RAMDISK}
	gzip -9 ../$RAMDISK
	echo Created ${PRODUCT_OUT}/$RAMDISK.gz
	cd - > /dev/null

	echo "Creating factory ramdisk.gz..."

	RAMDISK=ramdisk-factory
	RAMDISK_GZ=ramdisk-factory.gz
	rm -f ${PRODUCT_OUT}/$RAMDISK
	rm -f ${PRODUCT_OUT}/$RAMDISK.gz

	cd ${PRODUCT_OUT}/factory/root
	rm -rf data
	find . -print | cpio -H newc -o > ../../${RAMDISK}
	gzip -9 ../../$RAMDISK
	echo Created ${PRODUCT_OUT}/$RAMDISK.gz
	cd - > /dev/null
fi

