#!/usr/bin/perl -w
# 
($#ARGV == 2 || $#ARGV == 1 || $#ARGV == 0) || &Usage;
($prj, $platform, $emmc_support) = @ARGV;

(exit 0) if ($prj eq "generic");
$logDir = "out/target/product/$prj";

if (!-d $logDir){
print "Can NOT find $logDir\n";
exit 0;
} 

my @lnmatrix = ();
if ($ENV{'KBUILD_OUTPUT_SUPPORT'} eq "yes")
{
   @lnmatrix = (
  "mediatek/preloader/preloader_${prj}.bin",
  "bootable/bootloader/lk/build-${prj}/lk.bin",
  "bootable/bootloader/lk/build-${prj}/logo.bin",
  "kernel/out/kernel_${prj}.bin",
  );
}
else
{
  @lnmatrix = (
  "mediatek/preloader/preloader_${prj}.bin",
  "bootable/bootloader/lk/build-${prj}/lk.bin",
  "bootable/bootloader/lk/build-${prj}/logo.bin",
  "kernel/kernel_${prj}.bin",
  );   
}
  
if ($emmc_support eq "yes")
{
  push(@lnmatrix,"mediatek/misc/${platform}_Android_scatter_emmc.txt");
  push(@lnmatrix,"mediatek/misc/MBR");
	opendir (DIR,"mediatek/misc");
	@dir = readdir(DIR);
	foreach $temp (@dir){
		if($temp=~/(EBR\d)/){
			#print "~~~~~~~~~copy  $temp $1\n";
			push(@lnmatrix,"mediatek/misc/$1");
		}
	}
	closedir DIR;
}
else{
  push(@lnmatrix,"mediatek/misc/${platform}_Android_scatter.txt");	
}

chdir($logDir);
$relDir = $logDir;
$relDir =~ s/[^\/]+/../g;

foreach $i (@lnmatrix) {
  $lnfile = "${relDir}/$i";
  $i =~ /([^\/]+)$/;
  $j = $1;
  if ($j =~ /kernel\.bin/) {
    $j = "kernel.bin";
  }
  system("rm $j") if (-e $j);
  if (!-e $lnfile) {
     print("$lnfile does NOT exist!\n");
     next;
  }
  if ($lnfile =~ /kernel\.bin/) {
    system("ln -s $lnfile kernel.bin");
  } else {
    system("ln -s $lnfile .");
  }
}

exit 0;

sub Usage {
  warn << "__END_OF_USAGE";
Usage: (\$prj, \$platform, \$emmc_support) = @ARGV 
__END_OF_USAGE
  exit 1;
}

