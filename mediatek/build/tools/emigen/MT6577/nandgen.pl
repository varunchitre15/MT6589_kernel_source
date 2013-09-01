#!/usr/local/bin/perl -w

use File::Basename;
#use strict;
my $LOCAL_PATH;
BEGIN
{
    $LOCAL_PATH = dirname($0);
}

use lib "$LOCAL_PATH/../../Spreadsheet";
use lib "$LOCAL_PATH/../../";
require 'ParseExcel.pm';

#****************************************************************************
# Customization Field
#****************************************************************************
my $DebugPrint = "yes";
my $VersionAndChanglist = "2.0 support autodetect NAND ID and improvement\n";
my $PLATFORM = $ENV{MTK_PLATFORM};# MTxxxx
my $PROJECT = $ENV{PROJECT};
my $FULL_PROJECT = $ENV{FULL_PROJECT};
my $PAGE_SIZE = $ENV{MTK_NAND_PAGE_SIZE};
my @MemoryDeviceList;
print "\$PLATFORM=$PLATFORM,\$PROJECT=$PROJECT,\$FULL_PROJECT=$FULL_PROJECT,\$PAGE_SIZE=$PAGE_SIZE\n";
#****************************************************************************
# Main Thread Field
#****************************************************************************
    &ReadNANDExcelFile();
    &GenNANDHeaderFile();
    print "nandgen done\n";
    exit 0;


#****************************************************************************
# Subfunction Field
#****************************************************************************
sub ReadNANDExcelFile
{	my @all_column=[];#=qw(Vendor Part_Number Nand_ID AddrCycle IOWidth TotalSize_MB BlockSize_KB PageSize_B SpareSize_B Timing  CacheRead RandomRead);
    my $MEMORY_DEVICE_LIST_XLS = "mediatek/build/tools/emigen/${PLATFORM}/MemoryDeviceList_${PLATFORM}.xls";
    my $SheetName = "NAND";
    my $parser = Spreadsheet::ParseExcel->new();
    my $Book = $parser->Parse($MEMORY_DEVICE_LIST_XLS);
    my $sheet = $Book->Worksheet($SheetName);
    my %COLUMN_LIST;
    my $tmp;
    my $row;
    my $col;
    for($col = 0, $row = 0,$tmp = &xls_cell_value($sheet, $row, $col); $tmp; $col++, $tmp = &xls_cell_value($sheet, $row, $col))
    {
        $COLUMN_LIST{$tmp} = $col;
	}
	@all_column=sort (keys(%COLUMN_LIST));
	print "@all_column\n";
	
	for($row = 1,$tmp = &xls_cell_value($sheet, $row, $COLUMN_LIST{Part_Number});$tmp;$row++,$tmp = &xls_cell_value($sheet, $row, $COLUMN_LIST{Part_Number}))
	{
		foreach $i (@all_column){
			$MemoryDeviceList[$row-1]{$i}=&xls_cell_value($sheet, $row, $COLUMN_LIST{$i});
		}
	}

	if($DebugPrint eq "yes"){
		print "~~~~~~~~~EXCEL INFO~~~~~~~~~~~\n";
		for($index=0;$index<@MemoryDeviceList;$index++){
			print "index:$index\n";
			foreach $i (@all_column){
				printf ("%-15s:%-20s ",$i,$MemoryDeviceList[$index]->{$i});
			}
			print "\n";
		}
		print "~~~~~~~~~There are $index Nand Chips~~~~~~~~~~~\n";
	}
}

