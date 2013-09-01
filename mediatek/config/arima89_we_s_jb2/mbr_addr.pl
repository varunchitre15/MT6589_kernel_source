#!/usr/local/bin/perl -w

#You can change $MBR_Start_Address_KB, the size equals the BOOT1+BOOT2+RPMB of the max of eMMC chips you want use.
#You Can write a formula rather than a number calculated by yourself
# $MBR_Start_Address_KB = 1024+1024+128; is right
# $MBR_Start_Address_KB = 6*1024+128; is right

$MBR_Start_Address_KB = 6144;
print "[Ptgen in module] MBR_Start_Address_KB = $MBR_Start_Address_KB\n";

return $MBR_Start_Address_KB;
