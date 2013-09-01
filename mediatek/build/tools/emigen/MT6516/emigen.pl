#!/usr/bin/perl
#
#****************************************************************************/
#*
#* Filename:
#* ---------
#*   emigen_sp.pl
#*
#* Project:
#* --------
#*
#*
#* Description:
#* ------------
#*   This script will
#*       1. parse custom_MemoryDevice.h to get memory device type and part number
#*       2. read a excel file to get appropriate emi setting based on the part number
#*       3. based on the emi settings, generate custom_EMI.c if not exist
#*       4. based on the emi settings, generate custom_EMI.h if not exist
#*
#* Author:
#* -------
#*
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
my $os = &OsName();
my $start_num;
if ($os eq "windows")
{
  use strict;
  &gen_pm;
  require 'ForWindows.pm';
  $Win32::OLE::Warn = 3; 
  $start_num = 1;
}
elsif ($os eq "linux")
{
   print "Os = linux\n";
#   push(@INC, '/usr/local/lib/perl5/site_perl/5.8.8/');
   push(@INC, 'mediatek/build/tools/Spreadsheet');
   push(@INC, 'mediatek/build/tools');
   require 'ParseExcel.pm';
   $start_num = 0; 
}
else
{
  die "unknow OS!\n";
}
#****************************************************************************
# PLATFORM EMI support matrix
#****************************************************************************
my %BBtbl_LPSDRAM = 
(       
	'MT6516'  => 1,
);

#****************************************************************************
# Constants
#****************************************************************************
my $EMIGEN_VERNO  = " V0.01";
                    # V0.01, Zhen Jiang, Porting emigen to DUMA project
                    #
my $DebugPrint    = 1; # 1 for debug; 0 for non-debug

my $COLUMN_VENDOR               = $start_num + 0;
my $COLUMN_PART_NUMBER          = $start_num + 1;

# column index for Sheet 'LPSDRAM'
my $COLUMN_DENSITY              = $start_num + 2;
my $COLUMN_MT6516               = $start_num + 3;

my $CUSTOM_MEMORY_DEVICE_HDR  = $ARGV[0]; # src\custom\<project>, need full path for now
#my $MEMORY_DEVICE_LIST_XLS    = Win32::GetCwd()."\\memorydevicelist\\".$ARGV[1];
my $MEMORY_DEVICE_LIST_XLS    = $ARGV[1];
my $PLATFORM                  = $ARGV[2]; # MTxxxx
my $PROJECT               = $ARGV[3];

print "$CUSTOM_MEMORY_DEVICE_HDR\n$MEMORY_DEVICE_LIST_XLS\n$PLATFORM\n" if ($DebugPrint == 1);

# following parameters come from $CUSTOM_MEMORY_DEVICE_HDR
my $MEMORY_DEVICE_TYPE;
my $LPSDRAM_CHIP_SELECT = 0xFF;

# data structure of $part_number if ($MEMORY_DEVICE_TYPE eq 'LPSDRAM')
#
# my $part_info =
# {
#    CS       => { "0" => { PART_NUMBER     => $part_number,
#                           EXCEL_ROW       => $excel_row,
#                           VENDOR          => $vendor,
my $part_info     = ();   # has different data structures for different $MEMORY_DEVICE_TYPE

my $is_part_found = 0;    #  0: No part number is found
                          #  1: 1 part number is found

my $bank_num = 0;         #  0: No memory is attached        
                          #  1: 1 is attached        
                          #  2: 2 are attached      
                  
# locations of output EMI settings
# src\custom\<project>\DRV\bootloader\EMI
my $CUSTOM_EMI_H = $CUSTOM_MEMORY_DEVICE_HDR;
my $CUSTOM_EMI_C = $CUSTOM_MEMORY_DEVICE_HDR;
my $INFO_TAG = $CUSTOM_MEMORY_DEVICE_HDR;
my $MEMORY_INFO_H = $CUSTOM_MEMORY_DEVICE_HDR;

if ($os eq "windows")
{
  $CUSTOM_EMI_H =~ s/custom_MemoryDevice.h$/output\\custom_emi\.h/i;
  $CUSTOM_EMI_C =~ s/custom_MemoryDevice.h$/output\\custom_emi\.c/i;
  $MEMORY_INFO_H =~ s/custom_MemoryDevice.h$/output\\memory_info\.h/i; 
  `mkdir output` unless (-d "output");
}
elsif ($os eq "linux")
{
  $CUSTOM_EMI_H =~ s/custom_MemoryDevice.h$/custom_emi\.h/i;
  $CUSTOM_EMI_C =~ s/inc\/custom_MemoryDevice.h$/custom_emi\.c/i;
  $INFO_TAG     =~ s/inc\/custom_MemoryDevice.h$/MTK_Loader_Info\.tag/i;
  $MEMORY_INFO_H =~ s/preloader\/inc\/custom_MemoryDevice.h$/uboot\/inc\/memory_info\.h/i;
}