sub GenNANDHeaderFile()
{

    my $NAND_LIST_DEFINE_H_NAME = "mediatek/custom/$PROJECT/common/nand_device_list.h";
    my $CUSTOM_OUT_COMMON = "mediatek/custom/out/$FULL_PROJECT/common";
    my $CUSTOM_COMMON = "mediatek/custom/$PROJECT/common";
	my %InFileChip;
	my $Longest_ID=0;
	my $Chip_count=0;
    if(!-e $CUSTOM_COMMON)
    {
	    `mkdir -p $CUSTOM_COMMON `;
    }
	if(-e $NAND_LIST_DEFINE_H_NAME)
	{
		`chmod 777 $NAND_LIST_DEFINE_H_NAME`;
	}

	for($iter=0;$iter<@MemoryDeviceList;$iter++){
		if(($PAGE_SIZE eq "4K" && $MemoryDeviceList[$iter]->{PageSize_B} eq 4096) || ($PAGE_SIZE eq "2K" && $MemoryDeviceList[$iter]->{PageSize_B} eq 2048))
		{
			my $ID_length=0;
			my $advance_option=0;
			my $ID=$MemoryDeviceList[$iter]->{Nand_ID};
			if(!exists($InFileChip{$ID})){
				if(length($ID)%2){
					print "The chip:$ID have wrong number!\n";
				}else{	
					$ID_length=length($ID)/2-1;
					if($ID_length > $Longest_ID){
						$Longest_ID = $ID_length;
					}
					#print "\$Longest_ID=$Longest_ID\n";
					if ($MemoryDeviceList[$iter]->{CacheRead} eq "YES")
					{
						$advance_option += 2;
					}
					if ($MemoryDeviceList[$iter]->{RandomRead} eq "YES")
					{
						$advance_option += 1;
					}
					$InFileChip{$ID}={'Index'=>$iter,'IDLength'=>$ID_length,'ADV_OPT'=>$advance_option};
					$Chip_count++;
				}
			}else{
				print "There more than 1 chip have the ID:$MemoryDeviceList[$iter]->{Nand_ID},you should modify the excel\n";
			}
		}
		
	}
    open(FD, ">$NAND_LIST_DEFINE_H_NAME") or &error_handler("open $NAND_LIST_DEFINE_H_NAME fail\n", __FILE__, __LINE__);
    print FD "\n#ifndef __NAND_DEVICE_LIST_H__\n#define __NAND_DEVICE_LIST_H__\n\n";
	print FD "#define NAND_MAX_ID\t\t$Longest_ID\n";
	print FD "#define CHIP_CNT\t\t$Chip_count\n";
	print FD &struct_flashdev_info_define();
	print FD "static const flashdev_info gen_FlashTable[]={\n";
	foreach $ID (sort by_length (keys(%InFileChip))){
		my $it=$InFileChip{$ID}->{Index};
		#creat ID arry string
		my @ID_arry=($ID =~ m/([\dA-Fa-f]{2})/gs);
		my $arry_str="{";
		for($i=0;$i<$Longest_ID;$i++){
			if($i<@ID_arry){
				$arry_str.="0x$ID_arry[$i]";
			}else{
				$arry_str.="0x00";
			}
			if($i<$Longest_ID-1){
				$arry_str.=",";
			}
		}
		$arry_str.="}";
		print "ID=$arry_str\n";
		#print string to file
		print FD "\t{$arry_str, $InFileChip{$ID}->{IDLength},$MemoryDeviceList[$it]->{AddrCycle}, $MemoryDeviceList[$it]->{IOWidth},$MemoryDeviceList[$it]->{TotalSize_MB},$MemoryDeviceList[$it]->{BlockSize_KB},$MemoryDeviceList[$it]->{PageSize_B},$MemoryDeviceList[$it]->{SpareSize_B},$MemoryDeviceList[$it]->{Timing},";
		printf FD ("\"%.13s\",%d}, \n",$MemoryDeviceList[$it]->{Part_Number},$InFileChip{$ID}->{ADV_OPT});
	}

    print FD "};\n\n";
	print FD "#endif\n";
    close FD;

    if(!-e $CUSTOM_OUT_COMMON)
    {
	    `mkdir -p $CUSTOM_OUT_COMMON `;
		`rm $CUSTOM_OUT_COMMON/nand_device_list.h`;
        `cp $NAND_LIST_DEFINE_H_NAME $CUSTOM_OUT_COMMON `;
    }
}

#****************************************************************************************
# subroutine:  xls_cell_value
# return:      Excel cell value no matter it's in merge area or not, and in windows or not
# input:       $Sheet:  Specified Excel Sheet
# input:       $row:    Specified row number
# input:       $col:    Specified column number
#****************************************************************************************
sub xls_cell_value()
{
    my($Sheet, $row, $col) = @_;
    my $cell = $Sheet->get_cell($row, $col);
    if (defined $cell)
    {
        return $cell->Value();
    } else
    {
        print "$Sheet: row=$row, col=$col undefined\n";
        return;
    }
}

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler()
{
    my($error_msg, $file, $line_no) = @_;
    my $final_error_msg = "NandGen ERROR: $error_msg at $file line $line_no\n";
    print $final_error_msg;
    die $final_error_msg;
}
#*******************************************************************************
#by_number: sort algorithm
#*******************************************************************************
sub by_length
{
	if(length($a)>length($b))
	{-1}
	elsif(length($a)<length($b))
	{1}
	elsif(length($a)==length($b))
	{$a<=>$b;}
}

sub by_number
{$a<=>$b}

sub struct_flashdev_info_define()
{
    my $template = <<"__TEMPLATE";
#define RAMDOM_READ\t\t(1<<0)
#define CACHE_READ\t\t(1<<1)

typedef struct
{
   u8 id[NAND_MAX_ID];
   u8 id_length;
   u8 addr_cycle;
   u8 iowidth;
   u16 totalsize;
   u16 blocksize;
   u16 pagesize;
   u16 sparesize;
   u32 timmingsetting;
   u8 devciename[14];
   u32 advancedmode;
}flashdev_info,*pflashdev_info;

__TEMPLATE

   return $template;
}
