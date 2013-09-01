#!/usr/bin/perl

##########################################################
# Initialize Variables
##########################################################
my $secro_cfg = $ARGV[0];
my $prj = $ARGV[1];
my $custom_dir = $ARGV[2];
my $secro_ac = $ARGV[3];
my $secro_out = $ARGV[4];
my $sml_dir = "mediatek/custom/$custom_dir/security/sml_auth";
my $secro_tool = "mediatek/build/tools/SecRo/SECRO_POST";
my $MTK_ENABLE_MD1 = $ENV{"MTK_ENABLE_MD1"};
my $MTK_ENABLE_MD2 = $ENV{"MTK_ENABLE_MD2"};
my $MTK_PLATFORM = $ENV{"MTK_PLATFORM"};

print " Project           =  $prj\n";
print " Custom Dir  =  $custom_dir\n";
print " MTK_ENABLE_MD1 = $MTK_ENABLE_MD1\n";
print " MTK_ENABLE_MD2 = $MTK_ENABLE_MD2\n";
print " MTK_PLATFORM = $MTK_PLATFORM\n";

##########################################################
# SecRo Post Processing
##########################################################

my $ac_region = "mediatek/custom/$custom_dir/secro/AC_REGION";
my $and_secro = "mediatek/custom/$custom_dir/secro/AND_SECURE_RO";
my $md_secro = "mediatek/custom/out/$prj/modem/SECURE_RO";
my $md2_secro = "mediatek/custom/out/$prj/modem/SECURE_RO_sys2";

if (${secro_ac} eq "yes")
{
	$md_secro = "mediatek/custom/out/$prj/modem/SECURE_RO";
	if ( ! -e $md_secro )
	{
		print "this modem does not has modem specific SECRO image, use prj SECRO\n";
		$md_secro = "mediatek/custom/$custom_dir/secro/SECURE_RO";
	}

        $md2_secro = "mediatek/custom/out/$prj/modem/SECURE_RO_sys2";
        if ( ! -e $md2_secro )
        {
                print "this modem2 does not has modem2 specific SECRO image, use prj SECRO\n";
                $md2_secro = "mediatek/custom/$custom_dir/secro/SECURE_RO_sys2";
        }
}
else
{
	$md_secro = "mediatek/custom/$custom_dir/secro/SECURE_RO";
	$md2_secro = "mediatek/custom/$custom_dir/secro/SECURE_RO_sys2";
}

##########################################################
# Check the Existence of each Region
##########################################################
if ( ! -e $ac_region )
{
	$ac_region = "mediatek/custom/common/secro/AC_REGION";
	print "does not has aggregate specific AC_REGION image, use common AC_REGION\n";
}
print " ac_region = $ac_region\n";

if ( ! -e $and_secro )
{
        $and_secro = "mediatek/custom/common/secro/AND_SECURE_RO";
	print "does not has AP specific AC_REGION_RO image, use common AC_REGION_RO\n";
}
print " and_secro = $and_secro\n";

if ( ! -e $md_secro )
{
        $md_secro = "mediatek/custom/common/secro/SECURE_RO";
	print "does not has MODEM specific SECURE_RO image, use common SECURE_RO\n";
}
print " md_secro = $md_secro\n";

if ( ! -e $md2_secro )
{
        $md2_secro = "mediatek/custom/common/secro/SECURE_RO_sys2";
	print "does not has MODEM specific SECURE_RO image, use common SECURE_RO_sys2\n";
}
print " md2_secro = $md2_secro\n";

system("chmod 777 $ac_region") == 0 or die "can't configure $ac_region as writable";
print "MTK_SEC_SECRO_AC_SUPPORT = $secro_ac\n";
if (${secro_ac} eq "yes")
{		
	system("./$secro_tool $secro_cfg $sml_dir/SML_ENCODE_KEY.ini $and_secro $md_secro $md2_secro $ac_region $secro_out") == 0 or die "SECRO POST Tool return error\n";
}
