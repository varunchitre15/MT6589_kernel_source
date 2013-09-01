#!/usr/local/bin/perl -w
#
#****************************************************************************/
#*
#* Filename:
#* ---------
#*   ptgen.pl
#*
#* Project:
#* --------
#*
#*
#* Description:
#* ------------
#*   This script will generate partition layout files
#*        
#*
#* Author: Kai Zhu (MTK81086)
#* -------
#*
#* Version: 3.1
#*============================================================================
#*             HISTORY
#* Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
#*------------------------------------------------------------------------------
#* $Revision$
#* $Modtime$
#* $Log$
#*
#*
#*------------------------------------------------------------------------------
#* Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
#*============================================================================
#****************************************************************************/

#****************************************************************************
# Included Modules
#****************************************************************************
use File::Basename;
my $Version=3.2;
#my $ChangeHistory="3.1 AutoDetect eMMC Chip and Set MBR_Start_Address_KB\n";
my $ChangeHistory = "3.2 Support OTP\n";
# Partition_table.xls arrays and columns
my @PARTITION_FIELD ;
my @START_FIELD_Byte ;
my @START_FIELD_Byte_HEX;
my @SIZE_FIELD_KB ;
my @TYPE_FIELD;
my @DL_FIELD ;
my @PARTITION_IDX_FIELD ;
my @REGION_FIELD ;
my @RESERVED_FIELD;
my @FB_ERASE_FIELD;
my @FB_DL_FIELD;

my $COLUMN_PARTITION                = 1 ;
my $COLUMN_TYPE                     = $COLUMN_PARTITION + 1 ;
my $COLUMN_START                    = $COLUMN_TYPE + 1 ;
my $COLUMN_END                      = $COLUMN_START + 1 ;
my $COLUMN_SIZE                     = $COLUMN_END + 1 ;
my $COLUMN_SIZEKB                   = $COLUMN_SIZE + 1 ;
my $COLUMN_SIZE2                    = $COLUMN_SIZEKB + 1 ;
my $COLUMN_SIZE3                    = $COLUMN_SIZE2 + 1 ;
my $COLUMN_DL                       = $COLUMN_SIZE3 + 1 ;
my $COLUMN_FB_ERASE                 = $COLUMN_DL + 1;  # fastboot support
my $COLUMN_FB_DL                    = $COLUMN_FB_ERASE + 1;  # fastboot support
# emmc support
my $COLUMN_PARTITION_IDX            = $COLUMN_FB_DL + 1 ;
my $COLUMN_REGION		    = $COLUMN_PARTITION_IDX + 1;
my $COLUMN_RESERVED		    = $COLUMN_REGION + 1;


my $PMT_END_NAME;  #PMT_END_NAME
#eMMC 
#EXT4 Partition Size
my $SECRO_SIZE;
my $USERDATA_SIZE;
my $SYSTEM_SIZE;
my $CACHE_SIZE;

my $total_rows = 0 ; #total_rows in partition_table
my $User_Region_Size_KB; #emmc USER region start_address
my $MBR_Start_Address_KB;	#KB
my $Page_Size	=2; # default NAND page_size of nand
my $AutoModify	=0;
my $DebugPrint    = 1; # 1 for debug; 0 for non-debug
my $LOCAL_PATH;
my $SCAT_NAME;
my $SHEET_NAME;

my %preloader_alias; #alias for preloader c and h files modify
my %uboot_alias;	#alias for uboot c and h files modify
my %kernel_alias;	#alias for kernel c  file modify



BEGIN
{
  $LOCAL_PATH = dirname($0);
}

use lib "$LOCAL_PATH/../../Spreadsheet";
use lib "$LOCAL_PATH/../../";
require 'ParseExcel.pm';


#parse argv from alps/mediatek/build/makemtk.mk

if($ARGV[0]=~/MTK_PLATFORM=(.*)/){
	$PLATFORM = $1;
	$platform = lc($PLATFORM);
}

if($ARGV[1]=~/PROJECT=(.*)/){
	$PROJECT = $1;
}

if($ARGV[2]=~/FULL_PROJECT=(.*)/){
	$FULL_PROJECT = $1;
}

if($ARGV[3]=~/MTK_LCA_SUPPORT=(.*)/){
	$LCA_PRJ = $1;
}

if($ARGV[4]=~/MTK_NAND_PAGE_SIZE=(.*)/){
	$PAGE_SIZE = $1;
}

if($ARGV[5]=~/MTK_EMMC_SUPPORT=(.*)/){
	$EMMC_SUPPORT= $1;
}

if($ARGV[6]=~/EMMC_CHIP=(.*)/){
	 $EMMC_CHIP= $1;
}

if($ARGV[7]=~/MTK_LDVT_SUPPORT=(.*)/){
	 $LDVT_SUPPORT= $1;
}

if($ARGV[8]=~/TARGET_BUILD_VARIANT=(.*)/){
	 $TARGET_BUILD_VARIANT= $1;
}

if($ARGV[9]=~/MTK_EMMC_OTP_SUPPORT=(.*)/){
	$MTK_EMMC_OTP_SUPPORT= $1;
}

my $PART_TABLE_FILENAME         = "mediatek/build/tools/ptgen/$PLATFORM/partition_table_$PLATFORM.xls"; # excel file name
my $REGION_TABLE_FILENAME 		= "mediatek/build/tools/emigen/$PLATFORM/MemoryDeviceList_$PLATFORM.xls";  #eMMC region information
my $EMMC_COMPO					= "mediatek/config/$PROJECT/eMMC_Compo.pl" ;

my $CUSTOM_MEMORYDEVICE_H_NAME  = "mediatek/custom/$PROJECT/preloader/inc/custom_MemoryDevice.h";
my $PARTITION_DEFINE_H_NAME     = "mediatek/custom/$PROJECT/common/partition_define.h"; # 
my $PARTITION_DEFINE_C_NAME		= "mediatek/platform/$platform/kernel/drivers/dum-char/partition_define.c";
my $EMMC_PART_SIZE_LOCATION		= "mediatek/config/$PROJECT/configs/EMMC_partition_size.mk" ; # store the partition size for ext4 buil

#for autogen uboot preload and kernel partition struct
my $ProjectConfig		="mediatek/config/$PROJECT/ProjectConfig.mk";
my $UbootH	="mediatek/custom/$PROJECT/uboot/inc/mt65xx_partition.h";
my $UbootC	="mediatek/custom/$PROJECT/uboot/mt65xx_partition.c";
my $PreloaderH	="mediatek/custom/$PROJECT/preloader/inc/cust_part.h";
my $PreloaderC	="mediatek/custom/$PROJECT/preloader/cust_part.c";
my $KernelH 	="mediatek/custom/$PROJECT/kernel/core/src/partition.h";
my $SCAT_NAME_DIR   = "mediatek/misc/"; # 

my $PMT_H_NAME          = "mediatek/custom/$PROJECT/common/pmt.h";
my $Uboot_PartitionC = "mediatek/custom/$PROJECT/uboot/partition.c";
my $LK_CUSTOM =  "mediatek/custom/$PROJECT/lk";
my $LK_CUSTOM_INC = "mediatek/custom/$PROJECT/lk/inc";
my $LK_MT_PartitionH = "mediatek/custom/$PROJECT/lk/inc/mt_partition.h";
my $LK_PartitionC = "mediatek/custom/$PROJECT/lk/partition.c"; 
my $LK_CUSTOM_OUT_LK = "mediatek/custom/out/$FULL_PROJECT/lk";
my $LK_CUSTOM_OUT_LK_INC = "mediatek/custom/out/$FULL_PROJECT/lk/inc";
my $CUSTOM_OUT_COMMON = "mediatek/custom/out/$FULL_PROJECT/common";

#Set SCAT_NAME
mkdir($SCAT_NAME_DIR) if (!-d $SCAT_NAME_DIR);
if ($LCA_PRJ eq "yes")
{
    $SCAT_NAME = $SCAT_NAME_DIR . $PLATFORM ."_Android_scatter_LCA.txt" ;
}

if ($EMMC_SUPPORT eq "yes") 
{
     $SCAT_NAME = $SCAT_NAME_DIR . $PLATFORM ."_Android_scatter_emmc.txt" ;
}else{
     $SCAT_NAME = $SCAT_NAME_DIR . $PLATFORM ."_Android_scatter.txt" ;
}

#Set SHEET_NAME

if($PLATFORM eq "MT6575"){
	if($EMMC_SUPPORT eq "yes"){
		$SHEET_NAME = $PLATFORM ." emmc" ;
		if($MTK_EMMC_OTP_SUPPORT eq "yes"){
			$SHEET_NAME = $SHEET_NAME ." otp" ;
		}
	}else{
		$SHEET_NAME = $PLATFORM ." nand " . $PAGE_SIZE ;
		if($PAGE_SIZE=~/(\d)K/){
			$Page_Size=$1;
		}else{
			$Page_Size=2;	
		}
		if($Page_Size == 2){
			if($TARGET_BUILD_VARIANT eq "user"){
				$SHEET_NAME = $SHEET_NAME . " user";
			}else{
				$SHEET_NAME = $SHEET_NAME . " eng";	
			}
		}	
	}

	if($LDVT_SUPPORT eq "yes"){
		$SHEET_NAME = $PLATFORM . " ldvt";	
	}
	
}

#****************************************************************************
# main thread
#****************************************************************************
# get already active Excel application or open new
print "*******************Arguments*********************\n" ;
print "Version=$Version ChangeHistory:$ChangeHistory\n";
foreach $arg (@ARGV){
	print "$arg\n";
}
print "SHEET_NAME=$SHEET_NAME\n";
print "SCAT_NAME=$SCAT_NAME\n" ;

print "*******************Arguments*********************\n\n\n\n" ;

if ($EMMC_SUPPORT eq "yes"){
	&GetMBRStartAddress();
}

$PartitonBook = Spreadsheet::ParseExcel->new()->Parse($PART_TABLE_FILENAME);

&InitAlians();

&ReadExcelFile () ;