print "$CUSTOM_EMI_H\n$CUSTOM_EMI_C\n$INFO_TAG\n$MEMORY_INFO_H\n" if ($DebugPrint ==1);

# check existance of custom_EMI.h and custom_EMI.c
my $is_existed_h             = (-e $CUSTOM_EMI_H)?           1 : 0;
my $is_existed_c             = (-e $CUSTOM_EMI_C)?           1 : 0;
#
#if ( ($is_existed_h == 1) && ($is_existed_c == 1) )
#{
#   print "\n\nALL custom_EMI\.h, custom_EMI\.c are existed!!!\n\n\n";
#   exit;
#}


#****************************************************************************
# parse custom_MemoryDevice.h to extract MEMORY_DEVICE_TYPE & PART_NUMBER
#****************************************************************************
open CUSTOM_MEMORY_DEVICE_HDR, "<$CUSTOM_MEMORY_DEVICE_HDR" or &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: file error!", __FILE__, __LINE__);
my %CUSTOM_MEM_DEV_OPTIONS;

while (<CUSTOM_MEMORY_DEVICE_HDR>)
{
   # matching the following lines
   # "#define MEMORY_DEVICE_TYPE          LPSDRAM"
   # "#define CS0_PART_NUMBER             EDK1432CABH60"

   # error-checking
   if (/^#if|^#ifdef|^#ifndef|^#elif|^#else/)
   {
      &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Not allowed to set conditional keywords $_ in custom_MemoryDevice.h!", __FILE__, __LINE__)
          unless (/^#ifndef\s+__CUSTOM_MEMORYDEVICE__/);
   }

   if (/^#define\s+(\w+)\s+\((\w*)\)/ || /^#define\s+(\w+)\s+(\w*)/)
   {
      &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: $1 redefined in custom_MemoryDevice.h!", __FILE__, __LINE__) if defined($CUSTOM_MEM_DEV_OPTIONS{$1});
      if (!defined $2)
      {
          $CUSTOM_MEM_DEV_OPTIONS{$1} = 'TRUE';
      }
      else
      {
          $CUSTOM_MEM_DEV_OPTIONS{$1} = $2;
      }
      
      my $option  = $1;
      my $content = $2;
      if ($option =~ /CS(\d+)_PART_NUMBER/)
      {
         if ($CUSTOM_MEM_DEV_OPTIONS{MEMORY_DEVICE_TYPE} eq 'LPSDRAM')
         {
         	#Make sure this platform support LPSDRAM
         	if ( not exists $BBtbl_LPSDRAM{$PLATFORM} )
			{
				die "$PLATFORM not support LPSDRAM!\n";
			}    
			        	
			$LPSDRAM_CHIP_SELECT = $1;
            &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Only CS0 or CS1 is allowed for LPSDRAM!", __FILE__, __LINE__) if (($LPSDRAM_CHIP_SELECT != 0) && ($LPSDRAM_CHIP_SELECT != 1));
            
			$part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{PART_NUMBER} = $content;
         }
         
         $bank_num++;
      }
   }
}

my $EMI_GEND_MT6516;
my $Boot_Bank1_MT6516 = 0; # only boot from BANK1 for single memory device
  						   # 0, don't boot from BANK1
  						   # 1, boot from BANK1	
my $Bank1_Swap = "";
if (($PLATFORM eq 'MT6516'))
{
	if ( (defined($part_info->{CS}->{0}->{PART_NUMBER})) && ($bank_num == 1) )
	{
		print "$PLATFORM: The single DDR is attached at BANK0 !!\n";
		#$Boot_Bank1_MT6516 = 1;
		$EMI_GEND_MT6516 = "0x0001000E"; 
	}
	
	if ( (defined($part_info->{CS}->{1}->{PART_NUMBER})) && ($bank_num == 1) )
	{
		print "$PLATFORM: The single DDR is attached at BANK1 !!\n";
		$Boot_Bank1_MT6516 = 1;
		$EMI_GEND_MT6516 = "0x0002000D";
		$Bank1_Swap = &bank1_swap_func();
	}
	
	if ($bank_num == 2)
	{
		$EMI_GEND_MT6516 = "0x0003000C";
		#make sure the two banks have the same device.
		if ( $part_info->{CS}->{1}->{PART_NUMBER} ne $part_info->{CS}->{0}->{PART_NUMBER})
		{
			print "$PLATFORM: need to use the same memory device for two banks!!\n";
			exit;
		}
	}
}

print "EMI_GEND is $EMI_GEND_MT6516\n" if ($DebugPrint ==1);
 
$MEMORY_DEVICE_TYPE              = $CUSTOM_MEM_DEV_OPTIONS{MEMORY_DEVICE_TYPE};

close CUSTOM_MEMORY_DEVICE_HDR;

if ($DebugPrint == 1)
{
   print "Device Type : $MEMORY_DEVICE_TYPE  , ";
   print "$part_info->{CS}->{0}->{PART_NUMBER} , $part_info->{CS}->{1}->{PART_NUMBER} \n";
}

#****************************************************************************
# read a excel file to get emi settings
#****************************************************************************
# get already active Excel application or open new
if ($os eq "windows")
{
   $Excel = Win32::OLE->GetActiveObject('Excel.Application')
    || Win32::OLE->new('Excel.Application', 'Quit');

  # copy the Excel file to a temp file and open it;
  # this will prevent error due to simultaneous Excel access
  $Book    = $Excel->Workbooks->Open($MEMORY_DEVICE_LIST_XLS);
}
else
{
  my $parser = Spreadsheet::ParseExcel->new();
  $Book = $parser->Parse($MEMORY_DEVICE_LIST_XLS); 
}
# select worksheet
my $Sheet;

my $eos_flag       = 7; # if this flag counts to '0', it means End Of Sheet


if ($MEMORY_DEVICE_TYPE eq 'LPSDRAM')
{
    &DeviceListParser_LPSDRAM();
}# elsif ($MEMORY_DEVICE_TYPE eq 'LPSDRAM')
else
{
    undef $MEMORY_DEVICE_TYPE;
}

# close the temp Excel file
if ($os eq "windows")
{
$Book->Close(1);
}
&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Incorrect memory device type!", __FILE__, __LINE__) unless $MEMORY_DEVICE_TYPE;
&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: part number not supported!", __FILE__, __LINE__)    if ($is_part_found <= 0);

&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_RAM_MCP type!", __FILE__, __LINE__)     if (($MEMORY_DEVICE_TYPE eq 'NOR_RAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'YES'));
&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_LPSDRAM_MCP type!", __FILE__, __LINE__) if (($MEMORY_DEVICE_TYPE eq 'NOR_LPSDRAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'NO'));



#****************************************************************************
# generate custom_EMI.c
#****************************************************************************
#if ($is_existed_c == 0)
{
    open (CUSTOM_EMI_C, ">$CUSTOM_EMI_C") or &error_handler("$CUSTOM_EMI_C: file error!", __FILE__, __LINE__);

    print CUSTOM_EMI_C &copyright_file_header();
    print CUSTOM_EMI_C &description_file_header(                      "custom_emi.c",
          "This Module defines the EMI (external memory interface) related setting.",
                                                 "EMI auto generator". $EMIGEN_VERNO);
    print CUSTOM_EMI_C &custom_EMI_c_file_body();
    close CUSTOM_EMI_C or &error_handler("$CUSTOM_EMI_C: file error!", __FILE__, __LINE__);

    print "\n$CUSTOM_EMI_C is generated\n";
} # if ($is_existed_c == 0)

#****************************************************************************
# generate custom_emi.h
#****************************************************************************
#if ($is_existed_h == 0)
{
    open (CUSTOM_EMI_H, ">$CUSTOM_EMI_H") or &error_handler("CUSTOM_EMI_H: file error!", __FILE__, __LINE__);

    print CUSTOM_EMI_H &copyright_file_header();
    print CUSTOM_EMI_H &description_file_header(                      "custom_emi.h",
          "This Module defines the EMI (external memory interface) related setting.",
                                                 "EMI auto generator". $EMIGEN_VERNO);
    print CUSTOM_EMI_H &custom_EMI_h_file_body();
    close CUSTOM_EMI_H or &error_handler("$CUSTOM_EMI_H: file error!", __FILE__, __LINE__);

    print "\n$CUSTOM_EMI_H is generated\n";
} # if ($is_existed_h == 0)

##****************************************************************************
## generate memory_info.h
##****************************************************************************
#if ($is_existed_h == 0)
{
    open (MEMORY_INFO_H, ">$MEMORY_INFO_H") or &error_handler("MEMORY_INFO_H: file error!", __FILE__, __LINE__);

    print MEMORY_INFO_H &copyright_file_header();
    print MEMORY_INFO_H &description_file_header(                      "memory_info.h",
          "This Module defines the memory information related setting.",
                                                 "EMI auto generator". $EMIGEN_VERNO);
    print MEMORY_INFO_H &memory_info_h_file_body();
    close MEMORY_INFO_H or &error_handler("$MEMORY_INFO_H: file error!", __FILE__, __LINE__);

    print "\n$MEMORY_INFO_H is generated\n";
} # if ($is_existed_h == 0)

&write_tag($PROJECT);
exit;

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler
{
	   my ($error_msg, $file, $line_no) = @_;
	   
	   my $final_error_msg = "EMIGEN ERROR: $error_msg at $file line $line_no\n";
	   print $final_error_msg;
	   die $final_error_msg;
}

#****************************************************************************
# subroutine:  copyright_file_header
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header
{
    my $template = <<"__TEMPLATE";
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
__TEMPLATE

   return $template;
}

#****************************************************************************
# subroutine:  description_file_header
# return:      file header -- description 
# input:       $filename:     filename
# input:       $description:  one line description
# input:       $author:       optional
#****************************************************************************
sub description_file_header
{
    my ($filename, $description, $author) = @_;
    my @stat_ar = stat $MEMORY_DEVICE_LIST_XLS;
    my ($day, $month, $year) = (localtime($stat_ar[9]))[3,4,5]; $month++; $year+=1900;
    my $template = <<"__TEMPLATE";
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   $filename
 *
 * Project:
 * --------
 *   Android
 *
 * Description:
 * ------------
 *   $description
 *
 * Author:
 * -------
 *  $author
 *
 *   Memory Device database last modified on $year/$month/$day
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * \$Revision\$
 * \$Modtime\$
 * \$Log\$
 *
 *------------------------------------------------------------------------------
 * WARNING!!!  WARNING!!!   WARNING!!!  WARNING!!!  WARNING!!!  WARNING!!! 
 * This file is generated by EMI Auto-gen Tool.
 * Please do not modify the content directly!
 * It could be overwritten!
 *============================================================================
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

__TEMPLATE

   return $template;
}

#****************************************************************************
# subroutine:  HeaderBody_for_lpsdram
# return:      content for custom_EMI.h 
#****************************************************************************
sub custom_EMI_h_file_body
{
    my $dram_gend;
    my $density       = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{DENSITY};

    my $dram_gena     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_GENA_VAL};
    my $dram_coni     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_CONI_VAL};
    my $dram_conn     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_CONN_VAL};
	my $dram_drct	  = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_DRCT_VAL};

    my $dram_genb     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENB_VAL};
    my $dram_genc     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENC_VAL};
    my $dram_conj     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONJ_VAL};
    my $dram_conk     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONK_VAL};
    my $dram_conl     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONL_VAL};
    my $dram_dela     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELA_VAL};
    my $dram_delb     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELB_VAL};
    my $dram_delc     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELC_VAL};
    my $dram_deld     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELD_VAL};
    my $dram_deli     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELI_VAL};
    my $dram_delj     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELJ_VAL};
	
	if ($PLATFORM eq 'MT6516')
	{
		$dram_gend = $EMI_GEND_MT6516;
		if ( $Boot_Bank1_MT6516 == 1)
		{
			$dram_gena = "($dram_gena | 0x0000000$Boot_Bank1_MT6516)";
		}
	} 
    ###
    my $template = <<"__TEMPLATE";
