#!/usr/bin/perl
#use strict;
use Cwd;

# the path of package;
my $path = $ARGV[0];

# the path of pre-assign assets path;
my $assets_path = $ARGV[1];

# project name
my $proj = $ARGV[2];

# overlay_path
my $overlay_path = $ARGV[3];


# all log of optimization will be dumper to this file;


die "can not create log file:$!" if (! open OUTPUT, ">>out/target/product/${proj}_AppAssets_Overlay.log");


my $temp_dir = "out/target/product/${proj}_assetsOverlay";
 
if ($overlay_path eq ""){
    print "$assets_path";
    exit 0;
  }

my @overlays = split(" ",$overlay_path,7);

@overlays = reverse(@overlays);

print OUTPUT "overlays: @overlays \n";

# Ignore resource optimization if assets directory NOT exist
if (defined $assets_path)
{
  if (!-d $assets_path)
  {
    print "$assets_path";
    exit 0;
  }
}
else
{
    exit 0;
}



&p_system("rm -rf $temp_dir/$path") if (-d "$temp_dir/$path");
&p_system("mkdir -p $temp_dir/$path/");
&p_system("cp -af $assets_path $temp_dir/$path");

my $command = "find ".$assets_path."/";     
my @result = readpipe($command);
  

# est. ref.hash table
my $buildPass = 1;
my $errorMessage = "";

foreach $overlay (@overlays) {
   &p_system("cp -af $overlay/$assets_path $temp_dir/$path");
   my $commandSub = "find ".$overlay."/".$assets_path."/";     
   my @resultSub = readpipe($commandSub);
   foreach $resultSub (@resultSub) {
      chomp $resultSub;
      my $overlaySub = $overlay."/";
      my @search = split("$overlaySub",$resultSub,5);
      my $search = @search[1]; 
      my $foundlist = (grep/^$search$/,@result);
      if ($foundlist == 0){
      $errorMessage .= "[Error][assets_overlay]There is no ".$resultSub." in common folder!\n";
      $buildPass = 0;
       }
     }   
  }

if ($buildPass == 0){
    die "$errorMessage\n";
  }

print "$temp_dir/$assets_path";
# foreach ovrlays, call overlay_process
 


sub p_system
{
    my $rslt = system("$_[0]");
    print OUTPUT "$_[0]\n" if ($rslt == 0);
}