&GenHeaderFile_new () ;

&GenScatFile () ;

&GenLK_PartitionC();
&GenLK_MT_PartitionH();
if ($EMMC_SUPPORT eq "yes"){
	&GenMBRFile ();
	&GenPartSizeFile ();
}
&GenPerloaderCust_partC();
&GenPmt_H();
&GenUboot_PartitionC();
&GenUboot_Mt65xx_ParitionH();
if($EMMC_SUPPORT ne "yes"){
	&GenKernel_PartitionC();
}
#&ModifyPreloaderCust_PartH();
#&ModifyPreloaderCust_PartC();
#&ModifyUbootMt65xx_partitionH();
#&ModifyUbootMt65xx_partitionC();

unless ($EMMC_SUPPORT eq "yes"){
	#&ModifyKernelPartitionC();
}

print "**********Ptgen Done********** ^_^\n" ;

print "\n\nPtgen modified or Generated files list:\n$SCAT_NAME\n$PARTITION_DEFINE_H_NAME\n$EMMC_PART_SIZE_LOCATION\n/out/MBR EBR1 EBR2 \n\n\n\n\n";

exit ;

sub GetMBRStartAddress(){
	my %REGION_TABLE;
	my $BOOT1;
	my $BOOT2;
	my $RPMB;
# @REGION_TABLE = 
#{ EMMC_CHIP_PART_NUM=>{
#   PART_NUM;
#	VENDER;
#	USER;
#	BOOT1;
#	BOOT2;
#	RPMB; 
#  },
#...
#]
	my $EMMC_REGION_SHEET_NAME = "emmc_region";
	my $emmc_sheet;
	my $region_name;
	my $region = 0;
	my $boot1 = 2;
	my $boot2 = 3;
	my $rpmb = 4;
	my $EMMC_RegionBook = Spreadsheet::ParseExcel->new()->Parse($REGION_TABLE_FILENAME);

	$emmc_sheet = get_sheet($EMMC_REGION_SHEET_NAME,$EMMC_RegionBook) ;
	unless ($emmc_sheet)
	{
		my $error_msg="Ptgen CAN NOT find sheet=$EMMC_REGION_SHEET_NAME in $REGION_TABLE_FILENAME\n";
		print $error_msg;
		die $error_msg;
	}

	my $row = 1;
    $region_name = &xls_cell_value($emmc_sheet, $row, $region,$EMMC_REGION_SHEET_NAME);
	while($region_name ne "END"){
		$region_name	=~ s/\s+//g;
		$BOOT1     = &xls_cell_value($emmc_sheet, $row, $boot1,$EMMC_REGION_SHEET_NAME);
		$BOOT2     = &xls_cell_value($emmc_sheet, $row, $boot2,$EMMC_REGION_SHEET_NAME);
		$RPMB   = &xls_cell_value($emmc_sheet, $row, $rpmb,$EMMC_REGION_SHEET_NAME);
		$REGION_TABLE{$region_name}	= {BOOT1=>$BOOT1,BOOT2=>$BOOT2,RPMB=>$RPMB};
		#print "In $region_name,$BOOT1,$BOOT2,$RPMB\n";
		$row++;
		$region_name = &xls_cell_value($emmc_sheet, $row, $region,$EMMC_REGION_SHEET_NAME);
	}

	if (-e $CUSTOM_MEMORYDEVICE_H_NAME) {
		`chmod 777 $CUSTOM_MEMORYDEVICE_H_NAME`;
	}
    open (CUSTOM_MEMORYDEVICE_H_NAME, "<$CUSTOM_MEMORYDEVICE_H_NAME") or &error_handler("Ptgen open CUSTOM_MEMORYDEVICE_H_NAME fail!\n", __FILE__, __LINE__);
	my @lines;
	my $iter = 0;
	my $part_num;	
	my $MAX_address = 0;
	my $combo_start_address = 0;
	my $cur=0;
	while (<CUSTOM_MEMORYDEVICE_H_NAME>) {
		my($line) = $_;
  		chomp($line);
		if ($line =~ /^#define\sCS_PART_NUMBER\[[0-9]\]/) {
#			print "$'\n";
			$lines[$iter] = $';
			$lines[$iter] =~ s/\s+//g;
			if ($lines[$iter] =~ /(.*)\/\/(.*)/) {
				$lines[$iter] =$1;
			}
			#print "$lines[$iter] \n";
			$iter ++;
		}
			
	}
	foreach $part_num (@lines) {
		if(exists $REGION_TABLE{$part_num}){
			$cur = $REGION_TABLE{$part_num}{BOOT1} + $REGION_TABLE{$part_num}{BOOT2} + $REGION_TABLE{$part_num}{RPMB};
			print "Chose region layout: $part_num, $REGION_TABLE{$part_num}{BOOT1} + $REGION_TABLE{$part_num}{BOOT2} + $REGION_TABLE{$part_num}{RPMB}=$cur\n";
			if ($cur > $MAX_address) {
				$MAX_address = $cur;
			}
		}else{
			$MAX_address = 6*1024; #default Fix me!!!
			my $error_msg="ERROR:Ptgen CAN NOT find $part_num in $REGION_TABLE_FILENAME\n";
			print $error_msg;
#			die $error_msg;
		}
	}
	print "The MAX BOOT1+BOOT2+RPMB=$MAX_address  in $CUSTOM_MEMORYDEVICE_H_NAME\n";	

	if (-e $EMMC_COMPO)
	{
		`chmod 777 $EMMC_COMPO`;
		$combo_start_address = do "$EMMC_COMPO";
		
	}else{
#		print "No $EMMC_COMPO Set MBR_Start_Address_KB=6MB\n";
#		$combo_start_address = 6*1024; #Fix Me!!!
		print "No $EMMC_COMPO\n";
	}

	if ($MAX_address < $combo_start_address) {
		$MBR_Start_Address_KB = $combo_start_address;
		print "Get MBR_Start_Address_KB from $EMMC_COMPO = $combo_start_address\n";
	}else{
		$MBR_Start_Address_KB = $MAX_address;
		print "Get MBR_Start_Address_KB from $CUSTOM_MEMORYDEVICE_H_NAME = $MAX_address\n";
	}
}

#****************************************************************************
# subroutine:  InitAlians
# return:      
#****************************************************************************
sub InitAlians(){
	$preloader_alias{"SECCFG"}="SECURE";
	$preloader_alias{"SEC_RO"}="SECSTATIC";
	$preloader_alias{"ANDROID"}="ANDSYSIMG";
	$preloader_alias{"USRDATA"}="USER";

	$uboot_alias{"DSP_BL"}="DSP_DL";
	$uboot_alias{"SECCFG"}="SECURE";
	$uboot_alias{"SEC_RO"}="SECSTATIC";
	$uboot_alias{"EXPDB"}="APANIC";
	$uboot_alias{"ANDROID"}="ANDSYSIMG";
	$uboot_alias{"USRDATA"}="USER";

	$kernel_alias{"SECCFG"}="seccnfg";
	$kernel_alias{"BOOTIMG"}="boot";
	$kernel_alias{"SEC_RO"}="secstatic";
	$kernel_alias{"ANDROID"}="system";
	$kernel_alias{"USRDATA"}="userdata";

	$lk_alias{"BOOTIMG"}="boot";
	$lk_alias{"ANDROID"}="system";
	$lk_alias{"USRDATA"}="userdata";
}

#****************************************************************************
# subroutine:  ReadExcelFile
# return:      
#****************************************************************************