/* EMI_GENA value */
#define EMI_GENA_VAL         $dram_gena
/* EMI_GENB value */
#define EMI_GENB_VAL         $dram_genb
/* EMI_GENC value */
#define EMI_GENC_VAL         $dram_genc
/* EMI_GEND value */
#define EMI_GEND_VAL         $dram_gend

/* EMI_CONI value */
#define EMI_CONI_VAL         $dram_coni
/* EMI_CONJ value */
#define EMI_CONJ_VAL         $dram_conj
/* EMI_CONK value */
#define EMI_CONK_VAL         $dram_conk
/* EMI_CONL value */
#define EMI_CONL_VAL         $dram_conl

/* EMI_CONN value */
//EMI_CONN 
//1. address type, dram type and controller enable must be read from xls
//2. Run DRAM init flow 
//3. After enable auto-refresh, power down bits in EMI_CONN 
#define EMI_CONN_VAL         $dram_conn

/* EMI_DELA value */
#define EMI_DELA_VAL         $dram_dela
/* EMI_DELB value */
#define EMI_DELB_VAL         $dram_delb
/* EMI_DELC value */
#define EMI_DELC_VAL         $dram_delc
/* EMI_DELD value */
#define EMI_DELD_VAL         $dram_deld
/* EMI_DELI value */
#define EMI_DELI_VAL         $dram_deli
/* EMI_DELJ value */
#define EMI_DELJ_VAL         $dram_delj

