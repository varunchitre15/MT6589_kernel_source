echo "**********************************"
echo "Do the SBC base check process... "
echo "**********************************"

if [ -d ./out/tmp/sbchk ]; then 
	echo "del sbchk folder "
	rm -R ./out/tmp/sbchk
	echo "create sbchk folder "	
	mkdir ./out/tmp/sbchk
else
	echo "create sbchk folder "
	mkdir ./out/tmp
	mkdir ./out/tmp/sbchk
fi

cp -f ./out/target/product/arima89_we_s_jb2/system/bin/sbchk 				./out/tmp/sbchk
cp -f ./out/target/product/arima89_we_s_jb2/system/lib/modules/sec.ko		./out/tmp/sbchk
cp -f ./out/target/product/arima89_we_s_jb2/system/lib/modules/ccci.ko		./out/tmp/sbchk
cp -f ./out/target/product/arima89_we_s_jb2/system/lib/modules/ccci_plat.ko	./out/tmp/sbchk
cp -f ./out/target/product/arima89_we_s_jb2/root/init.rc 					./out/tmp/sbchk

wine ./mediatek/secutool/sbc/csbc.exe ./out/tmp/sbchk/ 	./mediatek/platform/mt6589/kernel/core/include/mach/sbchk_base.h

if [ -f ./out/tmp/sbchk/sbcerr ]; then 
	 cp -f  ./out/tmp/sbchk/sbchk_base.h.update ./mediatek/platform/mt6589/kernel/core/include/mach/sbchk_base.h
	echo "Checking the sbc base hase fail. Please do the "n kernel" and "bootimage" again."
else	
	echo "Checking the  sbc base hase pass."
fi