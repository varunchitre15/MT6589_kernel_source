#!/bin/bash

# Collect all loadmodules in a folder named after target product and kernel
# configuration
#
# The arguments to this script contain the environment variables that need
# to be set in order for this script to function properly, specifically
# $TOP and $KERNEL_DEFCONFIG

# Make any environment variables on the command line available.
export set TOP=$(pwd)
export set project=$1
export set SEC=$2
export set S1_Image=$3
export IMAGE_OUTPUT=${TOP}/images/${project}
export IMAGE_OUTPUT_SIGN=${TOP}/images/${project}_sign
export IMAGE_OUTPUT_S1=${TOP}/images/${project}_S1
export PRODUCT_OUT=${TOP}/out/target/product/${project}
export PRODUCT_SECOUT=${TOP}/out/target/product/${project}/signed_bin
export PRODUCT_S1OUT=${TOP}/out/target/product/${project}/S1_signbin
export MTK_CGEN=${TOP}/mediatek/cgen
export VMLINUX_PATH=${TOP}/kernel/out
export SYMBOLS_OUTPUT=${TOP}/symbols/${project}
export SYMBOLS_SRC=${TOP}/out/target/product/${project}/symbols

# Create destination directory
#mkdir -p ${IMAGE_OUTPUT}
#mkdir -p ${IMAGE_OUTPUT_SIGN}
#mkdir -p ${SYMBOLS_OUTPUT}

#No sign
if [ "$SEC" == "NO" ]; then
mkdir -p ${IMAGE_OUTPUT}
mkdir -p ${SYMBOLS_OUTPUT}

for FILE in `find ${PRODUCT_OUT}/*.* -name "*.*" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${PRODUCT_OUT}/${FILE} ${IMAGE_OUTPUT}
done

if [ -e ${PRODUCT_OUT}/DSP_BL ]; then
  ln -fs ${PRODUCT_OUT}/DSP_BL ${IMAGE_OUTPUT}
else
  echo "WARNING: ${PRODUCT_OUT}/DSP_BL not found"
fi

if [ -e ${PRODUCT_OUT}/EBR1 ]; then
  ln -fs ${PRODUCT_OUT}/EBR1 ${IMAGE_OUTPUT}
else
  echo "WARNING: ${PRODUCT_OUT}/EBR1 not found"
fi

if [ -e ${PRODUCT_OUT}/EBR2 ]; then
  ln -fs ${PRODUCT_OUT}/EBR2 ${IMAGE_OUTPUT}
else
  echo "WARNING: ${PRODUCT_OUT}/EBR1 not found"
fi

if [ -e ${PRODUCT_OUT}/MBR ]; then
  ln -fs ${PRODUCT_OUT}/MBR ${IMAGE_OUTPUT}
else
  echo "WARNING: ${PRODUCT_OUT}/MBR not found"
fi

for FILE in `find ${MTK_CGEN}/*.* -name "APDB*.*" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${MTK_CGEN}/${FILE} ${IMAGE_OUTPUT}
done

if [ -e ${PRODUCT_OUT}/kernel ]; then
  ln -fs ${PRODUCT_OUT}/kernel ${IMAGE_OUTPUT}
else
  echo "WARNING: ${PRODUCT_OUT}/kernel not found"
fi

ln -fs ${VMLINUX_PATH}/vmlinux ${IMAGE_OUTPUT}

fi

#MTK sign
if [ "$SEC" == "YES" ]; then
mkdir -p ${IMAGE_OUTPUT_SIGN}
mkdir -p ${SYMBOLS_OUTPUT}

for FILE in `find ${PRODUCT_OUT}/*.txt -name "*.txt" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${PRODUCT_OUT}/${FILE} ${IMAGE_OUTPUT_SIGN}
done

for FILE in `find ${PRODUCT_OUT}/flex.* -name "flex.*" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${PRODUCT_OUT}/${FILE} ${IMAGE_OUTPUT_SIGN}
done

for FILE in `find ${PRODUCT_OUT}/XT*.* -name "XT*.*" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${PRODUCT_OUT}/${FILE} ${IMAGE_OUTPUT_SIGN}
done

for FILE in `find ${PRODUCT_SECOUT} | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${PRODUCT_SECOUT}/${FILE} ${IMAGE_OUTPUT_SIGN}
done

if [ -e ${PRODUCT_OUT}/DSP_BL ]; then
  ln -fs ${PRODUCT_OUT}/DSP_BL ${IMAGE_OUTPUT_SIGN}
else
  echo "WARNING: ${PRODUCT_OUT}/DSP_BL not found"
fi

if [ -e ${PRODUCT_OUT}/preloader_${project}.bin ]; then
  ln -fs ${PRODUCT_OUT}/preloader_${project}.bin ${IMAGE_OUTPUT_SIGN}
else
  echo "WARNING: ${PRODUCT_OUT}/preloader_${project}.bin not found"
fi

for FILE in `find ${MTK_CGEN}/*.* -name "APDB*.*" | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${MTK_CGEN}/${FILE} ${IMAGE_OUTPUT_SIGN}
done

if [ -e ${PRODUCT_OUT}/kernel ]; then
  ln -fs ${PRODUCT_OUT}/kernel ${IMAGE_OUTPUT_SIGN}
else
  echo "WARNING: ${PRODUCT_OUT}/kernel not found"
fi

ln -fs ${VMLINUX_PATH}/vmlinux ${IMAGE_OUTPUT_SIGN}

fi

#Sony S1 sign
if [ "$SEC" == "S1" ]; then
mkdir -p ${IMAGE_OUTPUT_S1}
mkdir -p ${SYMBOLS_OUTPUT}

ln -fs ${PRODUCT_OUT}/${S1_Image}.zip ${IMAGE_OUTPUT_S1}
ln -fs ${VMLINUX_PATH}/vmlinux ${IMAGE_OUTPUT_S1}

fi


for FILE in `find ${SYMBOLS_SRC} | xargs -i basename {}`  
do
	echo File : ${FILE}
	ln -fs ${SYMBOLS_SRC}/${FILE} ${SYMBOLS_OUTPUT}
done

echo ""
echo "Kernel, u-boot, file system images and flashkit collected in:"
echo "${IMAGE_OUTPUT}/"
echo ""