/* EMI_DRCT value */
#define EMI_DRCT_VAL         $dram_drct

__TEMPLATE

    return $template;
}

#****************************************************************************
# subroutine:  custom_EMI_c_file_body
# return:      
#****************************************************************************
sub custom_EMI_c_file_body
{
	###
	my $template = <<"__TEMPLATE";
#include "mt6516.h"
#include "mt6516_typedefs.h"
#include "mt6516_emi_reg.h"
#include "custom_emi.h"

extern void store_8word();
extern UINT8 load_8word();

// void EMI_DDR_Enable_AutoTracking(void) will only appear when ETT perl detect DDR memory 

#define DDR_PATTERN1   0x5555aaaa
#define DDR_PATTERN2   0x12345678
#define MOD "MEM"

void Custom_EMI_DDR_Enable_AutoTracking(void) 
{
    unsigned char dqsdly_tbl[16]={0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30,0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78};

    UINT32 i, j;
    UINT32 dram_baseaddr;
    UINT32 dqs_tbl_size=sizeof(dqsdly_tbl)/sizeof(unsigned char);
    UINT32 dqs_offset;
    UINT32 valid_count;
    
    // reset and disable all DRAM bank auto-tracking first 
    // [Amos_20090211]Due to DRAM can only mounted in CS0/1, so CS2/3 DQ tracking is not necessary 
	DRV_WriteReg32(EMI_DQSA, 0x00000000);
	DRV_WriteReg32(EMI_DQSB, 0x00000000);
	DRV_WriteReg32(EMI_DQSC, 0x00000000);
	DRV_WriteReg32(EMI_DQSD, 0x00000000);

    // process all DRAM bank 
    for(i=0; i<2; i++) {

        // select DRAM bank 
        if( 0 == (DRV_Reg32(EMI_GEND) & (0x10000<<i)) ) {
            continue;
        }

        dram_baseaddr = 0x10000000*i;

        $Bank1_Swap
        
        // DQS auto-tracking initial value calibration 
        valid_count=0;
        store_8word(dram_baseaddr, DDR_PATTERN1);   // write PATTERN1 for clearing EMI read FIFO 
        for(j=dqs_tbl_size-1; j>=0; j--) 
        {
            // set DQS auto-tracking offset 
            dqs_offset = (dqsdly_tbl[j]<<24)|(dqsdly_tbl[j]<<16)|(dqsdly_tbl[j]<<8)|dqsdly_tbl[j];
            
            if( 0 == i ) {
                //*EMI_DQSA = dqs_offset;
                DRV_WriteReg32(EMI_DQSA, dqs_offset);       
            }
            else if( 1 == i ) {
                //*EMI_DQSB = dqs_offset;
                DRV_WriteReg32(EMI_DQSB, dqs_offset);       
            }             
            else if( 2 == i ) {
                //*EMI_DQSC = dqs_offset;
                DRV_WriteReg32(EMI_DQSC, dqs_offset);       
            } 
            else if( 3 == i ) {
                //*EMI_DQSD = dqs_offset;
                DRV_WriteReg32(EMI_DQSD, dqs_offset);       
            } 

            if( 0 == valid_count ) {
                // read back from DDR and compare with PATTERN1 
                if( 0 == load_8word(dram_baseaddr, DDR_PATTERN1) ) {
                    // EMI read FIFO is PATTERN1 now, write PATTERN2 for auto-tracking initial offset calibration 
                    store_8word(dram_baseaddr, DDR_PATTERN2);
                    valid_count++;
                }
            }  
	        if ( 1 == valid_count ) {            	
                // read back from DDR and compare with PATTERN2 
                if( 0 == load_8word(dram_baseaddr, DDR_PATTERN2) ) {
                    // PATTERN2 is matched, in order to skip bounary offset value, set to next DQS offset and read again 
                    valid_count++;
                }
            }
         
            if ( 2 == valid_count ) {
                // read back from DDR and compare with PATTERN2 
                if( 0 == load_8word(dram_baseaddr, DDR_PATTERN2) ) {
                    // a valid initial offset is found! 
                    break;
                }
                else {
                    // error, should not happen!
                    while(1);
                }
            }      
		}
        // enable auto-tracking 
        if( 0 == i ) {
            SETREG32(EMI_DQSA, 0x80808080);       
        }
        else if( 1 == i ) {
            SETREG32(EMI_DQSB, 0x80808080);       
        }        
        else if( 2 == i ) {
            SETREG32(EMI_DQSC, 0x80808080);       
        } 
        else if( 3 == i ) {
            SETREG32(EMI_DQSD, 0x80808080);       
        } 
	}
    // enable dummy read 
    DRV_WriteReg32(EMI_DRCT, EMI_DRCT_VAL);       
}


// call EMI_InitDDR if custom_ifSDRAMDDR return true
void Custom_EMI_InitDDR(void)
{
    UINT32 i;
    // Mapping EMI base register to handle potential MMU enable request
				
    // config DRAM bank, 
    DRV_WriteReg32(EMI_GEND, EMI_GEND_VAL);
    // switch to external boot and enable dram clock
    // enable HW DRAM self-refresh mode  
    DRV_WriteReg32(EMI_GENA, EMI_GENA_VAL);
    // config DRAM type and address type and enable DRAM controller        
    // EMI_CONN_Value 		0x00530001  
    DRV_WriteReg32(EMI_CONN, EMI_CONN_VAL);
    // DRAM AC timing setting 
   	DRV_WriteReg32(EMI_CONI, EMI_CONI_VAL);
	DRV_WriteReg32(EMI_CONJ, EMI_CONJ_VAL);   	
	DRV_WriteReg32(EMI_CONK, EMI_CONK_VAL);// calculated by fixed clock 3.25MHz 
	DRV_WriteReg32(EMI_CONL, EMI_CONL_VAL);
	// DRAM driving 
    DRV_WriteReg32(EMI_GENB, EMI_GENB_VAL);
	DRV_WriteReg32(EMI_GENC, EMI_GENC_VAL);
    // Setup/Hold time balance
    // Setup time adjustment: DQ_OUT and DQM_OUT (delay data to shorten setup time) 
    // Hold time adjustment: DQS_OUT and EDCLK_OUT (delay DQS to shorten hold time) 
    DRV_WriteReg32(EMI_DELA, EMI_DELA_VAL);
	DRV_WriteReg32(EMI_DELB, EMI_DELB_VAL);
	DRV_WriteReg32(EMI_DELC, EMI_DELC_VAL);
	DRV_WriteReg32(EMI_DELD, EMI_DELD_VAL);  
	DRV_WriteReg32(EMI_DELI, EMI_DELI_VAL);
	DRV_WriteReg32(EMI_DELJ, EMI_DELJ_VAL);
	  
    // DRAM init procedure 
    // pre-charge all 
    SETREG32(EMI_CONN, 0x10000000);
    // [shock_20090211]suppose delay loop is not necessary 
    for(i=0; i<10; i++);    
    CLRREG32(EMI_CONN, 0x10000000);
		
    // auto-refresh 1    
    SETREG32(EMI_CONN, 0x08000000);
    for(i=0; i<10; i++);    
    CLRREG32(EMI_CONN, 0x08000000);
    
    // auto-refresh 2    
    SETREG32(EMI_CONN, 0x04000000);
    for(i=0; i<10; i++);    
    CLRREG32(EMI_CONN, 0x04000000);

    // set mode register     
    SETREG32(EMI_CONN, 0x02000000);
    for(i=0; i<10; i++);
    CLRREG32(EMI_CONN, 0x02000000);

    // set extended mode register 
    SETREG32(EMI_CONN, 0x01000000);
    for(i=0; i<10; i++);    
	CLRREG32(EMI_CONN, 0x01000000);
				    
    // enable 3.25MHz fixed-clock DRAM auto-refresh      
	SETREG32(EMI_CONN, 0x00000004);
	// enable DDR 1/5T Digital DDL lock    
	SETREG32(EMI_CONN, 0x00000100);
	// NEW: Enable power down mode in CONN for power saving   
	SETREG32(EMI_CONN, 0x00000010);
	
	// NEW: High priority for MD (AHB3)   
	SETREG32(EMI_GENA, 0x00001000);
		
    // DDR enable auto-tracking and dummy read     
    Custom_EMI_DDR_Enable_AutoTracking();

}

void mt6516_set_emi(void)
{
    dbg_print("[%s] MT6516 EVB",MOD);		
    Custom_EMI_InitDDR();
}

__TEMPLATE
    return $template;  
}