sub ReadExcelFile()
{
    my $sheet = get_sheet($SHEET_NAME,$PartitonBook) ;

    unless ($sheet)
    {
		my $error_msg="Ptgen CAN NOT find sheet=$SHEET_NAME in $PART_TABLE_FILENAME\n";
		print $error_msg;
		die $error_msg;
    }
	my $row = 1 ;
    my $pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
	
	while($pt_name ne "END"){
		$PARTITION_FIELD[$row-1] = $pt_name;
		$SIZE_FIELD_KB[$row-1]    = &xls_cell_value($sheet, $row, $COLUMN_SIZEKB,$SHEET_NAME) ;
		$DL_FIELD[$row-1]        = &xls_cell_value($sheet, $row, $COLUMN_DL,$SHEET_NAME) ;
		$TYPE_FIELD[$row-1]		 = &xls_cell_value($sheet, $row, $COLUMN_TYPE,$SHEET_NAME) ;
		$FB_DL_FIELD[$row-1]    = &xls_cell_value($sheet, $row, $COLUMN_FB_DL,$SHEET_NAME) ;
 		$FB_ERASE_FIELD[$row-1]    = &xls_cell_value($sheet, $row, $COLUMN_FB_ERASE,$SHEET_NAME) ;
		if ($EMMC_SUPPORT eq "yes")
		{
            $PARTITION_IDX_FIELD[$row-1] = &xls_cell_value($sheet, $row, $COLUMN_PARTITION_IDX,$SHEET_NAME) ;
			$REGION_FIELD[$row-1]        = &xls_cell_value($sheet, $row, $COLUMN_REGION,$SHEET_NAME) ;
			$RESERVED_FIELD[$row-1]		= &xls_cell_value($sheet, $row, $COLUMN_RESERVED,$SHEET_NAME) ;
		}
		$row++;
		$pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
	}
#init start_address of partition
	$START_FIELD_Byte[0] = 0;	
	for($row=1;$row < @PARTITION_FIELD;$row++){
		if($PARTITION_FIELD[$row] eq "MBR"){
			$START_FIELD_Byte[$row] = $MBR_Start_Address_KB*1024;
			$SIZE_FIELD_KB[$row-1] = ($START_FIELD_Byte[$row] - $START_FIELD_Byte[$row-1])/1024;
			next;
		}
		if($PARTITION_FIELD[$row] eq "BMTPOOL" || $PARTITION_FIELD[$row] eq "OTP"){
			$START_FIELD_Byte[$row] = &xls_cell_value($sheet, $row+1, $COLUMN_START,$SHEET_NAME);
			next; 
		}
		
		$START_FIELD_Byte[$row] = $START_FIELD_Byte[$row-1]+$SIZE_FIELD_KB[$row-1]*1024;
		
	}
#convert dec start_address to hex start_address
	$START_FIELD_Byte_HEX[0]=0;
	for($row=1;$row < @PARTITION_FIELD;$row++){
		if($PARTITION_FIELD[$row] eq "BMTPOOL" || $PARTITION_FIELD[$row] eq "OTP"){
			$START_FIELD_Byte_HEX[$row] = $START_FIELD_Byte[$row];
		}else{
			$START_FIELD_Byte_HEX[$row] = sprintf("%x",$START_FIELD_Byte[$row]);
		}
	}
	
	if($DebugPrint eq 1){
		for($row=0;$row < @PARTITION_FIELD;$row++){
			print "START=0x$START_FIELD_Byte_HEX[$row],		Partition=$PARTITION_FIELD[$row],		SIZE=$SIZE_FIELD_KB[$row],	DL_=$DL_FIELD[$row]" ;
			if ($EMMC_SUPPORT eq "yes"){
            	print ", 	Partition_Index=$PARTITION_IDX_FIELD[$row],	REGION =$REGION_FIELD[$row]";
        	} 
			print "\n";
		}

	}

    $total_rows = @PARTITION_FIELD ;
	
	if ($total_rows == 0)
    {
        die "error in excel file no data!\n" ;
    } 
    print "There are $total_rows Partition totally!.\n" ;
}
#****************************************************************************
# subroutine:  GenHeaderFile
# return:      
#****************************************************************************
sub GenHeaderFile_new ()
{
    my $iter = 0 ;
    my $temp ;
	my $t;

	if (-e $PARTITION_DEFINE_H_NAME)
	{
		`chmod 777 $PARTITION_DEFINE_H_NAME`;
	}
    open (PARTITION_DEFINE_H_NAME, ">$PARTITION_DEFINE_H_NAME") or &error_handler("Ptgen open PARTITION_DEFINE_H_NAME fail!\n", __FILE__, __LINE__);

#write header
    print PARTITION_DEFINE_H_NAME &copyright_file_header_for_c();
    print PARTITION_DEFINE_H_NAME "\n#ifndef __PARTITION_DEFINE_H__\n#define __PARTITION_DEFINE_H__\n\n" ;
    print PARTITION_DEFINE_H_NAME "\n\n\n#define KB  (1024)\n#define MB  (1024 * KB)\n#define GB  (1024 * MB)\n\n" ;
#write part_name define
 	for ($iter=0; $iter< $total_rows; $iter++){
		$temp = "#define PART_$PARTITION_FIELD[$iter] \"$PARTITION_FIELD[$iter]\" \n";
		print PARTITION_DEFINE_H_NAME $temp ;
 	}
#preloader re-name
	print PARTITION_DEFINE_H_NAME "/*preloader re-name*/\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		if($preloader_alias{$PARTITION_FIELD[$iter]}){
			$temp = "#define PART_$preloader_alias{$PARTITION_FIELD[$iter]} \"$preloader_alias{$PARTITION_FIELD[$iter]}\" \n";
			print PARTITION_DEFINE_H_NAME $temp ;
		}
	}
#Uboot re-name
	print PARTITION_DEFINE_H_NAME "/*Uboot re-name*/\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		if($uboot_alias{$PARTITION_FIELD[$iter]}&&($uboot_alias{$PARTITION_FIELD[$iter]} ne $preloader_alias{$PARTITION_FIELD[$iter]})){
			$temp = "#define PART_$uboot_alias{$PARTITION_FIELD[$iter]} \"$uboot_alias{$PARTITION_FIELD[$iter]}\" \n";
			print PARTITION_DEFINE_H_NAME $temp ;
		}
	}
    print PARTITION_DEFINE_H_NAME "\n#define PART_FLAG_NONE              0 \n";   
    print PARTITION_DEFINE_H_NAME "#define PART_FLAG_LEFT             0x1 \n";  
    print PARTITION_DEFINE_H_NAME "#define PART_FLAG_END              0x2 \n";  
    print PARTITION_DEFINE_H_NAME "#define PART_MAGIC              0x58881688 \n\n";  
    for ($iter=0; $iter< $total_rows; $iter++)
    {
        if($PARTITION_FIELD[$iter] eq "BMTPOOL")
        {
			my $bmtpool=sprintf("%x",$SIZE_FIELD_KB[$iter]/64/$Page_Size);
			$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t(0x$bmtpool)\n" ;
    		print PARTITION_DEFINE_H_NAME $temp ;
        }else
        {
    		$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t($SIZE_FIELD_KB[$iter]*KB)\n" ;
			print PARTITION_DEFINE_H_NAME $temp ;
        }
        if($PARTITION_FIELD[$iter] eq "SECCFG" || $PARTITION_FIELD[$iter] eq "SEC_RO"){
		$temp = "#define PART_OFFSET_$PARTITION_FIELD[$iter]\t\t\t(0x$START_FIELD_Byte_HEX[$iter])\n"; 
		print PARTITION_DEFINE_H_NAME $temp ;
	}
    }
    
    print PARTITION_DEFINE_H_NAME "\n\n#define PART_NUM\t\t\t$total_rows\n\n";
    print PARTITION_DEFINE_H_NAME "\n\n#define PART_MAX_COUNT\t\t\t 40\n\n";
	print PARTITION_DEFINE_H_NAME "#define MBR_START_ADDRESS_BYTE\t\t\t($MBR_Start_Address_KB*KB)\n\n";
	if($EMMC_SUPPORT eq "yes"){
		print PARTITION_DEFINE_H_NAME "#define WRITE_SIZE_Byte		512\n";
	}else{
		print PARTITION_DEFINE_H_NAME "#define WRITE_SIZE_Byte		($Page_Size*1024)\n";
	}
	my $ExcelStruct = <<"__TEMPLATE";
typedef enum  {
	EMMC = 1,
	NAND = 2,
} dev_type;

typedef enum {
	USER = 0,
	BOOT_1,
	BOOT_2,
	RPMB,
	GP_1,
	GP_2,
	GP_3,
	GP_4,
} Region;


struct excel_info{
	char * name;
	unsigned long long size;
	unsigned long long start_address;
	dev_type type ;
	unsigned int partition_idx;
	Region region;
};
#ifdef  MTK_EMMC_SUPPORT
/*MBR or EBR struct*/
#define SLOT_PER_MBR 4
#define MBR_COUNT 8

struct MBR_EBR_struct{
	char part_name[8];
	int part_index[SLOT_PER_MBR];
};

extern struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT];
#endif
__TEMPLATE

	print PARTITION_DEFINE_H_NAME $ExcelStruct;
	print PARTITION_DEFINE_H_NAME "extern struct excel_info PartInfo[PART_NUM];\n";
	print PARTITION_DEFINE_H_NAME "\n\n#endif\n" ; 
   	close PARTITION_DEFINE_H_NAME ;

	if (-e $PARTITION_DEFINE_C_NAME)
	{
		`chmod 777 $PARTITION_DEFINE_C_NAME`;
	}
 	open (PARTITION_DEFINE_C_NAME, ">$PARTITION_DEFINE_C_NAME") or &error_handler("Ptgen open PARTITION_DEFINE_C_NAME fail!\n", __FILE__, __LINE__);	
    	print PARTITION_DEFINE_C_NAME &copyright_file_header_for_c();
	print PARTITION_DEFINE_C_NAME "#include <linux/module.h>\n";
	print PARTITION_DEFINE_C_NAME "#include \"partition_define.h\"\n";
	print PARTITION_DEFINE_C_NAME "struct excel_info PartInfo[PART_NUM]={\n";
	
	for ($iter=0; $iter<$total_rows; $iter++)
    {
    	$t = lc($PARTITION_FIELD[$iter]);
		$temp = "\t\t\t{\"$t\",";
		$t = ($SIZE_FIELD_KB[$iter])*1024;
		$temp .= "$t,0x$START_FIELD_Byte_HEX[$iter]";
		
		if($EMMC_SUPPORT eq "yes"){
			$temp .= ", EMMC, $PARTITION_IDX_FIELD[$iter],$REGION_FIELD[$iter]";
		}else{
			$temp .= ", NAND";	
		}
		$temp .= "},\n";
		print PARTITION_DEFINE_C_NAME $temp;
	}
	print PARTITION_DEFINE_C_NAME " };\n"; 
	print PARTITION_DEFINE_C_NAME "EXPORT_SYMBOL(PartInfo);\n";
#generate MBR struct
	my $G_MBR_Struct = <<"__TEMPLATE";	
#ifdef  MTK_EMMC_SUPPORT
struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT]={
		{"mbr",{1,2,3,4}},
		{"ebr1",{5,0,0,0}},
		{"ebr2",{6,0,0,0}},
};

