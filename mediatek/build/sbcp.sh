echo "**********************************"
echo "Do the SBC base check process... "
echo "**********************************"

rm -f ./out/tmp/sbchk/sbcerr

wine ./mediatek/secutool/sbc/csbc.exe ./out/tmp/sbchk/ 	./mediatek/platform/mt6589/kernel/core/include/mach/sbchk_base.h

if [ -f ./out/tmp/sbchk/sbcerr ]; then 
	cp -f  ./out/tmp/sbchk/sbchk_base.h.update ./mediatek/platform/mt6589/kernel/core/include/mach/sbchk_base.h
	echo "Checking the sbc base hase fail. Please do the "n kernel" and "bootimage" again."
else	
	echo "Checking the  sbc base hase pass."
fi