#****************************************************************************
# subroutine:  memory_info_h_file_body
# return:      
#****************************************************************************
sub memory_info_h_file_body
{
    my $density       = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{DENSITY} / 8;
    my $size;
    my $size0;
    my $size1;
    my $start1;
    
    if($density == 128)
    {
        $size = "SZ_128M";
    }
    elsif($density == 256)
    {
        $size = "SZ_256M";
    }
    
    if($bank_num == 1)
    {
        $size0 = $size;
        $size1 = 0;
        $start1 = 0;
    }
    elsif($bank_num == 2)
    {
        $size0 = $size;
        $size1 = $size;
        $start1 = "0x10000000";
    }
        
    ###        
	my $template = <<"__TEMPLATE";
#ifndef __MT65XX_MEM_INFO_H__
#define __MT65XX_MEM_INFO_H__

#define DRAM_BANKS_NR   $bank_num

#define CFG_PHYS_SDRAM_0_SIZE         ($size0 - RIL_SIZE)
#define CFG_PHYS_SDRAM_0_START        RIL_SIZE
#define CFG_PHYS_SDRAM_1_SIZE         ($size1)
#define CFG_PHYS_SDRAM_1_START        $start1
#define CFG_PHYS_SDRAM_2_SIZE         0
#define CFG_PHYS_SDRAM_2_START        0
#define CFG_PHYS_SDRAM_3_SIZE         0
#define CFG_PHYS_SDRAM_3_START        0

#endif //__MT65XX_MEM_INFO_H__	
__TEMPLATE
    return $template;  
}
	