EXPORT_SYMBOL(MBR_EBR_px);
#endif
__TEMPLATE
	print PARTITION_DEFINE_C_NAME $G_MBR_Struct;
   	close PARTITION_DEFINE_C_NAME ;

	unless(-e $CUSTOM_OUT_COMMON){
		{`mkdir -p $CUSTOM_OUT_COMMON`;}	
	}
	{`cp $PARTITION_DEFINE_H_NAME $CUSTOM_OUT_COMMON`;}

}
#****************************************************************************
# subroutine:  GenHeaderFile
# return:      
#****************************************************************************
sub GenHeaderFile ()
{
    my $iter = 0 ;
    my $temp ;
	my $t;

	if (-e $PARTITION_DEFINE_H_NAME)
	{
		`chmod 777 $PARTITION_DEFINE_H_NAME`;
	}
    open (PARTITION_DEFINE_H_NAME, ">$PARTITION_DEFINE_H_NAME") or &error_handler("Ptgen open PARTITION_DEFINE_H_NAME fail!\n", __FILE__, __LINE__);

#write header
    print PARTITION_DEFINE_H_NAME &copyright_file_header_for_c();
    print PARTITION_DEFINE_H_NAME "\n#ifndef __PARTITION_DEFINE_H__\n#define __PARTITION_DEFINE_H__\n\n" ;
    print PARTITION_DEFINE_H_NAME "\n\n\n#define KB  (1024)\n#define MB  (1024 * KB)\n#define GB  (1024 * MB)\n\n" ;

        
    for ($iter=0; $iter< $total_rows; $iter++)
    {
        if($PARTITION_FIELD[$iter] eq "BMTPOOL")
        {
			my $bmtpool=sprintf("%x",$SIZE_FIELD_KB[$iter]/64/$Page_Size);
			$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t(0x$bmtpool)\n" ;
    		print PARTITION_DEFINE_H_NAME $temp ;
        }else
        {
    		$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t($SIZE_FIELD_KB[$iter]*KB)\n" ;
			print PARTITION_DEFINE_H_NAME $temp ;
        }
        
    }
    
    print PARTITION_DEFINE_H_NAME "\n\n#define PART_NUM\t\t\t$total_rows\n\n";
	print PARTITION_DEFINE_H_NAME "#define MBR_START_ADDRESS_BYTE\t\t\t($MBR_Start_Address_KB*KB)\n\n";
	if($EMMC_SUPPORT eq "yes"){
		print PARTITION_DEFINE_H_NAME "#define WRITE_SIZE_Byte		512\n";
	}else{
		print PARTITION_DEFINE_H_NAME "#define WRITE_SIZE_Byte		($Page_Size*1024)\n";
	}
	my $ExcelStruct = <<"__TEMPLATE";
typedef enum  {
	EMMC = 1,
	NAND = 2,
} dev_type;

typedef enum {
	USER = 0,
	BOOT_1,
	BOOT_2,
	RPMB,
	GP_1,
	GP_2,
	GP_3,
	GP_4,
} Region;


struct excel_info{
	char * name;
	unsigned long long size;
	unsigned long long start_address;
	dev_type type ;
	unsigned int partition_idx;
	Region region;
};
__TEMPLATE

	print PARTITION_DEFINE_H_NAME $ExcelStruct;
	print PARTITION_DEFINE_H_NAME "extern struct excel_info PartInfo[PART_NUM];\n";
	print PARTITION_DEFINE_H_NAME "\n\n#endif\n" ; 
   	close PARTITION_DEFINE_H_NAME ;

	if (-e $PARTITION_DEFINE_C_NAME)
	{
		`chmod 777 $PARTITION_DEFINE_C_NAME`;
	}
 	open (PARTITION_DEFINE_C_NAME, ">$PARTITION_DEFINE_C_NAME") or &error_handler("Ptgen open PARTITION_DEFINE_C_NAME fail!\n", __FILE__, __LINE__);	
    	print PARTITION_DEFINE_C_NAME &copyright_file_header_for_c();
	print PARTITION_DEFINE_C_NAME "#include <linux/module.h>\n";
	print PARTITION_DEFINE_C_NAME "#include \"partition_define.h\"\n";
	print PARTITION_DEFINE_C_NAME "struct excel_info PartInfo[PART_NUM]={\n";
	
	for ($iter=0; $iter<$total_rows; $iter++)
    {
    	$t = lc($PARTITION_FIELD[$iter]);
		$temp = "\t\t\t{\"$t\",";
		$t = ($SIZE_FIELD_KB[$iter])*1024;
		$temp .= "$t,0x$START_FIELD_Byte_HEX[$iter]";
		
		if($EMMC_SUPPORT eq "yes"){
			$temp .= ", EMMC, $PARTITION_IDX_FIELD[$iter],$REGION_FIELD[$iter]";
		}else{
			$temp .= ", NAND";	
		}
		$temp .= "},\n";
		print PARTITION_DEFINE_C_NAME $temp;
	}
	print PARTITION_DEFINE_C_NAME " };\n"; 
	print PARTITION_DEFINE_C_NAME "EXPORT_SYMBOL(PartInfo);\n";
   	close PARTITION_DEFINE_C_NAME ;

}
#****************************************************************************
# subroutine:  GenScatFile
# return:      
#****************************************************************************
sub GenScatFile ()
{
    my $iter = 0 ;
	`chmod 777 $SCAT_NAME_DIR` if (-e $SCAT_NAME_DIR);
    open (SCAT_NAME, ">$SCAT_NAME") or &error_handler("Ptgen open $SCAT_NAME Fail!", __FILE__, __LINE__) ;

    for ($iter=0; $iter<$total_rows; $iter++)
    {   
		my $temp;
        if ($DL_FIELD[$iter] == 0)
        {
            $temp .= "__NODL_" ;
        }
	if($EMMC_SUPPORT eq "yes" && $RESERVED_FIELD[$iter] == 1 && $PLATFORM eq "MT6577"){
			$temp .= "RSV_";
	}
	$temp .= "$PARTITION_FIELD[$iter]" ;
	if($MTK_SHARED_SDCARD eq "yes" && $PARTITION_FIELD[$iter] =~ /USRDATA/){
		$PMT_END_NAME = "$temp";
	}elsif($PARTITION_FIELD[$iter] =~ /FAT/){
		$PMT_END_NAME = "$temp";	
	}
	$temp .= " 0x$START_FIELD_Byte_HEX[$iter]\n{\n}\n";

        print SCAT_NAME $temp ;
    }

    print SCAT_NAME "\n\n" ;
    close SCAT_NAME ;
}

#****************************************************************************************
# subroutine:  GenMBRFile 
# return:
#****************************************************************************************

sub GenMBRFile {
	#my $eMMC_size_block = $User_Region_Size_KB*1024/512;
	my $iter = 0;
# MBR & EBR table init
#	
#	MBR
#			P1: extend partition, include SECRO & SYS
#			P2:	CACHE
#			P3: DATA
#			P4: VFAT
#	EBR1
#			P5: SECRO
#	EBR2
#			P6: SYS
#
	my $mbr_start;
	my $p1_start_block;
	my $p1_size_block;
	my $p2_start_block;
	my $p2_size_block;
	my $p3_start_block;
	my $p3_size_block;
	my $p4_start_block;
	my $p4_size_block;
	my $p5_start_block;
	my $p5_size_block;
	my $p6_start_block;
	my $p6_size_block;

my @BR = (
	["mediatek/misc/MBR", [	[0x5,	0x0,0x0],
						[0x83,0x0,0x0],
						[0x83,0x0,0x0],
						[0xb,0x0,0x0]]],
	["mediatek/misc/EBR1", [[0x83,0x0,0x0],
						[0x05,0x0,0x0]]],
	["mediatek/misc/EBR2", [[0x83,0x0,0x0]]]
);

    #$sheet = get_sheet($SHEET_NAME,$PartitonBook) ;
# Fill MBR & EBR table -----------------------------------------------------
	for ($iter=0; $iter<@PARTITION_FIELD; $iter++) {   
		if($PARTITION_FIELD[$iter] eq "MBR"){
			$mbr_start = $START_FIELD_Byte[$iter];
		}
		if($PARTITION_FIELD[$iter] eq "CACHE"){
			$p2_start_block = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			#sync with img size -1MB
			$p2_size_block =  ($SIZE_FIELD_KB[$iter]-1024)*1024/512; 
		}
		if($PARTITION_FIELD[$iter] eq "USRDATA"){
			$p3_start_block = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			$p3_size_block =  ($SIZE_FIELD_KB[$iter]-1024)*1024/512;
		}
		if($PARTITION_FIELD[$iter] eq "SEC_RO"){
			$p5_start_block = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			$p5_size_block =  ($SIZE_FIELD_KB[$iter]-1024)*1024/512;
		}
		if($PARTITION_FIELD[$iter] eq "ANDROID"){
			$p6_start_block = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			$p6_size_block =  ($SIZE_FIELD_KB[$iter]-1024)*1024/512;
		}
	}
	#MBR
	print "MBR start is $mbr_start\n";
	$BR[0][1][0][1] = $p1_start_block = 0x20;
	$BR[0][1][1][1] = $p2_start_block;
	$BR[0][1][2][1] = $p3_start_block;
	$BR[0][1][3][1] = $p4_start_block = $p3_start_block + $p3_size_block+2048;#0xCEF20;
	print "P1 start is $p1_start_block\n";
	print "P2 start is $p2_start_block\n";
	print "P3 start is $p3_start_block\n";
	print "P4 start is $p4_start_block\n";
	$BR[0][1][0][2] = $p1_size_block = $p2_start_block - $p1_start_block;
	$BR[0][1][1][2] = $p2_size_block;
	$BR[0][1][2][2] = $p3_size_block;
	#$BR[0][1][3][2] = $p4_size_block = $eMMC_size_block -$p4_start_block;
	$BR[0][1][3][2] = $p4_size_block =0xffffffff;
	print "P1 size is $p1_size_block\n";
	print "P2 size is $p2_size_block\n";
	print "P3 size is $p3_size_block\n";
	print "P4 size is $p4_size_block\n";

	print "P5 start is $p5_start_block\n";
	print "P5 size is $p5_size_block\n";
	print "P6 start is $p6_start_block\n";
	print "P6 size is $p6_size_block\n";
	#EBR1
	$BR[1][1][0][1] = $p5_start_block - $p1_start_block;
	$BR[1][1][0][2] = $p5_size_block;
	$BR[1][1][1][1] = $p6_start_block - 0x20 - 0x20;
	$BR[1][1][1][2] = $p6_size_block + 0x20;

	#EBR2
	$BR[2][1][0][1] = 0x20;
	$BR[2][1][0][2] = $p6_size_block;

# Generate MBR&EBR binary file -----------------------------------------------------
foreach my $sBR (@BR){
	print("Generate $sBR->[0] bin file\n");
	
	#create file
	open(FH,">$sBR->[0]")|| die "create $sBR->[0] file failed\n";
	print FH pack("C512",0x0);

	#seek to tabel
	seek(FH,446,0);

	foreach (@{$sBR->[1]}){
		#type
		seek(FH,4,1);
		print FH pack("C1",$_->[0]);
		#offset and length
		seek(FH,3,1);
		print FH pack("I1",$_->[1]);
		print FH pack("I1",$_->[2]);
	}
	
	#end label
	seek(FH,510,0);
	print FH pack("C2",0x55,0xAA);

	close(FH);
}

}

