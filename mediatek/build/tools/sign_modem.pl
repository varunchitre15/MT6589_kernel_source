#!/usr/bin/perl

##########################################################
# Initialize Variables
##########################################################

#//<20130603-25620-Eric Lin, Sign modem support live.
my $cust_key = $ARGV[0];
my $prj = $ARGV[1];
my $modem_encode = $ARGV[2];
#my $modem_encode = "no";
my $modem_auth = $ARGV[3];
my $custom_dir = $ARGV[4];
my $secro_ac = $ARGV[5];

# //<2013/04/30-24466-EricLin, SONY do not want us to AES encrypt the modem signed image.
my $modem_cipher = "no";
# //>2013/04/30-24466-EricLin

my $sml_dir = "mediatek/custom/$custom_dir/security/sml_auth";

my $cipher_tool = "mediatek/build/tools/CipherTool/CipherTool";
my $sign_tool = "mediatek/build/tools/SignTool/SignTool.sh";
my $secro_tool = "mediatek/build/tools/SecRo/SECRO_POST";

my $cust_hspa= "_hspa";
my $cust_modem= "$prj$cust_hspa";
##########################################################
# Check Parameter
##########################################################
print "cust_key = $cust_key\n";
print "prj = $prj\n";
print "modem_encode = $modem_encode\n";
print "modem_auth = $modem_auth\n";
print "custom_dir = $custom_dir\n";
print "secro_ac = $secro_ac\n";

print "cust_modem    =  $cust_modem\n";
# //>20130603-25620-Eric Lin

print "\n\n";
print "********************************************\n";
print " CHECK PARAMETER \n";
print "********************************************\n";

if (${modem_auth} eq "yes")
{
	if (${modem_encode} eq "no")
	{
		die "Error! MTK_SEC_MODEM_AUTH is 'yes' but MTK_SEC_MODEM_ENCODE is 'no'\n";
	}
}

if (${prj} eq "mt6577_evb_mt" || ${prj} eq "mt6577_phone_mt" || ${prj} eq "moto77_ics")
{
	$modem_cipher = "no"
}


print "parameter check pass (2 MDs)\n";
print "MTK_SEC_MODEM_AUTH    =  $modem_auth\n";
print "MTK_SEC_MODEM_ENCODE  =  $modem_encode\n";
print "modem_cipher  =  $modem_cipher\n";

##########################################################
# Process Modem Image
##########################################################

my $md_load = "mediatek/custom/out/$prj/modem/modem.img";
my $b_md_load = "mediatek/custom/out/$prj/modem/modem.img.bak";
my $c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem.img";
my $s_md_load = "mediatek/custom/out/$prj/modem/signed_modem.img";
&process_modem_image;

$md_load = "mediatek/custom/out/$prj/modem/modem_E1.img";
$b_md_load = "mediatek/custom/out/$prj/modem/modem_E1.img.bak";
$c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem_E1.img";
$s_md_load = "mediatek/custom/out/$prj/modem/signed_modem_E1.img";
&process_modem_image;

$md_load = "mediatek/custom/out/$prj/modem/modem_E2.img";
$b_md_load = "mediatek/custom/out/$prj/modem/modem_E2.img.bak";
$c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem_E2.img";
$s_md_load = "mediatek/custom/out/$prj/modem/signed_modem_E2.img";
&process_modem_image;

$md_load = "mediatek/custom/out/$prj/modem/modem_sys2.img";
$b_md_load = "mediatek/custom/out/$prj/modem/modem_sys2.img.bak";
$c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem_sys2.img";
$s_md_load = "mediatek/custom/out/$prj/modem/signed_modem_sys2.img";
&process_modem_image;

$md_load = "mediatek/custom/out/$prj/modem/modem_sys2_E1.img";
$b_md_load = "mediatek/custom/out/$prj/modem/modem_sys2_E1.img.bak";
$c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem_sys2_E1.img";
$s_md_load = "mediatek/custom/out/$prj/modem/signed_modem_sys2_E1.img";
&process_modem_image;

$md_load = "mediatek/custom/out/$prj/modem/modem_sys2_E2.img";
$b_md_load = "mediatek/custom/out/$prj/modem/modem_sys2_E2.img.bak";
$c_md_load = "mediatek/custom/out/$prj/modem/cipher_modem_sys2_E2.img";
$s_md_load = "mediatek/custom/out/$prj/modem/signed_modem_sys2_E2.img";
&process_modem_image;