#****************************************************************************
# subroutine:  DeviceListParser_LPSDRAM
# return:      None
#****************************************************************************
sub DeviceListParser_LPSDRAM
{
    my $row        = $start_num + 0;                   # scan from row 2 when $MEMORY_DEVICE_TYPE eq 'LPSDRAM'
    my $col        = $COLUMN_PART_NUMBER; # scan column 2 for Part Number
    $Sheet = get_sheet($MEMORY_DEVICE_TYPE);
    # find cell address of the specified part number
    my $scan_idx = &xls_cell_value($Sheet, $row, $col);
    while (defined ($scan_idx) || ($eos_flag > 0))
    {
       ++$row;
       $scan_idx = &xls_cell_value($Sheet, $row, $col);
        unless (defined $scan_idx)
        {
            print "[$row][scan_idx]No Value\n" if $DebugPrint == 1;
            $eos_flag --;
            next;
        }

        $eos_flag   = 7;

        # remove leading and tailing spaces
        $scan_idx =~ s/^\s+// if $DebugPrint == 1;
        $scan_idx =~ s/\s+$// if $DebugPrint == 1;
		
        if ($scan_idx =~ /$part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{PART_NUMBER}/) # scan column 2 for Part Number
        {
            if (defined($part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{EXCEL_ROW}))
            {
                &error_handler("$MEMORY_DEVICE_LIST_XLS: redefine for the same part number in row $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{EXCEL_ROW}!", __FILE__, __LINE__);
            }
            else
            {
                $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{EXCEL_ROW} = $row;
                print "part_number($part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{PART_NUMBER}) found in row $row\n";
                $is_part_found = 1;
                last;
            }
        }

    } # while (defined (my $scan_idx = $Sheet->Cells(++$row, $col)->{'Value'}) || ($eos_flag > 0))
	    # collect information for specified part number from Excel
    #
    # data structure of $part_info if ($MEMORY_DEVICE_TYPE eq 'LPSDRAM')
    #
    # my $part_info =
    # {
    #    CS       => { "0" => { VENDOR        => $vendor,
    #                           DENSITY       => $density,
    #                           MT6516        => { "EMI_GENA_VAL"  => $setting,
    #											   "EMI_CONI_VAL"  => $setting,
    #											   "EMI_CONN_VAL"  => $setting,
    #											   "EMI_DRCT_VAL"  => $setting,    	 	
    #                                              "EMI_104MHZ_DRIVING"  => { "EMI_CONJ_VAL" => $setting,
    #                                                                         "EMI_CONK_VAL" => $setting,
    #                                                                         "EMI_CONL_VAL" => $setting,
    #                                                                         "EMI_GENB_VAL" => $setting,
    #                                                                         "EMI_GENC_VAL" => $setting,
    #                                                                         "EMI_DELA_VAL" => $setting,
    #                                                                         "EMI_DELB_VAL" => $setting,
    #                                                                         "EMI_DELC_VAL" => $setting,
    #                                                                         "EMI_DELD_VAL" => $setting,
    #                                                                         "EMI_DELI_VAL" => $setting,
    #                                                                         "EMI_DELJ_VAL" => $setting,},
    
    my $cs_value = $LPSDRAM_CHIP_SELECT;
    $_ = $row;
    $part_info->{CS}->{$cs_value}->{VENDOR}   = &xls_cell_value($Sheet, $_, $COLUMN_VENDOR);
    $part_info->{CS}->{$cs_value}->{DENSITY}  = &xls_cell_value($Sheet, $_, $COLUMN_DENSITY);

    # find the correct platform
    my $platform_scan_idx = $COLUMN_MT6516; #First EMI controller
    my $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx);
    while (!($tmp_platform =~ $PLATFORM))
    {
        $platform_scan_idx++;
        $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx);
    }
    &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: $PLATFORM not support LPSDRAM!", __FILE__, __LINE__) if ($platform_scan_idx > $COLUMN_MT6516);

    if ((&xls_cell_value($Sheet, $start_num, $platform_scan_idx) eq 'MT6516'))
    {
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_GENA_VAL}                       = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_CONI_VAL}                       = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_CONN_VAL}                       = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_DRCT_VAL}                       = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONJ_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONK_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONL_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENB_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENC_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELA_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELB_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELC_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELD_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELI_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
        $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELJ_VAL} = &xls_cell_value($Sheet, $_, $platform_scan_idx++);
  
		# make sure the value is valid.
	    if (($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_GENA_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_CONI_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_CONN_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_DRCT_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONJ_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONK_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONL_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENB_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENC_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELA_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELB_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELC_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELD_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELI_VAL} eq 'x') ||
	       	($part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELJ_VAL} eq 'x'))
	    {
	    	print "The value get from excel is invalid !!!\n";
	    	exit;
	    }
	    #make sure MT6516 only support, check CONN bit 16,17 are not 0, or 1
	    if ( $part_info->{CS}->{$cs_value}->{$PLATFORM}->{EMI_CONN_VAL} =~ /0x(\w{3}[0,1]\w{4})/)
	    {
	    	print "$PLATFORM only support DDR RAM!\n";
	    	exit;
	    }
    }  
}