#****************************************************************************************
# subroutine:  GenPartSizeFile;
# return:      
#****************************************************************************************

sub GenPartSizeFile
{
	`chmod 777 $EMMC_PART_SIZE_LOCATION` if (-e $EMMC_PART_SIZE_LOCATION);
        open (EMMC_PART_SIZE_LOCATION, ">$EMMC_PART_SIZE_LOCATION") or &error_handler("CAN NOT open $EMMC_PART_SIZE_LOCATION", __FILE__, __LINE__) ;
	print EMMC_PART_SIZE_LOCATION "\#!/usr/local/bin/perl\n";
	print EMMC_PART_SIZE_LOCATION &copyright_file_header_for_shell();
	print EMMC_PART_SIZE_LOCATION "\nifeq (\$(MTK_EMMC_SUPPORT),yes)\n";
	
	my $temp;
	my $index=0;
	for($index=0;$index < $total_rows;$index++){
		if($PARTITION_FIELD[$index] eq "SEC_RO")
	     {
			#1MB for sec info check in with CR ALPS00073371 
		 	$temp = $SIZE_FIELD_KB[$index]/1024-1;
			print EMMC_PART_SIZE_LOCATION "BOARD_SECROIMAGE_PARTITION_SIZE:=$temp". "M\n" ;
	     }
	     if($PARTITION_FIELD[$index] eq "ANDROID")
	     {
		 	$temp=$SIZE_FIELD_KB[$index]/1024-1;
			print EMMC_PART_SIZE_LOCATION "BOARD_SYSTEMIMAGE_PARTITION_SIZE:=$temp". "M\n" ;
	      }
	     if($PARTITION_FIELD[$index] eq "CACHE")
	     {
		 	$temp=$SIZE_FIELD_KB[$index]/1024-1;
			print EMMC_PART_SIZE_LOCATION "BOARD_CACHEIMAGE_PARTITION_SIZE:=$temp". "M\n" ;
	      }
	     if($PARTITION_FIELD[$index] eq "USRDATA")
	     {		# Reserve 1 MB for "encrypt phone" function. It needs 16k for CRYPT_FOOTER.MBR include it.
			# 1MB for sec info check in with CR ALPS00073371
		 	$temp=$SIZE_FIELD_KB[$index]/1024-1-1;
			print EMMC_PART_SIZE_LOCATION "BOARD_USERDATAIMAGE_PARTITION_SIZE:=$temp". "M\n" ;
	      }
	}

 	print EMMC_PART_SIZE_LOCATION "endif \n" ;
    close EMMC_PART_SIZE_LOCATION ;
}

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler()
{
	   my ($error_msg, $file, $line_no) = @_;
	   my $final_error_msg = "Ptgen ERROR: $error_msg at $file line $line_no\n";
	   print $final_error_msg;
	   die $final_error_msg;
}

#****************************************************************************
# subroutine:  copyright_file_header_for_c
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header_for_c()
{
    my $template = <<"__TEMPLATE";
/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
__TEMPLATE

   return $template;
}
#****************************************************************************
# subroutine:  copyright_file_header_for_shell
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header_for_shell()
{
    my $template = <<"__TEMPLATE";
 # Copyright Statement:
 #
 # This software/firmware and related documentation ("MediaTek Software") are
 # protected under relevant copyright laws. The information contained herein
 # is confidential and proprietary to MediaTek Inc. and/or its licensors.
 # Without the prior written permission of MediaTek inc. and/or its licensors,
 # any reproduction, modification, use or disclosure of MediaTek Software,
 # and information contained herein, in whole or in part, shall be strictly prohibited.
 #
 # MediaTek Inc. (C) 2010. All rights reserved.
 #
 # BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 # THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 # RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 # AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 # EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 # NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 # SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 # SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 # THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 # THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 # CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 # SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 # STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 # CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 # AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 # OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 # MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 #
 # The following software/firmware and/or related documentation ("MediaTek Software")
 # have been modified by MediaTek Inc. All revisions are subject to any receiver's
 # applicable license agreements with MediaTek Inc.
 #/
__TEMPLATE

   return $template;
}

#****************************************************************************************
# subroutine:  GenLK_PartitionC
# return:      
#****************************************************************************************
sub GenLK_PartitionC(){
	my $iter = 0;
 	my $temp;
	unless (-e $LK_CUSTOM)
		{mkdir ($LK_CUSTOM,0777) or  &error_handler("Ptgen CAN NOT create $LK_CUSTOM", __FILE__, __LINE__);}
	if (-e $LK_PartitionC)
		{`chmod 777 $LK_PartitionC`;}	
	open (SOURCE, ">$LK_PartitionC") or &error_handler("Ptgen CAN NOT open $LK_PartitionC", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();

	print SOURCE "#include \"mt_partition.h\"\n";


	print SOURCE "\npart_t partition_layout[] = {\n";

	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" ||$PARTITION_FIELD[$iter] eq "OTP");
		if($uboot_alias{$PARTITION_FIELD[$iter]}){
			$temp = "\t{PART_$uboot_alias{$PARTITION_FIELD[$iter]}, PART_BLKS_$uboot_alias{$PARTITION_FIELD[$iter]}, 0, PART_FLAG_NONE},\n";
			print SOURCE $temp;		
		}else{
			$temp = "\t{PART_$PARTITION_FIELD[$iter], PART_BLKS_$PARTITION_FIELD[$iter], PART_FLAG_NONE,0},\n";
			print SOURCE $temp;
		}
	}
	$temp = "\t{NULL, 0, PART_FLAG_END, 0},\n};";
	print SOURCE $temp;
	print SOURCE "\n\nstruct part_name_map g_part_name_map[PART_MAX_COUNT] = {\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		$temp_t = lc($TYPE_FIELD[$iter]);
		if($lk_alias{$PARTITION_FIELD[$iter]}){
			if($uboot_alias{$PARTITION_FIELD[$iter]}){
				$temp = "\t{\"$lk_alias{$PARTITION_FIELD[$iter]}\",\tPART_$uboot_alias{$PARTITION_FIELD[$iter]},\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}else{	
				$temp = "\t{\"$lk_alias{$PARTITION_FIELD[$iter]}\",\tPART_$PARTITION_FIELD[$iter],\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}
			print SOURCE $temp;		
		}else{
			$temp = lc($PARTITION_FIELD[$iter]);
			if($uboot_alias{$PARTITION_FIELD[$iter]}){
				$temp = "\t{\"$temp\",\tPART_$uboot_alias{$PARTITION_FIELD[$iter]},\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}else{	
				$temp = "\t{\"$temp\",\tPART_$PARTITION_FIELD[$iter],\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}
			
			print SOURCE $temp;
		}
	}
	print SOURCE "};\n";
	close SOURCE;
	
	unless(-e $LK_CUSTOM_OUT_LK){
		{`mkdir -p $LK_CUSTOM_OUT_LK`;}	
	}
	{`cp $LK_PartitionC $LK_CUSTOM_OUT_LK`;}
}
#****************************************************************************************
# subroutine:  GenLK_MT_ParitionH
# return:      
#****************************************************************************************
sub GenLK_MT_PartitionH(){
	my $iter = 0;
	unless (-e $LK_CUSTOM_INC)
		{mkdir ($LK_CUSTOM_INC,0777) or  &error_handler("Ptgen CAN NOT create $LK_CUSTOM_INC", __FILE__, __LINE__);}
	if (-e $LK_MT_PartitionH)
		{`chmod 777 $LK_MT_PartitionH`;}	
	open (SOURCE, ">$LK_MT_PartitionH") or &error_handler("Ptgen CAN NOT open $LK_MT_PartitionH", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();

	my $template = <<"__TEMPLATE";

#ifndef __MT_PARTITION_H__
#define __MT_PARTITION_H__


#include <platform/part.h>
#include "partition_define.h"
#include <platform/mt_typedefs.h>

#define NAND_WRITE_SIZE	 2048

#define BIMG_HEADER_SZ				(0x800)
#define MKIMG_HEADER_SZ				(0x200)

#define BLK_BITS         (9)
#define BLK_SIZE         (1 << BLK_BITS)
#ifdef MTK_EMMC_SUPPORT
#define BLK_NUM(size)    ((unsigned long long)(size) / BLK_SIZE)
#else
#define BLK_NUM(size)    ((unsigned long)(size) / BLK_SIZE)
#endif
#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

__TEMPLATE
	print SOURCE $template;
  
#uboot re-name
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		if($uboot_alias{$PARTITION_FIELD[$iter]}){
			$temp = "#define PART_BLKS_$uboot_alias{$PARTITION_FIELD[$iter]}   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print SOURCE $temp;		
		}else{
			$temp = "#define PART_BLKS_$PARTITION_FIELD[$iter]   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print SOURCE $temp;
		}
	}

	print SOURCE "\n\n#define PMT_END_NAME \"$PMT_END_NAME\"";
	print SOURCE "\n\nstruct NAND"."_CMD\{\n";
	
	$template = <<"__TEMPLATE";
	u32	u4ColAddr;
	u32 u4RowAddr;
	u32 u4OOBRowAddr;
	u8	au1OOB[64];
	u8*	pDataBuf;
};

typedef union {
    struct {    
        unsigned int magic;        /* partition magic */
        unsigned int dsize;        /* partition data size */
        char         name[32];     /* partition name */
    } info;
    unsigned char data[BLK_SIZE];
} part_hdr_t;

typedef struct {
    unsigned char *name;        /* partition name */
    unsigned long  blknum;      /* partition blks */
    unsigned long  flags;       /* partition flags */
    unsigned long  startblk;    /* partition start blk */
} part_t;

struct part_name_map{
	char fb_name[32]; 	/*partition name used by fastboot*/	
	char r_name[32];  	/*real partition name*/
	char *partition_type;	/*partition_type*/
	int partition_idx;	/*partition index*/
	int is_support_erase;	/*partition support erase in fastboot*/
	int is_support_dl;	/*partition support download in fastboot*/
};

typedef struct part_dev part_dev_t;

struct part_dev {
    int init;
    int id;
    block_dev_desc_t *blkdev;
    int (*init_dev) (int id);
#ifdef MTK_EMMC_SUPPORT
	int (*read)  (part_dev_t *dev, u64 src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, u64 dst, int size);
#else
    int (*read)  (part_dev_t *dev, ulong src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, ulong dst, int size);
#endif
};
extern struct part_name_map g_part_name_map[];
extern int mt_part_register_device(part_dev_t *dev);
extern part_t* mt_part_get_partition(char *name);
extern part_dev_t* mt_part_get_device(void);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);
extern int partition_get_index(const char * name);
extern u64 partition_get_offset(int index);
extern u64 partition_get_size(int index);
extern int partition_get_type(int index, char **p_type);
extern int partition_get_name(int index, char **p_name);
extern int is_support_erase(int index);
extern int is_support_flash(int index);
extern u64 emmc_write(u64 offset, void *data, u64 size);
extern u64 emmc_read(u64 offset, void *data, u64 size);
extern int emmc_erase(u64 offset, u64 size);
extern unsigned long partition_reserve_size(void);
#endif /* __MT_PARTITION_H__ */

