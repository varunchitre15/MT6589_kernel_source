#!/usr/bin/perl


#######################################
# Description
## The script is for checking the consistency between modem.img (multiple) and MTK_MODEM_SUPPORT/MTK_MD2_SUPPORT/MTK_PLATFORM option setting.
## Check item: mode(2G/3G), platform, project ID(branch/week), serial number
#######################################
#######################################
# NOTICE
## This script is specific for MT6589
## NOT backward compatible!!!!
#######################################

usage() if ($#ARGV < 1);

my ($AP_Project_Name, $AP_SW_Version, $modem_path, $MTK_PLATFORM, $mtk_modem_support, $mtk_modem_2nd_support, $bin_info); # = @ARGV;

$AP_SW_Version = $ENV{MTK_BUILD_VERNO};        # $AP_SW_Version   --> modem-info

if($ARGV[0]=~/PROJECT=(.*)/){
	$AP_Project_Name = $1;
}

if($ARGV[1]=~/PRIVATE_MODEM_PATH=(.*)/){
	$modem_path = $1;
}

if($ARGV[2]=~/MTK_PLATFORM=(.*)/){
	$MTK_PLATFORM = $1;
}

if($ARGV[3]=~/MTK_MODEM_SUPPORT=(.*)/){
	$mtk_modem_support = $1;
}

if($ARGV[4]=~/MTK_MD2_SUPPORT=(.*)/){
	$mtk_modem_2nd_support = $1;
}

if($ARGV[5]=~/MTK_GET_BIN_INFO=(.*)/){	
	$bin_info= $1;
	if ($bin_info =~ /info/)
	{
		$bin_info= 1;
	}
	else
	{
		$bin_info='';
	}
}


#######################################
# Initialization
#######################################
my $whence = 0;

my $length1 = 188; # modem.img rear
my $length2 = 172;

my $length3 = 128; # modem.img project name
my $length4 = 36;  # modem.img flavor
my $length5 = 64;  # modem.img verno

my $mode;

my $MD_IMG;
my $MD_IMG_DEBUG;
my $MD_IMG_MODE;
my $MD_IMG_PLATFORM;
my $MD_IMG_CHIPVER;
my $MD_IMG_PROJECT_ID;
my $MD_IMG_SERIAL_NO;

my $MD_IMG_2ND;
my $MD_IMG_DEBUG_2ND;
my $MD_IMG_MODE_2ND;
my $MD_IMG_PLATFORM_2ND;
my $MD_IMG_CHIPVER_2ND;
my $MD_IMG_PROJECT_ID_2ND;
my $MD_IMG_SERIAL_NO_2ND;

my $MD_IMG_PROJECT_NAME;
my $MD_IMG_PROJECT_NAME_2ND;
my $MD_IMG_PROJECT_FLAVOR;
my $MD_IMG_PROJECT_FLAVOR_2ND;
my $MD_IMG_PROJECT_VERNO;
my $MD_IMG_PROJECT_VERNO_2ND;

my $errCnt = 0;

if ($mtk_modem_support eq "modem_3g") {
	$mode = 2; # 3G
} elsif ($mtk_modem_support eq "modem_2g") {
	$mode = 1; # 2G
} else {
	$mode = -1; # invalid
}

if ($mtk_modem_2nd_support eq "modem2_3g") {
	$mode_2nd = 2; # 3G
} elsif ($mtk_modem_2nd_support eq "modem2_2g") {
	$mode_2nd = 1; # 2G
} else {
	$mode_2nd = -1; # invalid
}

#######################################
# Check if MODEM file exists 
#######################################
if ($ENV{MTK_ENABLE_MD1} eq "yes") {
	$MD_IMG = $modem_path . "/modem.img";
	die "[MODEM CHECK FAILED]: The file \"$MD_IMG\" does NOT exist!\n" if (!-e $MD_IMG);
}
if ($ENV{MTK_ENABLE_MD2} eq "yes") {
	$MD_IMG_2ND = $modem_path . "/modem_sys2.img";
	die "[MODEM CHECK FAILED]: The file \"$MD_IMG_2ND\" does NOT exist!\n" if (!-e $MD_IMG_2ND);
}

#######################################
# Read mode(2G/3G), debug/release flag, platform info, project info, serial number, etc. from modem.img
#######################################
if ($ENV{MTK_ENABLE_MD1} eq "yes") {
	($MD_IMG_DEBUG, $MD_IMG_MODE, $MD_IMG_PLATFORM, $MD_IMG_PROJECT_ID, $MD_IMG_SERIAL_NO, $MD_IMG_PROJECT_NAME, $MD_IMG_PROJECT_FLAVOR, $MD_IMG_PROJECT_VERNO) = &Parse_MD($MD_IMG); 
}
if ($ENV{MTK_ENABLE_MD2} eq "yes") {
	($MD_IMG_DEBUG_2ND, $MD_IMG_MODE_2ND, $MD_IMG_PLATFORM_2ND, $MD_IMG_PROJECT_ID_2ND, $MD_IMG_SERIAL_NO_2ND, $MD_IMG_PROJECT_NAME_2ND, $MD_IMG_PROJECT_FLAVOR_2ND, $MD_IMG_PROJECT_VERNO_2ND) = &Parse_MD($MD_IMG_2ND);
}

sub Parse_MD {
	my $tmp_header_str;
	my $tmp_inutility1;
	my $tmp_debug_mode;
	my $tmp_mode;
	my $tmp_platform;
	my $tmp_inutility2;
	my $tmp_project_id;
	my $tmp_serial_num;
	
	my $tmp_project_name;
	my $tmp_flavor;
	my $tmp_verno;

	my ($parse_md_file) = @_;
	my $md_file_size = -s $parse_md_file;
#
#	open(MODEM, "< $parse_md_file") or die "Can NOT open file $parse_md_file\n";
#	binmode(MODEM);
#	seek(MODEM, $md_file_size - $length1, $whence) or die "Can NOT seek to the position $position_point in \"$parse_md_file\"!\n";
#	read(MODEM, $buffer, $length2) or die "Failed to read the file \"$parse_md_file\"!\n";
#	($tmp_header_str, $tmp_inutility1, $tmp_debug_mode, $tmp_mode, $tmp_platform, $tmp_inutility2, $tmp_project_id, $tmp_serial_num) = unpack("A12 L L L A16 A64 A64, L", $buffer);
#	die "[MODEM CHECK FAILED]: Reading from MODEM failed! No CHECK_HEADER info!\n" if ($tmp_header_str ne "CHECK_HEADER") ;
#	close(MODEM);
#	return ($tmp_debug_mode, $tmp_mode, $tmp_platform, $tmp_project_id, $tmp_serial_num);
	
	my $tmp = $length1+$length3+$length4+$length5;

	open(MODEM, "< $parse_md_file") or die "Can NOT open file $parse_md_file\n";
	binmode(MODEM);
	seek(MODEM, $md_file_size - $tmp, $whence) or die "Can NOT seek to the position $position_point in \"$parse_md_file\"!\n";
	read(MODEM, $buffer, $tmp) or die "Failed to read the file \"$parse_md_file\"!\n";
	($tmp_project_name, $tmp_flavor, $tmp_verno, $tmp_header_str, $tmp_inutility1, $tmp_debug_mode, $tmp_mode, $tmp_platform, $tmp_inutility2, $tmp_project_id, $tmp_serial_num) = unpack("A128 A36 A64 A12 L L L A16 A64 A64, L", $buffer);
	#$tmp_project_name = $tmp_header_str;
	die "[MODEM CHECK FAILED]: Reading from MODEM failed! No CHECK_HEADER info!\n" if ($tmp_header_str ne "CHECK_HEADER") ;
	close(MODEM);
	return ($tmp_debug_mode, $tmp_mode, $tmp_platform, $tmp_project_id, $tmp_serial_num, $tmp_project_name, $tmp_flavor, $tmp_verno);


}

#######################################
# Output debug information
#######################################
my $debug = 0; # output debug info.

if ($debug) {
	print "\n==========================\n";
	print "Modem path = $modem_path\n";
	if ($ENV{MTK_ENABLE_MD1} eq "yes") {
		print "\n==========================\n";
		print "*** Info from 1st modem image ***\n\n";
		print "modem image is $MD_IMG\n";
		print "\$MD_IMG_DEBUG = $MD_IMG_DEBUG [" . sprintf("0x%08x",$MD_IMG_DEBUG) . "]\n";
		print "\$MD_IMG_MODE = $MD_IMG_MODE [" . sprintf("0x%08x",$MD_IMG_MODE) . "]\n";
		print "\$MD_IMG_PLATFORM = $MD_IMG_PLATFORM\n";
		print "\$MD_IMG_PROJECT_ID = $MD_IMG_PROJECT_ID\n";
		print "\$MD_IMG_SERIAL_NO = $MD_IMG_SERIAL_NO [" . sprintf("0x%08x",$MD_IMG_SERIAL_NO) . "]\n";
	}
	if ($ENV{MTK_ENABLE_MD2} eq "yes") {
		print "\n==========================\n";
		print "*** Info from 2nd modem image ***\n\n";
		print "modem image is $MD_IMG_2ND\n";
		print "\$MD_IMG_DEBUG_2ND = $MD_IMG_DEBUG_2ND [" . sprintf("0x%08x",$MD_IMG_DEBUG_2ND) . "]\n";
		print "\$MD_IMG_MODE_2ND = $MD_IMG_MODE_2ND [" . sprintf("0x%08x",$MD_IMG_MODE_2ND) . "]\n";
		print "\$MD_IMG_PLATFORM_2ND = $MD_IMG_PLATFORM_2ND\n";
		print "\$MD_IMG_PROJECT_ID_2ND = $MD_IMG_PROJECT_ID_2ND\n";
		print "\$MD_IMG_SERIAL_NO_2ND = $MD_IMG_SERIAL_NO_2ND [" . sprintf("0x%08x",$MD_IMG_SERIAL_NO_2ND) . "]\n";
	}
	print "\n==========================\n";
	print "*** Info from feature option configuration ***\n\n";
	print "\$MTK_MODEM_SUPPORT = $mtk_modem_support\n";
	print "\$MTK_MD2_SUPPORT = $mtk_modem_2nd_support\n";
	print "\$MTK_PLATFORM = $MTK_PLATFORM\n\n";
}
#######################################
# Output modem information
#######################################
if ($bin_info){
	print "\[AP Project Name\]: $AP_Project_Name\n";
	print "\[AP SW Version\]: $AP_SW_Version\n";
	print "\[MD1 Project Name\]: $MD_IMG_PROJECT_NAME\n";
	print "\[MD1 SW Version\]: $MD_IMG_PROJECT_VERNO\n";
	print "\[MD1 Flavor\]: $MD_IMG_PROJECT_FLAVOR\n";
	print "\[MD2 Project Name\]: $MD_IMG_PROJECT_NAME_2ND\n";
	print "\[MD2 SW Version\]: $MD_IMG_PROJECT_VERNO_2ND\n";
	print "\[MD2 Flavor\]: $MD_IMG_PROJECT_FLAVOR_2ND\n";
	print "\[Site\]: ALPS\n";
}

#######################################
# Check file consistency of modem.img and MTK_MODEM_SUPPORT/MTK_MD2_SUPPORT
## 2G/3G
#######################################
my $modeErrCnt = 0;

if (($ENV{MTK_ENABLE_MD1} eq "yes") && ($MD_IMG_MODE != $mode)) {
	$modeErrCnt++;
	warn "[MODEM CHECK FAILED]: Inconsistent 2G/3G mode for 1st MD!\n";
	warn "[MODEM CHECK FAILED]: \"$MD_IMG\" ==> " . &getMODE (MODE, $MD_IMG_MODE) . "\n";
	warn "[MODEM CHECK FAILED]: \"MTK_MODEM_SUPPORT\" ==> " .  &getMODE (MODE, $mode) . "\n\n";
}
if (($ENV{MTK_ENABLE_MD2} eq "yes") && ($MD_IMG_MODE_2ND != $mode_2nd)) {
	$modeErrCnt++;
	warn "[MODEM CHECK FAILED]: Incorrect modem number for 2nd MD!\n";
	warn "[MODEM CHECK FAILED]: \"$MD_IMG_2ND\" ==> " . &getMODE (MODE, $MD_IMG_MODE_2ND) . "\n"; 
	warn "[MODEM CHECK FAILED]: \"MTK_MD2_SUPPORT\" ==> " .  &getMODE (MODE, $mode_2nd) . "\n\n";	
}

#######################################
# Check consistency of modem.img and MTK_PLATFORM
## platform
#######################################
my $platformErrCnt = 0;

if ($ENV{MTK_ENABLE_MD1} eq "yes") {
	$MD_IMG_PLATFORM =~ s/_(.*)//; 
	$MD_IMG_CHIPVER = $1;
	$platformErrCnt++ if ($MD_IMG_PLATFORM ne $MTK_PLATFORM);
}
if ($ENV{MTK_ENABLE_MD2} eq "yes") {
	$MD_IMG_PLATFORM_2ND =~ s/_(.*)//; 
	$MD_IMG_CHIPVER_2ND = $1;
	$platformErrCnt++ if ($MD_IMG_PLATFORM_2ND ne $MTK_PLATFORM);
}

if ($platformErrCnt) {
	warn "[MODEM CHECK FAILED]: Inconsistent platform!\n";
	warn "[MODEM CHECK FAILED]: \"$MD_IMG\" ==> " . $MD_IMG_PLATFORM . "\n" if ($ENV{MTK_ENABLE_MD1} eq "yes");
	warn "[MODEM CHECK FAILED]: \"$MD_IMG_2ND\" ==> " . $MD_IMG_PLATFORM_2ND . "\n" if ($ENV{MTK_ENABLE_MD2} eq "yes");
	warn "[MODEM CHECK FAILED]: MTK_PLATFORM ==> $MTK_PLATFORM\n\n";
}

#######################################
# Check consistency of modem.img
## project ID (branch/week info)
#######################################
my $projectErrCnt = 0;

#######################################
# Check modem serial number
## modem.img: 0x00
## modem_sys2.img: 0x01 
#######################################
my $serialErroCnt = 0;

if (($ENV{MTK_ENABLE_MD1} eq "yes") && ($MD_IMG_SERIAL_NO ne "1")) {
	$serialErroCnt ++;
	warn "[MODEM CHECK FAILED]: Incorrect modem number for 1st MD!\n";
	warn "[MODEM CHECK FAILED]: \"$MD_IMG\" ==> ".  $MD_IMG_SERIAL_NO . "\n";
	warn "[MODEM CHECK FAILED]: The value should be 1!\n";
	warn "\n";
}
if (($ENV{MTK_ENABLE_MD2} eq "yes") && ($MD_IMG_SERIAL_NO_2ND ne "2")) {
	$serialErroCnt ++;
	warn "[MODEM CHECK FAILED]: Incorrect modem number for 2nd MD!\n";
	warn "[MODEM CHECK FAILED]: \"$MD_IMG_2ND\" ==> ".  $MD_IMG_SERIAL_NO_2ND . "\n";
	warn "[MODEM CHECK FAILED]: The value should be 2!\n";
	warn "\n";
}

#######################################
# print error message if any and exit
#######################################
$errCnt = $errCnt + $modeErrCnt + $debugErrCnt + $platformErrCnt + $projectErrCnt + $serialErroCnt;
die "[MODEM CHECK FAILED]: Please check the modem images inconsistency!\n" if ($errCnt);

print "[MODEM CHECK INFO]: Check modem image DONE!\n" unless ($bin_info);
exit 0;

sub getMODE {
	my ($type,$type_value) = @_;
	if ($type_value == 2) {
		$return_tpye = "3G" if ($type eq "MODE");
		$return_tpye = "release" if ($type eq "DEBUG");
	} elsif($type_value == 1) {
		$return_tpye = "2G" if ($type eq "MODE");
		$return_tpye = "debug" if ($type eq "DEBUG");
	} else {
		$return_tpye = "invalid";
	}
	return $return_tpye;
}

sub usage
{
	print <<"__EOFUSAGE";

Usage:
$0 [Modem Path] [MTK_PLATFORM] [MTK_MODEM_SUPPORT] [MTK_MD2_SUPPORT]

Modem Path           Path of modem folder
MTK_PLATFORM         current project platform
                     (ex. MT6516, MT6573)
MTK_MODEM_SUPPORT    mode of 1st modem of current project
                     (ex. modem_3g, modem_2g)
MTK_MD2_SUPPORT    mode of 2nd modem of current project
                     (ex. modem_3g, modem_2g)                  
__EOFUSAGE
	exit 1;
}