sub bank1_swap_func
{
    my $template = <<"__TEMPLATE";
        if(0==dram_baseaddr){
            dram_baseaddr=0x10000000;
        }
        else if(0x10000000==dram_baseaddr){
            dram_baseaddr=0x0;
        }
__TEMPLATE

    return $template;
}
#****************************************************************************************
# subroutine:  OsName
# return:      which os this script is running
# input:       no input
#****************************************************************************************
sub OsName {
  my $os = `set os`;
  if(!defined $os) { 
    $os = "linux";
  } 
  else {
    $os = "windows";
  }
}
#*************************************************************************************************
# subroutine:  gen_pm
# return:      no return, but will generate a ForWindows.pm in "/perl/lib" where your perl install
#*************************************************************************************************
sub gen_pm {
  foreach (@INC) {
    if(/^.*:\/Perl\/lib$/) {
      open FILE, ">${_}\/ForWindows.pm";
      print FILE "package ForWindows;\n";
      print FILE "use Win32::OLE qw(in with);\n";
      print FILE "use Win32::OLE::Const 'Microsoft Excel';\n";
      print FILE "\$Win32::OLE::Warn = 3;\n";
      print FILE "1;";
      close(FILE);
      last;
    }
  }
}
#****************************************************************************************
# subroutine:  get_sheet
# return:      Excel worksheet no matter it's in merge area or not, and in windows or not
# input:       Specified Excel Sheetname
#****************************************************************************************
sub get_sheet {
  my $MEMORY_DEVICE_TYPE = $_[0];
  if ($os eq "windows") {
    return $Sheet     = $Book->Worksheets($MEMORY_DEVICE_TYPE);
  }
  else {
    return $Sheet     = $Book->Worksheet($MEMORY_DEVICE_TYPE);
  }
}