__TEMPLATE
	print SOURCE $template;
	close SOURCE;

	unless(-e $LK_CUSTOM_OUT_LK_INC){
		{`mkdir -p $LK_CUSTOM_OUT_LK_INC`;}	
	}
	{`cp $LK_MT_PartitionH $LK_CUSTOM_OUT_LK_INC`;}
}
#****************************************************************************************
# subroutine:  GenPerloaderCust_partC
# return:		Gen Cust_Part.C in Preloader
# input:       no input
#****************************************************************************************
sub GenPerloaderCust_partC{
 	my $iter = 0 ;
 	my $temp;
 	
	if (-e $PreloaderC)
		{`chmod 777 $PreloaderC`;}	
	open (SOURCE, ">$PreloaderC") or &error_handler("Ptgen CAN NOT open $PreloaderC", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();
	print SOURCE "\n#include \"typedefs.h\"\n";
	print SOURCE "#include \"platform.h\"\n";
	print SOURCE "#include \"blkdev.h\"\n";
	print SOURCE "#include \"cust_part.h\"\n";
#static part_t platform_parts[PART_MAX_COUNT];
	print SOURCE "\nstatic part_t platform_parts[PART_MAX_COUNT] = {\n";

	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" ||$PARTITION_FIELD[$iter] eq "OTP" );
		if($preloader_alias{$PARTITION_FIELD[$iter]}){
			$temp = "\t{PART_$preloader_alias{$PARTITION_FIELD[$iter]}, 0, PART_SIZE_$PARTITION_FIELD[$iter], 0,PART_FLAG_NONE},\n";
			print SOURCE $temp;		
		}else{
			$temp = "\t{PART_$PARTITION_FIELD[$iter], 0, PART_SIZE_$PARTITION_FIELD[$iter], 0,PART_FLAG_NONE},\n";
			print SOURCE $temp;
		}
	}
	$temp = "\t{NULL,0,0,0,PART_FLAG_END},\n};\n\n";
	print SOURCE $temp;

#fuction

#   print SOURCE  "void cust_part_init(void){}\n\n";

 #  print SOURCE  "part_t *cust_part_tbl(void)\n";
#   print SOURCE "{\n";
 #  print SOURCE "\t return &platform_parts[0];\n";
  # print SOURCE "}\n";
	 my $template = <<"__TEMPLATE";
void cust_part_init(void){}

part_t *cust_part_tbl(void)
{
	 return &platform_parts[0];
}

__TEMPLATE
	print SOURCE $template;
	close SOURCE;
}
#****************************************************************************************
# subroutine:  GenPmt_H
# return:      
#****************************************************************************************
sub GenPmt_H(){
	if (-e $PMT_H_NAME)
	{`chmod 777 $PMT_H_NAME`;}	
	open (SOURCE, ">$PMT_H_NAME") or &error_handler("Ptgen CAN NOT open $PMT_H_NAME", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();

    my $template = <<"__TEMPLATE";

#ifndef _PMT_H
#define _PMT_H

#include "partition_define.h"

//mt6516_partition.h has defination
//mt6516_download.h define again, both is 20

#define MAX_PARTITION_NAME_LEN 64
#ifdef MTK_EMMC_SUPPORT
/*64bit*/
typedef struct
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long long size;     						/* partition size */	
    unsigned long long offset;       					/* partition start */
    unsigned long long mask_flags;       				/* partition flags */

} pt_resident;
/*32bit*/
typedef struct 
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long  size;     						/* partition size */	
    unsigned long  offset;       					/* partition start */
    unsigned long mask_flags;       				/* partition flags */

} pt_resident32;
#else

typedef struct
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long size;     						/* partition size */	
    unsigned long offset;       					/* partition start */
    unsigned long mask_flags;       				/* partition flags */

} pt_resident;
#endif


#define DM_ERR_OK 0
#define DM_ERR_NO_VALID_TABLE 9
#define DM_ERR_NO_SPACE_FOUND 10
#define ERR_NO_EXIST  1

//Sequnce number


//#define PT_LOCATION          4090      // (4096-80)
//#define MPT_LOCATION        4091            // (4096-81)
#define PT_SIG      0x50547631            //"PTv1"
#define MPT_SIG    0x4D505431           //"MPT1"
#define PT_SIG_SIZE 4
#define is_valid_mpt(buf) ((*(u32 *)(buf))==MPT_SIG)
#define is_valid_pt(buf) ((*(u32 *)(buf))==PT_SIG)
#define RETRY_TIMES 5


typedef struct _DM_PARTITION_INFO
{
    char part_name[MAX_PARTITION_NAME_LEN];             /* the name of partition */
    unsigned int start_addr;                                  /* the start address of partition */
    unsigned int part_len;                                    /* the length of partition */
    unsigned char part_visibility;                              /* part_visibility is 0: this partition is hidden and CANNOT download */
                                                        /* part_visibility is 1: this partition is visible and can download */                                            
    unsigned char dl_selected;                                  /* dl_selected is 0: this partition is NOT selected to download */
                                                        /* dl_selected is 1: this partition is selected to download */
} DM_PARTITION_INFO;

typedef struct {
    unsigned int pattern;
    unsigned int part_num;                              /* The actual number of partitions */
    DM_PARTITION_INFO part_info[PART_MAX_COUNT];
} DM_PARTITION_INFO_PACKET;

typedef struct {
	int sequencenumber:8;
	int tool_or_sd_update:8;
	int mirror_pt_dl:4;   //mirror download OK
	int mirror_pt_has_space:4;
	int pt_changed:4;
	int pt_has_space:4;
} pt_info;

#endif
    
__TEMPLATE
	print SOURCE $template;
	close SOURCE;
	
	unless(-e $CUSTOM_OUT_COMMON){
		{`mkdir -p $CUSTOM_OUT_COMMON`;}	
	}
	{`cp $PMT_H_NAME $CUSTOM_OUT_COMMON`;}
}
#****************************************************************************************
# subroutine:  GenUboot_PartitionC
# return:      
#****************************************************************************************
sub GenUboot_PartitionC(){
	my $iter = 0;
 	my $temp;
 	
	if (-e $Uboot_PartitionC)
		{`chmod 777 $Uboot_PartitionC`;}	
	open (SOURCE, ">$Uboot_PartitionC") or &error_handler("Ptgen CAN NOT open $Uboot_PartitionC", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();

	print SOURCE "#include <common.h>\n";
	print SOURCE "#include \"mt65xx_partition.h\"\n";

	#if($PLATFORM eq "MT6575"){
	#	$temp = lc($PLATFORM);
	#	print SOURCE "\npart_t $temp"."_parts[] = {\n";
	#}else{
		print SOURCE "\npart_t partition_layout[] = {\n";
	#}
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" ||$PARTITION_FIELD[$iter] eq "OTP");
		if($uboot_alias{$PARTITION_FIELD[$iter]}){
			$temp = "\t{PART_$uboot_alias{$PARTITION_FIELD[$iter]}, PART_BLKS_$uboot_alias{$PARTITION_FIELD[$iter]}, 0, PART_FLAG_NONE},\n";
			print SOURCE $temp;		
		}else{
			$temp = "\t{PART_$PARTITION_FIELD[$iter], PART_BLKS_$PARTITION_FIELD[$iter], PART_FLAG_NONE,0},\n";
			print SOURCE $temp;
		}
	}
	$temp = "\t{NULL, 0, PART_FLAG_END, 0},\n};";
	print SOURCE $temp;

	close SOURCE;
	
}
#****************************************************************************************
# subroutine:  GenUboot_Mt65xx_ParitionH
# return:      
#****************************************************************************************
sub GenUboot_Mt65xx_ParitionH(){
	my $iter = 0;
	
	if (-e $UbootH)
		{`chmod 777 $UbootH`;}	
	open (SOURCE, ">$UbootH") or &error_handler("Ptgen CAN NOT open $UbootH", __FILE__, __LINE__);
	print SOURCE &copyright_file_header_for_c();

	my $template = <<"__TEMPLATE";

#ifndef __MT65XX_PARTITION_H__
#define __MT65XX_PARTITION_H__


#include <part.h>
#include "partition_define.h"

#define NAND_WRITE_SIZE	 2048

#define BLK_BITS         (9)
#define BLK_SIZE         (1 << BLK_BITS)
#ifdef MTK_EMMC_SUPPORT
#define BLK_NUM(size)    ((unsigned long long)(size) / BLK_SIZE)
#else
#define BLK_NUM(size)    ((unsigned long)(size) / BLK_SIZE)
#endif
#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

__TEMPLATE
	print SOURCE $template;
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		if($uboot_alias{$PARTITION_FIELD[$iter]}){
			$temp = "#define PART_BLKS_$uboot_alias{$PARTITION_FIELD[$iter]}   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print SOURCE $temp;		
		}else{
			$temp = "#define PART_BLKS_$PARTITION_FIELD[$iter]   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print SOURCE $temp;
		}
	}

	$temp = lc($PLATFORM);
	print SOURCE "\n\nstruct NAND_CMD\{\n";
	
	$template = <<"__TEMPLATE";
	u32	u4ColAddr;
	u32 u4RowAddr;
	u32 u4OOBRowAddr;
	u8	au1OOB[64];
	u8*	pDataBuf;
};

