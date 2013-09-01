#!/usr/bin/perl

##########################################################
# Initialize Variables
##########################################################
my $prj = $ARGV[0];
my $key_dir = "mediatek/custom/$prj/security/image_auth";
my $cfg_dir = "mediatek/custom/$prj/security/sec_file_list";
my $cipher_tool = "mediatek/build/tools/CipherTool/CipherTool";
my $sign_tool = "mediatek/build/tools/SignTool/SignTool.sh";


##########################################################
# Sign ANDROID Secure File List
##########################################################
print "\n\n*** Sign ANDROID Secure File List ***\n\n";

my $and_secfl = "mediatek/custom/$prj/security/sec_file_list/ANDRO_SFL.ini";
my $s_andro_fl = "mediatek/external/seclib/S_ANDRO_SFL.ini";
# //<2013/05/22-25179-EricLin, Add sign modem and SFL by S1 INT sign tool.
my $s_andro_s1 = "mediatek/external/seclib/S_ANDRO_SFL.ini.s1";
my $s_andro_org = "mediatek/external/seclib/S_ANDRO_SFL.ini.org";
# //>2013/05/22-25179-EricLin

if (-e "$and_secfl")
{
	if (-e "$s_andro_fl")
	{
		print "remove old file list (1) ... \n";
		system("rm -f $s_andro_fl");
	}
					
	system("./$sign_tool $key_dir/IMG_AUTH_KEY.ini $cfg_dir/SFL_CFG.ini $and_secfl $s_andro_fl");
	
	if (! -e "$s_andro_fl")
	{
		die "sign failed. please check";
	}
# //<2013/05/22-25179-EricLin, Add sign modem and SFL by S1 INT sign tool.
	#S1 sign tool
	print "\n\nS1 sign tool - signning SFL \n\n";
	print "./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i $s_andro_fl -o $s_andro_s1 \n\n";
	system("./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i $s_andro_fl -o $s_andro_s1");

	print "\n\nmv -f $s_andro_fl $s_andro_org \n\n";
	system("mv -f $s_andro_fl $s_andro_org") == 0 or die "can't rename sfl -> original sfl binary\n";
	print "\n\ncp -f $s_andro_s1 $s_andro_fl \n\n";
	system("cp -f $s_andro_s1 $s_andro_fl") == 0 or die "can't cp S1 sfl -> sfl binary\n";	
# //>2013/05/22-25179-EricLin
}
else
{
	print "file doesn't exist\n";	
}


##########################################################
# Sign SECRO Secure File List
##########################################################
print "\n\n*** Sign SECRO Secure File List ***\n\n";

my $secro_secfl = "mediatek/custom/$prj/security/sec_file_list/SECRO_SFL.ini";
my $s_secro_fl_o1 = "mediatek/custom/$prj/secro/S_SECRO_SFL.ini";
my $s_secro_fl_o2 = "mediatek/external/seclib/S_SECRO_SFL.ini";

if (-e "$secro_secfl")
{				
	if (-e "$s_secro_fl_o1")
	{
		print "remove old file list (1) ... \n";
		system("rm -f $s_secro_fl_o1");
	}

	if (-e "$s_secro_fl_o2")
	{
		print "remove old file list (2) ... \n";
		system("rm -f $s_secro_fl_o2");
	}

	system("./$sign_tool $key_dir/IMG_AUTH_KEY.ini $cfg_dir/SFL_CFG.ini $secro_secfl $s_secro_fl_o1");

	if (! -e "$s_secro_fl_o1")
	{
		die "sign failed. please check";
	}
	
	system("cp -f $s_secro_fl_o1 $s_secro_fl_o2");	
}
else
{
	print "file doesn't exist\n";
}

