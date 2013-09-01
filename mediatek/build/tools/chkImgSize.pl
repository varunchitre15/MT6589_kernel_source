#!/usr/bin/perl -w
use strict;
use File::Basename;
use File::stat;

usage() if ($#ARGV < 2);

my $DEBUG = 0;

my ($scatFile, $project, @images) = @ARGV;

# Data structure
my $imgSize = [];

# $imgSize = 
#[
#  {
#    NAME => string,
#    STARTADDR => hex,
#    QUOTA => dec,
#    SIZE  => dec
#  },
#...
#]

# There are following blocks defined in scatter file now
# PRELOADER,DSP_BL,UBOOT,BOOTIMG,RECOVERY,SEC_RO,ANDROID,LOGO,USRDATA
my $imgTbl = 
{
  PRELOADER => "preloader_$project.bin",
  DSP_BL    => "DSP_BL",
  UBOOT     => "uboot_$project.bin",
  BOOTIMG   => "boot.img",
  RECOVERY  => "recovery.img",
  SEC_RO    => "secro.img",
  ANDROID   => "system.img",
  LOGO      => "logo.bin",
  USRDATA   => "userdata.img"
};

$imgSize = parseScatter($scatFile, $imgSize);
$imgSize = calcImgSizeQuota($imgSize);
$imgSize = getImgSize(\@images, $imgTbl, $imgSize);

chkExpiredImg($imgSize, $imgTbl);
print "Checking memory usage DONE!\n";
exit 0;

# Parse scatter file to get the allocated start address of each image
sub parseScatter
{
  my ($file, $arrRef) = @_;
  my $imgBlockIdx = 0;

  open (SCATTER, "<$file") or die "Can not open file \"$file\"!\n";
  while(<SCATTER>)
  {
    chomp;
    if (/^(\w+)\s+(0x\w+)/i)
    {
      $arrRef->[$imgBlockIdx]->{NAME} = $1;
      $arrRef->[$imgBlockIdx]->{STARTADDR} = hex($2);
      $imgBlockIdx++;
    }
  }
  close(SCATTER);
  return $arrRef;
}

# Calculate the quota of each image
sub calcImgSizeQuota
{
  my $arrRef = shift;
  for (my $idx=0; $idx<=$#$arrRef; $idx++)
  {
	#print "$arrRef->[$idx]->{NAME}:";
    if ($idx == $#$arrRef || ($arrRef->[$idx]->{NAME} =~/__NODL_/) || ($arrRef->[$idx]->{NAME} =~/FAT/))
    {	
		#print "No need to check quota \n";
      $arrRef->[$idx]->{QUOTA} = "N/A";
    }
    else
    { 
      # in bytes
      $arrRef->[$idx]->{QUOTA} = $arrRef->[$idx+1]->{STARTADDR} - $arrRef->[$idx]->{STARTADDR};
		#print "quota is $arrRef->[$idx]->{QUOTA}\n";
    }
  }
  return $arrRef;
}

# Get image real size
sub getImgSize
{
  my ($imagesRef, $imgTblRef, $imgSizeRef) = @_;
  foreach my $img (@$imagesRef)
  {
    die "\"$img\" does NOT exist!\n" if (!-e $img);
    for (my $i=0; $i<=$#$imgSizeRef; $i++)
    {
      if (defined($imgTblRef->{$imgSizeRef->[$i]->{NAME}}) 
       && (basename($img) eq $imgTblRef->{$imgSizeRef->[$i]->{NAME}}))
      {
        $imgSizeRef->[$i]->{SIZE} = stat($img)->size; # in bytes
      }
      else
      {
        next if (defined($imgSizeRef->[$i]->{SIZE}) && ($imgSizeRef->[$i]->{SIZE}>0));
        $imgSizeRef->[$i]->{SIZE} = 0;
      }
    }
  }
  return $imgSizeRef;
}

# Check if image size exceeds the allocated quota
sub chkExpiredImg
{
  my ($imgSizeRef, $imgTblRef) = @_;
  my $errCnt = 0;
  for (my $idx=0; $idx<$#$imgSizeRef; $idx++)
  {
	if($imgSizeRef->[$idx]->{QUOTA} ne "N/A"){
		#print "$imgSizeRef->[$idx]->{NAME} size: $imgSizeRef->[$idx]->{SIZE} quota:$imgSizeRef->[$idx]->{QUOTA}\n";
		if ($imgSizeRef->[$idx]->{SIZE} > $imgSizeRef->[$idx]->{QUOTA})
		{
		$errCnt++;
		warn "[MEM SIZE EXPIRED] ",$imgTblRef->{$imgSizeRef->[$idx]->{NAME}},"\n";
		printf("%s%s\t%s%08X\n","[MEM SIZE EXPIRED] ","Start Addr:","0x",$imgSizeRef->[$idx]->{STARTADDR});
		print "[MEM SIZE EXPIRED] Alloc Quota:\t",$imgSizeRef->[$idx]->{QUOTA}," byte(s)\n";
		print "[MEM SIZE EXPIRED] Real Size:\t",$imgSizeRef->[$idx]->{SIZE}," byte(s)\n";
		print "[MEM SIZE EXPIRED] Shortage:\t",($imgSizeRef->[$idx]->{SIZE} - $imgSizeRef->[$idx]->{QUOTA})," byte(s)\n";
		print "\n";
		}
		elsif ($imgSizeRef->[$idx]->{SIZE} > $imgSizeRef->[$idx]->{QUOTA}*0.9)
		{# image size alert once 90+% quota used
		warn "[MEM SIZE ALERT] ",$imgTblRef->{$imgSizeRef->[$idx]->{NAME}},"\n";
		printf("%s%s\t%s%08X\n","[MEM SIZE ALERT] ","Start Addr:","0x",$imgSizeRef->[$idx]->{STARTADDR});
		print "[MEM SIZE ALERT] Alloc Quota:\t",$imgSizeRef->[$idx]->{QUOTA}," byte(s)\n";
		print "[MEM SIZE ALERT] Real Size:\t",$imgSizeRef->[$idx]->{SIZE}," byte(s)\n";
		print "[MEM SIZE ALERT] Remaining:\t",($imgSizeRef->[$idx]->{QUOTA} - $imgSizeRef->[$idx]->{SIZE})," byte(s)\n";
		print "\n";
		}
	}
  }
  if ($errCnt)
  {
    exit 1;
  }
}

sub dumpImgSize
{
  my $hashRef = shift;
  printf("%s\t%s%08X\n","Start Addr:","0x",$hashRef->{STARTADDR});
  print "Alloc Quota:\t",$hashRef->{QUOTA}," byte(s)\n";
  print "Real Size:\t",$hashRef->{SIZE}," byte(s)\n";
}

sub usage
{
  print <<"__EOFUSAGE";

Usage:
$0  scatter_file project_name image_name [image_name2 ... [image_nameN]]

scatter_file     scatter file (ex. MT6516_Android_scatter.txt)
project_name     project name (ex. e1kv2)
image_name       image file name (ex. system.img)

__EOFUSAGE
  exit 1;
}