typedef union {
    struct {    
        unsigned int magic;        /* partition magic */
        unsigned int dsize;        /* partition data size */
        char         name[32];     /* partition name */
    } info;
    unsigned char data[BLK_SIZE];
} part_hdr_t;

typedef struct {
    unsigned char *name;        /* partition name */
    unsigned long  blknum;      /* partition blks */
    unsigned long  flags;       /* partition flags */
    unsigned long  startblk;    /* partition start blk */
} part_t;


typedef struct part_dev part_dev_t;

struct part_dev {
    int init;
    int id;
    block_dev_desc_t *blkdev;
    int (*init_dev) (int id);
#ifdef MTK_EMMC_SUPPORT
	int (*read)  (part_dev_t *dev, u64 src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, u64 dst, int size);
#else
    int (*read)  (part_dev_t *dev, ulong src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, ulong dst, int size);
#endif
};

extern int mt_part_register_device(part_dev_t *dev);
extern part_t* mt_part_get_partition(char *name);
extern part_dev_t* mt_part_get_device(void);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);

#endif /* __MT65XX_PARTITION_H__ */

__TEMPLATE
	print SOURCE $template;
	close SOURCE;
}

#****************************************************************************************
# subroutine:  GenKernel_PartitionC
# return:      
#****************************************************************************************
sub GenKernel_PartitionC(){
	my $iter = 0;
 	my $temp;
 	
	if (-e $KernelH)
		{`chmod 777 $KernelH`;}	
	open (SOURCE, ">$KernelH") or &error_handler("Ptgen CAN NOT open $KernelH", __FILE__, __LINE__);

	print SOURCE &copyright_file_header_for_c();
	my $template = <<"__TEMPLATE";

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include "partition_define.h"


/*=======================================================================*/
/* NAND PARTITION Mapping                                                  */
/*=======================================================================*/
static struct mtd_partition g_pasStatic_Partition[] = {

__TEMPLATE
	print SOURCE $template;
	for ($iter=0; $iter< $total_rows; $iter++){
		
		$temp = lc($PARTITION_FIELD[$iter]);
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		print SOURCE "\t{\n";
		if($kernel_alias{$PARTITION_FIELD[$iter]}){
			print SOURCE "\t\t.name = \"$kernel_alias{$PARTITION_FIELD[$iter]}\",\n";
		}
		else{
			print SOURCE "\t\t.name = \"$temp\",\n";
		}
		if($iter == 0){
			print SOURCE "\t\t.offset = 0x0,\n";
		}else{
			print SOURCE "\t\t.offset = MTDPART_OFS_APPEND,\n";
		}
		if($PARTITION_FIELD[$iter] ne "USRDATA"){
			print SOURCE "\t\t.size = PART_SIZE_$PARTITION_FIELD[$iter],\n";
		}else{
			print SOURCE "\t\t.size = MTDPART_SIZ_FULL,\n";
		}
		if($PARTITION_FIELD[$iter] eq "PRELOADER" ||$PARTITION_FIELD[$iter] eq "DSP_BL" ||$PARTITION_FIELD[$iter] eq "UBOOT" || $PARTITION_FIELD[$iter] eq "SEC_RO"){
			print SOURCE "\t\t.mask_flags  = MTD_WRITEABLE,\n";
		}
		print SOURCE "\t},\n";
	}
	print SOURCE "};\n";

	$template = <<"__TEMPLATE";
#define NUM_PARTITIONS ARRAY_SIZE(g_pasStatic_Partition)
extern int part_num;	// = NUM_PARTITIONS;
__TEMPLATE
	print SOURCE $template;
	close SOURCE;


}
#****************************************************************************************
# subroutine:  ModifyPreloaderCust_PartH
# return:		Modify Cust_Part.H in Preloader
# input:       no input
#****************************************************************************************
sub ModifyPreloaderCust_PartH {
	if (-e $PreloaderH)
	{`chmod 777 $PreloaderH`;}	
	open (SOURCE, "$PreloaderH") or &error_handler("Ptgen CAN NOT open $PreloaderH", __FILE__, __LINE__);
	
	my $index=0;
	my $iter=0;
	my $findpt=0;
	my $NeedAdd=0;	
	my @arry=<SOURCE>;
	my $partalians;
	my @AddPart;
	foreach $part (@PARTITION_FIELD){
		if($part eq "END" || $part eq "BMTPOOL" || $part eq "OTP"){
			next;
		}
		if($preloader_alias{$part}){
			$partalians = $preloader_alias{$part};
		}else{
			$partalians = $part;
		}
		$index=0;
		while($index < @arry){
			if($arry[$index]=~/#define.*PART_$partalians/){
				$foundpt=1;
				last;
			}
			$index++;
		}
		
		if($foundpt eq 0){
			$AddPart[$iter]=$part;
			$NeedAdd=1;
			$iter++;
		}
		$foundpt=0;
	}
	
	if($NeedAdd == 1){
		$PreloaderHBUF=$PreloaderH.".tmp";
		open (BUF,">$PreloaderHBUF");
		$index=0;
		while($index < @arry){
			print BUF $arry[$index];
			if($arry[$index]=~/#define.*CUST_PART_H/){
				my $i=0;
				print BUF "/*autogen by Ptgen(Kai.Zhu mtk81086)*/\n";
				while($i < @AddPart){
					print BUF "#define PART_$AddPart[$i]			\"$AddPart[$i]\" \n";
					print BUF "#define CFG_USE_$AddPart[$i]_PARTITION	\n";
					$i++;
				}
				print BUF "/*autogen by Ptgen(Kai.Zhu mtk81086)*/\n";
			}
		$index++;
		}
		print "$PreloaderH has been modified\n";
		close SOURCE;
		close BUF;
		rename($PreloaderHBUF,$PreloaderH) or &error_handler("CAN NOT rename $PreloaderHBUF\n",__FILE__, __LINE__);
	}else{
		close SOURCE;
	}
	
}

#****************************************************************************************
# subroutine:  ModifyPreloaderCust_PartC
# return:		Modify Cust_Part.C in Preloader
# input:       no input
#****************************************************************************************
sub ModifyPreloaderCust_PartC {
	if (-e $PreloaderC)
	{`chmod 777 $PreloaderC`;}	
	open (SOURCE, "$PreloaderC") or &error_handler("Ptgen CAN NOT open $PreloaderC", __FILE__, __LINE__);
	
	my $new=$PreloaderC.".tmp";
	open (NEW,">$new");

	my $index=0;
	my $start_replace=0;
	my $end_replace=0;
	my $br=0;
	my $matchBr=0;
	my $findbr=0;	
	my @arry=<SOURCE>;
	my $i;

	for($index=0;$index < @arry;$index++){
		#need replacement if match
		if($arry[$index]=~/void.*cust_part_init.*(.*void.*)/)
		{
			$start_replace = $index;
		}
		if($start_replace ne 0){
		#if number of { eq number of },stop the replacement
			my @leftbr;
			my @rightbr;
			@leftbr = $arry[$index] =~/\{/g ;
			$br += @leftbr;
			$matchBr +=@leftbr;
			@rightbr = $arry[$index] =~/\}/g ;
			$br += @rightbr;
			$matchBr -=@rightbr;
			if($br ne 0 && $matchBr eq 0){
				$end_replace = $index;
				last;	
			}
		}
	}
	
#write tmp file
	for($index=0;$index < @arry;$index++){
		if($index == $start_replace){
			print NEW "void cust_part_init(void)
{
	u32 index = 0;
    memset(platform_parts, 0, sizeof(part_t) * PART_MAX_COUNT);\n";
			for($i=0;$i < @PARTITION_FIELD;$i++){
				if($PARTITION_FIELD[$i] eq "BMTPOOL" || $PARTITION_FIELD[$i] eq "OTP"){
					next;
				}
				print NEW "	{\n";
				if($preloader_alias{$PARTITION_FIELD[$i]}){
					print NEW "		platform_parts[index].name = PART_$preloader_alias{$PARTITION_FIELD[$i]};\n"
				}else{
					print NEW "		platform_parts[index].name = PART_$PARTITION_FIELD[$i];\n";
				}
				
				print NEW "		platform_parts[index].size = PART_SIZE_$PARTITION_FIELD[$i];\n";
				print NEW "		platform_parts[index].flags = PART_FLAG_NONE;\n";
				
				if($i == 0){
					print NEW "		platform_parts[index].startblk = 0;\n";
				}
				
				print NEW "		index++;\n";
				print NEW "	}\n";
			}
			
			print NEW "
\#ifdef CFG_USE_DSP_ROM_PARTITION    
    {
        platform_parts[index].name = PART_DSP_ROM;
        platform_parts[index].size = 1 * MB;
        platform_parts[index].flags = PART_FLAG_NONE;
        index++;
    }
\#endif

\#ifdef CFG_USE_AP_ROM_PARTITION
    {
        platform_parts[index].name = PART_AP_ROM;
        platform_parts[index].size = 50 * MB;
        platform_parts[index].flags = PART_FLAG_NONE;
        index++;
    }
\#endif

\#ifdef CFG_USE_MD_ROM_PARTITION
    {
        platform_parts[index].name = PART_MD_ROM;
        platform_parts[index].size = 8 * MB;
        platform_parts[index].flags = PART_FLAG_NONE;
        index++;
    }
\#endif

    {
        platform_parts[index].name = NULL;
        platform_parts[index].size = 0;
        platform_parts[index].flags = PART_FLAG_END;
        index++;
    }\n";
		
			print NEW "}\n";
			
			$index=$end_replace;
		}else{
			print NEW $arry[$index];
		}
			
	}

	close SOURCE;
	close NEW;
	rename($new,$PreloaderC) or &error_handler("CAN NOT rename $new\n",__FILE__, __LINE__);
}
#****************************************************************************************
# subroutine:  ModifyUbootMt65xx_partitionH
# return:      
#****************************************************************************************
sub ModifyUbootMt65xx_partitionH()
{
	if (-e $UbootH)
	{`chmod 777 $UbootH`;}	
	open (SOURCE, "$UbootH") or &error_handler("Ptgen CAN NOT open $UbootH", __FILE__, __LINE__);
	
	my $index=0;
	my $iter=0;
	my $findpt=0;
	my $NeedAdd=0;	
	my @arry=<SOURCE>;
	my $partalians;
	my @AddPart;
	foreach $part (@PARTITION_FIELD){
		if($part eq "END" || $part eq "BMTPOOL" || $part eq "OTP"){
			next;
		}
		if($uboot_alias{$part}){
			$partalians = $uboot_alias{$part};
		}else{
			$partalians = $part;
		}
		$index=0;
		while($index < @arry){
			if($arry[$index]=~/#define.*PART_$partalians/){
				$foundpt=1;
				last;
			}
			$index++;
		}
		
		if($foundpt eq 0){
			print "$part not find\n";
			$AddPart[$iter]=$part;
			$NeedAdd=1;
			$iter++;
		}
		$foundpt=0;
	}
	
	if($NeedAdd == 1){
		$UbootHTmp=$UbootH.".tmp";
		open (BUF,">$UbootHTmp");
		$index=0;
		while($index < @arry){
			print BUF $arry[$index];
			if($arry[$index]=~/#define.*__MT65XX_PARTITION_H__/){
				my $i=0;
				print BUF "/*autogen by Ptgen(Kai.Zhu mtk81086)*/\n";
				while($i < @AddPart){
					print BUF "#define PART_$AddPart[$i]			\"$AddPart[$i]\"\n";
					print BUF "#define PART_BLKS_$AddPart[$i]		\"BLK_NUM(PART_SIZE_$AddPart[$i])\"\n";
					$i++;
				}
				print BUF "/*autogen by Ptgen(Kai.Zhu mtk81086)*/\n";
			}
		$index++;
		}
		print " $UbootH has been modified\n";
		close SOURCE;
		close BUF;
		rename($UbootHTmp,$UbootH) or &error_handler("CAN NOT rename $UbootHTmp\n",__FILE__, __LINE__);
	}else{
		close SOURCE;
	}
	
}
#****************************************************************************************
# subroutine:  ModifyUbootMt65xx_partitionC
# return:      
#****************************************************************************************
sub ModifyUbootMt65xx_partitionC(){
	if (-e $UbootC)
	{`chmod 777 $UbootC`;}	
	open (SOURCE, "$UbootC") or &error_handler("Ptgen CAN NOT open $UbootC", __FILE__, __LINE__);
	
	my $new=$UbootC.".tmp";
	open (NEW,">$new");

	my $index=0;
	my $start_replace=0;
	my $end_replace=0;
	my $br=0;
	my $matchBr=0;
	my $findbr=0;	
	my @arry=<SOURCE>;
	my $i;

	for($index=0;$index < @arry;$index++){
		#need replacement if match
		if($PLATFORM eq "MT6575"){
			if($arry[$index]=~/part_t.*mt6575_parts\[.*\].*=/){
				$start_replace = $index;
			}
		}elsif($PLATFORM eq "MT6577"){
			if($arry[$index]=~/part_t.*partition_layout\[.*\].*=/){
				$start_replace = $index;
			}
		}
		if($start_replace ne 0){
		#if number of { eq number of },stop the replacement
			my @leftbr;
			my @rightbr;
			@leftbr = $arry[$index] =~/\{/g ;
			$br += @leftbr;
			$matchBr +=@leftbr;
			@rightbr = $arry[$index] =~/\}/g ;
			$br += @rightbr;
			$matchBr -=@rightbr;
			if($br ne 0 && $matchBr eq 0){
				$end_replace = $index;
				last;	
			}
		}
	}
	

#write tmp file
	for($index=0;$index < @arry;$index++){
		if($index == $start_replace){
			if($PLATFORM eq "MT6575"){
				print NEW "part_t mt6575_parts[] = {\n";						
			}elsif($PLATFORM eq "MT6577"){
				print NEW "part_t partition_layout[] = {\n";
			}
			for($i=0;$i < @PARTITION_FIELD;$i++){
				if($PARTITION_FIELD[$i] eq "BMTPOOL" || $PARTITION_FIELD[$i] eq "OTP"){
					next;
				}
				print NEW "	{\n";
				if($uboot_alias{$PARTITION_FIELD[$i]}){
					print NEW "		.name   = PART_$uboot_alias{$PARTITION_FIELD[$i]},\n"
				}else{
					print NEW "		.name   = PART_$PARTITION_FIELD[$i],\n";
				}
				print NEW "		.blknum = BLK_NUM(PART_SIZE_$PARTITION_FIELD[$i]),\n";
				print NEW "		.flags  = PART_FLAG_NONE,\n";
				
				if($i == 0){
					print NEW "		.startblk = 0x0,\n";
				}
				
				print NEW "	},\n";
			}
			
			print NEW "
    {
        .name   = NULL,
        .flags  = PART_FLAG_END,
    },\n";
		
			print NEW "};\n";
			
			$index=$end_replace;
		}else{
			print NEW $arry[$index];
		}
			
	}

	close SOURCE;
	close NEW;
	rename($new,$UbootC) or &error_handler("CAN NOT rename $new\n",__FILE__, __LINE__);
}
#****************************************************************************************
# subroutine:  ModifyKernelPartitionC
# return:      
#****************************************************************************************
sub ModifyKernelPartitionC {
		if (-e $KernelH)
	{`chmod 777 $KernelH`;}	
	open (SOURCE, "$KernelH") or &error_handler("Ptgen CAN NOT open $KernelH", __FILE__, __LINE__);
	
	my $new=$KernelH.".tmp";
	open (NEW,">$new");

	my $index=0;
	my $start_replace=0;
	my $end_replace=0;
	my $br=0;
	my $matchBr=0;
	my $findbr=0;	
	my @arry=<SOURCE>;
	my $i;

	for($index=0;$index < @arry;$index++){
		#need replacement if match
		if($arry[$index]=~/static.*struct.*mtd_partition.*g_pasStatic_Partition.*\[.*\].*=.*{/)
		{
			$start_replace = $index;
		}
		if($start_replace ne 0){
		#if number of { eq number of },stop the replacement
			my @leftbr;
			my @rightbr;
			@leftbr = $arry[$index] =~/\{/g ;
			$br += @leftbr;
			$matchBr +=@leftbr;
			@rightbr = $arry[$index] =~/\}/g ;
			$br += @rightbr;
			$matchBr -=@rightbr;
			if($br ne 0 && $matchBr eq 0){
				$end_replace = $index;
				last;	
			}
		}
	}
	
#write tmp file
	for($index=0;$index < @arry;$index++){
		if($index == $start_replace){
			print NEW "static struct mtd_partition g_pasStatic_Partition[] = {\n";
			for($i=0;$i < @PARTITION_FIELD;$i++){
				if($PARTITION_FIELD[$i] eq "BMTPOOL" || $PARTITION_FIELD[$i] eq "OTP"){
					next;
				}
				print NEW "	{\n";
				if($kernel_alias{$PARTITION_FIELD[$i]}){
					print NEW "		.name   = \"$kernel_alias{$PARTITION_FIELD[$i]}\",\n"
				}else{
					my $t=lc($PARTITION_FIELD[$i]);
					print NEW "		.name   = \"$t\",\n";
				}
				if($i == 0){
					print NEW "		.offset = 0x0,\n";
				}else{
					print NEW "		.offset = MTDPART_OFS_APPEND,\n"
				}
				if($PARTITION_FIELD[$i] ne "USRDATA"){
					print NEW "		.size = PART_SIZE_$PARTITION_FIELD[$i],\n";
				}else{
					print NEW "		.size = MTDPART_SIZ_FULL,\n";
				}
				if($PARTITION_FIELD[$i] eq "PRELOADER" ||$PARTITION_FIELD[$i] eq "DSP_BL" ||$PARTITION_FIELD[$i] eq "UBOOT" ||$PARTITION_FIELD[$i] eq "SEC_RO"){
					print NEW "		.mask_flags  = MTD_WRITEABLE,\n";
				}
				print NEW "	},\n";
			}
			print NEW "};\n";
			$index=$end_replace;
		}else{
			print NEW $arry[$index];
		}
			
	}

	close SOURCE;
	close NEW;
	rename($new,$KernelH) or &error_handler("CAN NOT rename $new\n",__FILE__, __LINE__);
}
#****************************************************************************************
# subroutine:  get_sheet
# return:      Excel worksheet no matter it's in merge area or not, and in windows or not
# input:       Specified Excel Sheetname
#****************************************************************************************
sub get_sheet {
  my ($sheetName,$Book) = @_;
  return $Book->Worksheet($sheetName);
}


#****************************************************************************************
# subroutine:  xls_cell_value
# return:      Excel cell value no matter it's in merge area or not, and in windows or not
# input:       $Sheet:  Specified Excel Sheet
# input:       $row:    Specified row number
# input:       $col:    Specified column number
#****************************************************************************************
sub xls_cell_value {
	my ($Sheet, $row, $col,$SheetName) = @_;
	my $cell = $Sheet->get_cell($row, $col);
	if(defined $cell){
		return  $cell->Value();
  	}else{
		my $error_msg="ERROR in ptgen.pl: (row=$row,col=$col) undefine in $SheetName!\n";
		print $error_msg;
		die $error_msg;
	}
}
