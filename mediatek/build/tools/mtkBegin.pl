#!/usr/bin/perl
# 
($#ARGV != 0) && &Usage;
($prj) = @ARGV;

($prj = "generic") if ($prj eq "emulator");

$flag_subrel = "mediatek/build/android/full/config.mk";
$flag_custrel = "mediatek/build/android/full/config.mk.custrel";
$srcDir = "vendor/mediatek/$prj/artifacts/out/";
$dstDir = "out/";
$tmpDir = "vendor/mediatek/$prj/artifacts/kernel/out/";

exit 0, if (!-e $flag_subrel && !-e $flag_custrel);
exit 0, if (-e $flag_subrel && -e $flag_custrel);

if (!-e $flag_subrel)
#if (0)
{
	if (-d $srcDir)
	{
	  system("rsync -av --exclude=.svn --exclude=.git --exclude=.cvs $srcDir $dstDir > auto_sync_android.log 2>&1");
	}
	exit 0;
}

if (!-e $flag_custrel)
#if (!-e $flag_subrel)
{
	my $binaryAppPath = $srcDir . "/target/product/$prj/system/app/";
	#print "app list $binaryAppPath\n";
	my @applist = <$binaryAppPath/*.apk>;
        
	foreach my $app (@applist)
	{
	  #print "Signing using customerization signature for $app \n";
	  &signApk($app);
	}
	if (-d $srcDir)
	{
	  system("rsync -av --exclude=.svn --exclude=.git --exclude=.cvs $srcDir $dstDir > auto_sync_android.log 2>&1");
	} 
	if (-d $tmpDir)
	{
	  system("rsync -av --exclude=.svn --exclude=.git --exclude=.cvs $tmpDir kernel/out/ > auto_sync_kernel.log 2>&1\n");
	}
}

exit 0;

sub Usage {
  warn << "__END_OF_USAGE";
Usage: $myCmd project
__END_OF_USAGE
  exit 1;
}

sub signApk {
  my ($src) = @_;
  my $keypath = "";
  my $src_tmp = $src . ".bak";
  my $signTool = $srcDir . "/host/linux-x86/framework/signapk.jar";
  if ($ENV{"MTK_SIGNATURE_CUSTOMIZATION"} eq "yes")
  {
    if ($ENV{"MTK_INTERNAL"} eq "yes")
    {
      $keypath = "build/target/product/security/common";
    }
    else
    {
      $keypath = "build/target/product/security/$prj";
    }
  }
  else
  {
    $keypath = "build/target/product/security";
  }
  my $key1 = "$keypath/platform.x509.pem";
  my $key2 = "$keypath/platform.pk8";
  #print "java -jar $signTool $key1 $key2 $src $src_tmp";
  system ("java -jar $signTool $key1 $key2 $src $src_tmp");
  system ("mv $src_tmp $src");
}

