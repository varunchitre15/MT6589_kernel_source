#!/usr/bin/perl
#
#
#
($#ARGV != 0) && &Usage;
$filename = $ARGV[0];

die "Can't find the file: $filename\n" if (!-e $filename);
die "Should be a file, not a folder: $filename\n" if (-d $filename);
exit 0 if (-z $filename);

$pmPath = "mediatek/build/tools";
$pmFile = "yusuLib.pm";
$i = 0;
while (!-e "${pmPath}/${pmFile}") {
  $i++;
  $pmPath= "../$pmPath";
  last if ($i > 8);
}
die "Can NOT find $pmPath" if (!-e $pmPath);

unshift(@INC, $pmPath);
require $pmFile;

$showthefile = -1;
$send_admin_notify = 0;

  open(F, $filename ) || die "Can't open $filename";

  while(<F>) {
    if (/In file included from (\S+)/) { # appear beofre depend header error
      $thefile = $1;
      if ($thefile =~ /([^:]+)/) {
        $thefile = $1;
      } else {
        &SendAdmEmail(0x4, $thefile);
        # SendAdmEmail is for exception that out of our expectation
      }
      $showthefile = 0;
      $errlist{$thefile} .= $_;
      #print "$thefile\n";
    } elsif (/^\s+ from (\S+)/) {
      # this line should be appeard after "In file included from"
      $errlist{$thefile} .= $_;
      #print "$thefile\n";
    } elsif (/(\S+):\d+: undefined reference /) { # link error
      print "$_";
      $thefile = $1;
      $errlist{$thefile} .= $_;
      $errfilelist{$thefile} = 1;
    } elsif (/(\S+) error:/) { # error in .c/.h
      $depfile = $1;
      $errModule++;
      print "$_";

      if ($depfile =~ /([^:]+)/){
        $depfile = $1;
        if (!defined($errfilelist{$depfile})) {
           $errfilelist{$depfile} = 2;
        }
      } else {
        &SendAdmEmail(0x4, $_);
      }

      if ($depfile =~ /\.h/) {
        $errlist{$thefile} .= $_;
      } elsif (($depfile =~ /([^\/]+\.)c/) || ($depfile =~ /([^\/]+\.)cpp/))  {
        $objfile = $1 . "o";
        $errlist{$depfile} .= $_;
        $showthefile = 1;
      }

      # the following part should be reviewed
      if ($showthefile < 0) {
        print "\n\n$thefile:\n";
        $showthefile = 1;
      } elsif ($showthefile == 0) {
        print "\n\n$thefile:\n";
        $showthefile = 1;
        $errfilelist{$thefile} = 1;
      }
      if ($errModule > 300) {
        &SendAdmEmail(0x8, $errModule);
        last;
      }
    } elsif (/(\S+): Permission denied/) {
      &SendAdmEmail(0x2, $_); # need to check permission or codebase
      $perlist{$1} = 1;
      print "$_";
    } elsif (/No such file or directory/) { # may copy fully.
      &SendAdmEmail(0x10, $_);
      print "$_";
    } elsif (/No rule to make target/) { # Android.mk define the file that is not exist in P4
      &SendAdmEmail(0x40, $_);
      print "$_";
    } elsif (/directory does not exist/) { # java related
      &SendAdmEmail(0x1, $_);
      print "$_";
    } elsif (/\[(\S+)\] Error/) { # how to map Java ???
      if (!(($objfile ne "") && (/$objfile/))) { 
        # Skip if obj file error since we have dealt this part.
        &SendAdmEmail(0x1, $_);
        print "$_";
      }
    }
    $preLine = $_;
  } ## while
  close(F);

exit 0;
     
sub Usage {
  warn << "__END_OF_USAGE";
Usage:  parse_err.pl SOURCE_LOG_FILE
Parse SOURCE_LOG_FILE and extract error infomation to stdout.

SOURCE_LOG_FILE is the file from stdout while compiling uboot, kernel,
or Android system.
__END_OF_USAGE

  exit 1;
}