#****************************************************************************************
# subroutine:  xls_cell_value
# return:      Excel cell value no matter it's in merge area or not, and in windows or not
# input:       $Sheet:  Specified Excel Sheet
# input:       $row:    Specified row number
# input:       $col:    Specified column number
#****************************************************************************************
sub xls_cell_value {
  my ($Sheet, $row, $col) = @_;
  if ($os eq "windows") {
    return &win_xls_cell_value($Sheet, $row, $col);
  }
  else {
      return &lin_xls_cell_value($Sheet, $row, $col);
  }
}
sub win_xls_cell_value
{
    my ($Sheet, $row, $col) = @_;

    if ($Sheet->Cells($row, $col)->{'MergeCells'})
    {
        my $ma = $Sheet->Cells($row, $col)->{'MergeArea'};
        return ($ma->Cells(1, 1)->{'Value'});
    }
    else
    {
        return ($Sheet->Cells($row, $col)->{'Value'})
    }
}
sub lin_xls_cell_value
{
  my ($Sheet, $row, $col) = @_;
  my $cell = $Sheet->get_cell($row, $col);
  exit 1 unless (defined $cell);
  my $value = $cell->Value();

}

sub write_tag()
{
  my $project = lc($_[0]);

  open FILE,">$INFO_TAG";
  print FILE pack("a27", "MTK_BLOADER_INFO_v04");
  seek(FILE, 0x1b, 0);
  $pre_bin = "preloader_${project}.bin";
  print "PROJECT = $project, pre_bin = $pre_bin\n";
  print FILE pack("a61", $pre_bin); 
  seek(FILE, 0x58, 0);
  print FILE pack("H8", 56313136);
  print FILE pack("H8", 22884433);
  print FILE pack("H8", "00200040");
  my $dram_gend;
  my $dram_gena     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_GENA_VAL};
  seek(FILE,0x64, 0);
  print FILE pack("L", hex($dram_gena));

  my $dram_genb     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENB_VAL};
  print FILE pack("L", hex($dram_genb));

  my $dram_genc     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_GENC_VAL};
  print FILE pack("L", hex($dram_genc));

  if ($PLATFORM eq 'MT6516')
  {
    $dram_gend = $EMI_GEND_MT6516;
    if ( $Boot_Bank1_MT6516 == 1)
    {
      $dram_gena = "($dram_gena | 0x0000000$Boot_Bank1_MT6516)";
    }
  } 
  print FILE pack("L", hex($dram_gend));

  my $dram_coni     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_CONI_VAL};
  print FILE pack("L", hex($dram_coni));

  my $dram_conj     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONJ_VAL};
  print FILE pack("L", hex($dram_conj));

  my $dram_conk     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONK_VAL};
  print FILE pack("L", hex($dram_conk));

  my $dram_conl     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_CONL_VAL};
  print FILE pack("L", hex($dram_conl));
  print FILE pack("H8", 00000000);

  my $dram_conn     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_CONN_VAL};
  print FILE pack("L", hex($dram_conn));
#	my $dram_drct	  = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_DRCT_VAL};


  my $dram_dela     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELA_VAL};
  print FILE pack("L", hex($dram_dela));

  my $dram_delb     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELB_VAL};
  print FILE pack("L", hex($dram_delb));

  my $dram_delc     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELC_VAL};
  print FILE pack("L", hex($dram_delc));
  
  my $dram_deld     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELD_VAL};
  print FILE pack("L", hex($dram_deld));
  seek(FILE, 16, 1);
  
  my $dram_deli     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELI_VAL};
  print FILE pack("L", hex($dram_deli));
  
  my $dram_delj     = $part_info->{CS}->{$LPSDRAM_CHIP_SELECT}->{$PLATFORM}->{EMI_104MHZ_DRIVING}->{EMI_DELJ_VAL};
  print FILE pack("L", hex($dram_delj));
  print FILE pack("a8", "MTK_BIN");
  print FILE pack("H8", bc000000);
  print "$INFO_TAG is generated!\n";
}