sub process_modem_image
{
	print "\n\n";
	print "********************************************\n";
	print " PROCESS MODEM IMAGE ($md_load)\n";
	print "********************************************\n";	
#//<20130603-25620-Eric Lin, Sign modem support live.
	my $md_mtk_load = "mediatek/custom/out/$prj/modem/modem.mtkkey.img";
	my $md_target_load = "mediatek/secutool/signedmd/$cust_modem/modem.img.signed.$cust_key";
	
	# S1 sign modem 
	my $md_load_mtk = "mediatek/custom/out/$prj/modem/modem.img";
	my $md_load_org = "mediatek/custom/out/$prj/modem/modem.img.org";
	my $md_load_s1 = "mediatek/custom/out/$prj/modem/modem.img.s1";	

	
	# if (-e "$b_md_load")
	if (-e "$b_md_load" || ${cust_key} eq "KEY_TYPE_RED")
	{
		if (-e "$b_md_load")
		{
			print "$md_load already processed ... \n";
		}
		else
		{
			print "For SONY live sign, you should not re-sign modem ... \n";
		}
	}
# //>20130603-25620-Eric Lin
	else
	{
		if (-e "$md_load")
		{
			system("cp -f $md_load $b_md_load") == 0 or die "can't backup modem image";

			########################################		
			# Encrypt and Sign Modem Image
			########################################		
			if (${modem_encode} eq "yes")
			{
				if (${modem_cipher} eq "yes")
				{
					system("./$cipher_tool ENC $sml_dir/SML_ENCODE_KEY.ini $sml_dir/SML_ENCODE_CFG.ini $md_load $c_md_load") == 0 or die "Cipher Tool return error\n";
				
					if(-e "$c_md_load")
					{
						system("rm -f $md_load") == 0 or die "can't remove original modem binary\n";
						system("mv -f $c_md_load $md_load") == 0 or die "can't generate cipher modem binary\n";
					}
				}
				
				system("./$sign_tool $sml_dir/SML_AUTH_KEY.ini $sml_dir/SML_AUTH_CFG.ini $md_load $s_md_load");
	
				if(-e "$s_md_load")
				{
					system("rm -f $md_load") == 0 or die "can't remove original modem binary\n";
					system("mv -f $s_md_load $md_load") == 0 or die "can't generate signed modem binary\n";
				}		

#//<20130603-25620-Eric Lin, Sign modem support live.
				# S1 sign modem 
				# my $md_load_mtk = "mediatek/custom/out/$prj/modem/modem.img";
				# my $md_load_org = "mediatek/custom/out/$prj/modem/modem.img.org";
				# my $md_load_s1 = "mediatek/custom/out/$prj/modem/modem.img.s1";

				print "\n\nS1 sign tool - signning mdoem \n\n";
				print "./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY2 -t MTK -i $md_load_mtk -o $md_load_s1 \n\n";
				system("./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY2 -t MTK -i $md_load_mtk -o $md_load_s1");

				print "\n\n mv -f $md_load_mtk $md_load_org\n\n ";
				system("mv -f $md_load_mtk $md_load_org") == 0 or die "can't rename modem -> original modem binary\n";
				print "\n\n cp -f $md_load_s1 $md_load_mtk\n\n ";
				system("cp -f $md_load_s1 $md_load_mtk") == 0 or die "can't rename S1 modem -> modem binary\n";	
				
				system("cp -f  $md_load $md_mtk_load") ;
				system("cp -f  $md_load $md_target_load") ;
# //>20130603-25620-Eric Lin
			}
			else
			{
				print "doesn't execute Cipher Tool and Sign Tool ... \n";
			}		
		}
		else
		{
			print "$md_load is not existed\n";			
		}
	}
}

##########################################################
# Fill AC_REGION
##########################################################

print "\n\n";
print "********************************************\n";
print " Fill AC_REGION \n";
print "********************************************\n";

my $secro_def_cfg = "mediatek/custom/common/secro/SECRO_DEFAULT_LOCK_CFG.ini";
my $secro_out = "mediatek/custom/$custom_dir/secro/AC_REGION";
my $secro_script = "mediatek/build/tools/SecRo/secro_post.pl";
system("./$secro_script $secro_def_cfg $prj $custom_dir $secro_ac $secro_out") == 0 or die "SECRO post process return error\n";

##########################################################
# Process SECFL.ini
##########################################################

print "\n\n";
print "********************************************\n";
print " PROCESS SECFL.ini \n";
print "********************************************\n";

my $secfl_pl = "mediatek/build/tools/sign_sec_file_list.pl";
system("./$secfl_pl $custom_dir") == 0 or die "SECFL Perl return error\n";

#//<20130603-25620-Eric Lin, Sign modem support live.
my $s1_signed_modem = "mediatek/secutool/signedmd/$cust_modem/modem.img.signed.$cust_key";
$md_load = "mediatek/custom/out/$prj/modem/modem.img";
if (${cust_key} eq "KEY_TYPE_RED")
{
	print "[arima][mp key] Non bypass that  copy signed modem back to out folder.\n";
	if (-e "$s1_signed_modem")
	{
# //<2013/06/05-25702-EricLin, Support red/live share one image files.
		#system("cp -f $s1_signed_modem $md_load") ;
# //>2013/06/05-25702-EricLin
		print "[arima][mp key] copy done.";
	}
	else
	{
		print "[arima][mp key] open file $s1_signed_modem fail.";
		die "doesn't exist: $s1_signed_modem\n";
	}		 
}
else
{
	print "[arima][mtk key] To bypass that  copy signed modem back to out folder.\n";
}
# //>20130603-25620-Eric Lin
