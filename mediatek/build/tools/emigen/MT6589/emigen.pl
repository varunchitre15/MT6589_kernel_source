#!/usr/local/bin/perl




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
#   push(@INC, '.');
#   push(@INC, './Spreadsheet');
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
	'MT6589'  => 1,
);

#****************************************************************************
# Constants
#****************************************************************************
my $EMIGEN_VERNO  = " V0.01";
                    # V0.01, Zhen Jiang, Porting emigen to DUMA project
                    #
my $DebugPrint    = 1; # 1 for debug; 0 for non-debug

my $COLUMN_VENDOR               = $start_num + 0;
my $COLUMN_PART_NUMBER	        = $COLUMN_VENDOR + 1 ;
my $COLUMN_TYPE	                = $COLUMN_PART_NUMBER + 1 ;
my $COLUMN_DENSITY	            = $COLUMN_TYPE + 1 ;
my $COLUMN_BOARD_ID	            = $COLUMN_DENSITY + 1 ;
my $COLUMN_NAND_EMMC_ID	        = $COLUMN_BOARD_ID + 1 ;
my $COLUMN_FW_ID	        = $COLUMN_NAND_EMMC_ID + 1 ;
my $COLUMN_NAND_PAGE_SIZE       = $COLUMN_FW_ID + 1 ;
my $COLUMN_PLATFORM             = $COLUMN_NAND_PAGE_SIZE + 1 ;

my $CUSTOM_MEMORY_DEVICE_HDR  = $ARGV[0]; # src\custom\<project>, need full path for now
#my $MEMORY_DEVICE_LIST_XLS    = Win32::GetCwd()."\\memorydevicelist\\".$ARGV[1];
my $MEMORY_DEVICE_LIST_XLS    = $ARGV[1];
my $PLATFORM                  = $ARGV[2]; # MTxxxx
my $PROJECT               = $ARGV[3];


print "$CUSTOM_MEMORY_DEVICE_HDR\n$MEMORY_DEVICE_LIST_XLS\n$PLATFORM\n" if ($DebugPrint == 1);

# following parameters come from $CUSTOM_MEMORY_DEVICE_HDR
my $MEMORY_DEVICE_TYPE;

# data structure of $part_number if ($MEMORY_DEVICE_TYPE eq 'LPSDRAM')
#
# my $part_info =
# {
#    CS       => { "0" => { PART_NUMBER     => $part_number,
#                           EXCEL_ROW       => $excel_row,
#                           VENDOR          => $vendor,
my $part_info     = ();   # has different data structures for different $MEMORY_DEVICE_TYPE

my $bank_num = 0;         #  0: No memory is attached        
                          #  1: 1 is attached        
                          #  2: 2 are attached      
                  
# locations of output EMI settings
# src\custom\<project>\DRV\bootloader\EMI
my $CUSTOM_EMI_H = $CUSTOM_MEMORY_DEVICE_HDR;
my $CUSTOM_EMI_C = $CUSTOM_MEMORY_DEVICE_HDR;
my $INFO_TAG = $CUSTOM_MEMORY_DEVICE_HDR;

if ($os eq "windows")
{
    $CUSTOM_EMI_H =~ s/custom_MemoryDevice.h$/output\\custom_emi\.h/i;
    $CUSTOM_EMI_C =~ s/custom_MemoryDevice.h$/output\\custom_emi\.c/i;
    `mkdir output` unless (-d "output");
}
elsif ($os eq "linux")
{
    $CUSTOM_EMI_H =~ s/custom_MemoryDevice.h$/custom_emi\.h/i;
    $CUSTOM_EMI_C =~ s/inc\/custom_MemoryDevice.h$/custom_emi\.c/i;
    $INFO_TAG     =~ s/inc\/custom_MemoryDevice.h$/MTK_Loader_Info\.tag/i;
}

print "$CUSTOM_EMI_H\n$CUSTOM_EMI_C\n$INFO_TAG\n" if ($DebugPrint ==1);

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

# CustCS_CustemChips:
#  the number of part number which customer assigned
#  in mediatek/custom/$project/preloader/inc/custom_MemoryDevice.h
# TotalCustemChips:
#   because one part number may matches multiple emmc/nand ID, the TotalCustemChips >= CustCS_CustemChips
#   the final number of part number which will use to create emi_setting
#   in mediatek/custom/$project/preloader/inc/custom_emi.h
#
my $CustCS_CustemChips = 0 ;
my $TotalCustemChips = 0 ;

#
#   arrays
#
#   this should be an array of structurs, but it is said perl does not support it.
#   these are input, except EMI_GEND
#

# CustCS_PART_NUMBER:
#  the content of part number which customer assigned
#  in mediatek/custom/$project/preloader/inc/custom_MemoryDevice.h
# Total_PART_NUMBER:
#   the final part number which will show
#   in mediatek/custom/$project/preloader/inc/custom_emi.h
 my $CustCS_PART_NUMBER ;
my $Total_PART_NUMBER ;


######################################################################################
my $DEV_TYPE;
my $DEV_TYPE1;
my $DEV_TYPE2;
my $NAND_EMMC_ID;
my $FW_ID ;
my $ID_String ;
my $FW_ID_String ;
my $Sub_version;
my $USE_EMMC_ID_LEN=9;
my $MAX_NAND_EMMC_ID_LEN=16;
my $MAX_FW_ID_LEN=8;
#my $ORI_ID_Length;
my $fw_id_len;
my $NAND_PAGE_SIZE;
my $EMI_CONA_VAL;           
my $DRAMC_DRVCTL0_VAL;      
my $DRAMC_DRVCTL1_VAL;      
my $DRAMC_DLE_VAL;          
my $DRAMC_ACTIM_VAL;        
my $DRAMC_GDDR3CTL1_VAL;    
my $DRAMC_CONF1_VAL;        
my $DRAMC_DDR2CTL_VAL;      
my $DRAMC_TEST2_3_VAL;      
my $DRAMC_CONF2_VAL;        
my $DRAMC_PD_CTRL_VAL;      
my $DRAMC_PADCTL3_VAL;      
my $DRAMC_DQODLY_VAL;       
my $DRAMC_ADDR_OUTPUT_DLY;  
my $DRAMC_CLK_OUTPUT_DLY;   
my $DRAMC_ACTIM1_VAL;   
my $DRAMC_MISCTL0_VAL;   
my $DRAM_RANK0_SIZE;   
my $DRAM_RANK1_SIZE; 

my $DDR1_2_3 ;
#union
#1
my $DDR2_MODE_REG1;
my $DDR2_MODE_REG2;
my $DDR2_MODE_REG3;
my $DDR2_MODE_REG10;
my $DDR2_MODE_REG63;
#2
my $DDR1_MODE_REG;
my $DDR1_EXT_MODE_REG;
#3
my $DDR3_MODE_REG0;
my $DDR3_MODE_REG1;
my $DDR3_MODE_REG2;
my $DDR3_MODE_REG3;

my $EMI_SETTINGS ;
#
# all above are arrays, each represents an user defined chip.
#
# this is the ID of the custom board.
my $CustBoard_ID ;

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
    if (/^#define\s+(\w+)\[(\d+)\]\s+\(([-\w]*)\)/ || /^#define\s+(\w+)\[(\d+)\]\s+([-\w]*)/ || 
        /^#define\s+(MEMORY_DEVICE_TYPE)\s+\((\w*)\)/ || /^#define\s+(MEMORY_DEVICE_TYPE)\s+(\w*)/ ||
        /^#define\s+(BOARD_ID)\s+\((\w*)\)/ || /^#define\s+(BOARD_ID)\s+(\w*)/) 
    {
#        print "\n $1, $2, $3\n" ;
        
        if ($1 eq "BOARD_ID")
        {
            $CustBoard_ID = $2 ;
        }
        elsif ($1 eq "CS_PART_NUMBER")
        {
            #print "\nCS0 $2, $3\n" ;
            $CustCS_PART_NUMBER[$2] = $3 ;
            $CustCS_CustemChips = $CustCS_CustemChips + 1 ;
            
            print "$CustCS_PART_NUMBER[$2]\n"
        }
    }

}
print "\n$CustCS_CustemChips\n" if ($DebugPrint ==1);
close CUSTOM_MEMORY_DEVICE_HDR;
#
#
# we now read in all the needed infomation form custom defination file,
# so close it.
#
#


#
#   check if data validate.
#
if ($CustCS_CustemChips > 5)
{
    die "\n[Error]CustCS_CustemChips($CustCS_CustemChips) > 5\n" ;
}
if ($CustCS_CustemChips == 0)
{
    die "\n[Error]CustCS_CustemChips($CustCS_CustemChips) = 0?\n" ;
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
my $eos_flag       = 4; # if this flag counts to '0', it means End Of Sheet
my $iter = 0 ;
my $CustCS_part_number_iter = 0;
my $total_part_number_iter = 0;

    while ($iter<$CustCS_CustemChips)
    {
        $eos_flag = 4;
        $total_part_number_iter = &DeviceListParser_LPSDRAM($iter,$total_part_number_iter);     
        $iter = $iter + 1 ;
    }
    
    # if the value read from excel validate
    $iter = 0 ;
    my $Discrete_DDR = 0 ;
    my $EMMC_NAND_MCP = "00" ;
    my $Page_size = "0" ;
    print "TotalCustemChips:$TotalCustemChips\n";
    while ($iter < $TotalCustemChips)
    {
        # only one Discrete is allowed
        if ($DEV_TYPE1[$iter] eq "00")      
        {
            if ($Discrete_DDR == 0)
            {
                $Discrete_DDR = $Discrete_DDR + 1 ;
                if ($TotalCustemChips > 1) 
                {
                    die "[Error]At most one discrete DRAM is allowed in the Combo MCP list\n" ;
                }
                    
            }
            else
            {
                die "[Error]more than 1 Discrete DDR used!\n" ;
            }
        }
        
    unless($ENV{PROJECT} eq "mt6589_evb1"){
        # nand emmc can't used together.
        if ($EMMC_NAND_MCP == "00")
        {
            $EMMC_NAND_MCP = $DEV_TYPE1[$iter] ;
        }
        elsif ($EMMC_NAND_MCP != $DEV_TYPE1[$iter] && $DEV_TYPE1[$iter] != "00")
        {
            die "[Error]Both NAND and eMMC are used!\n"
        }
    }
        # NAND can't use different page size.
        if ($DEV_TYPE1[$iter] == "01") 
        {
            if ($Page_size == "0")
            {
                $Page_size = $NAND_PAGE_SIZE[$iter] ;
            }
            else
            {
                if ($Page_size != $NAND_PAGE_SIZE[$iter])
                {
                    die "[Error]different MCP page size!$Page_size, $NAND_PAGE_SIZE[$iter]\n" ;
                }
            }
        }
        
        # Nand or eMMC's + FW ID must unique
        my $iter2 ;
        $iter2 = $iter + 1 ;
        while ($iter2 < $TotalCustemChips)
        {
            if ($DEV_TYPE1[$iter] != "00")
            {
                if ($NAND_EMMC_ID[$iter] eq $NAND_EMMC_ID[$iter2])
                {
                    if ( $Total_PART_NUMBER[$iter] ne $Total_PART_NUMBER[$iter2] )
                    {
                        die "[Error]Different part number:$Total_PART_NUMBER[$iter],$Total_PART_NUMBER[$iter2] with same NAND/eMMC ID:$NAND_EMMC_ID[$iter]"
                    }
                    else
                    { #$Total_PART_NUMBER[$iter] eq $Total_PART_NUMBER[$iter2] and $NAND_EMMC_ID[$iter] eq $NAND_EMMC_ID[$iter2] and $FW_ID[$iter] eq $FW_ID[$iter2]
                        if ($FW_ID[$iter] eq $FW_ID[$iter2])
                        {
                            die "[Error]Two same part number:$Total_PART_NUMBER[$iter],$Total_PART_NUMBER[$iter2] with same NAND/eMMC ID:$NAND_EMMC_ID[$iter] and same FW ID:$FW_ID[$iter]"
                        }
                    }
                }
            }
            $iter2 = $iter2 + 1 ;
        }
                
        $iter = $iter + 1 ;
    }





# close the temp Excel file
if ($os eq "windows")
{
    $Book->Close(1);
}


#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Incorrect memory device type!", __FILE__, __LINE__) unless $MEMORY_DEVICE_TYPE;
#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: part number not supported!", __FILE__, __LINE__)    if ($is_part_found <= 0);

#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_RAM_MCP type!", __FILE__, __LINE__)     if (($MEMORY_DEVICE_TYPE eq 'NOR_RAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'YES'));
#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_LPSDRAM_MCP type!", __FILE__, __LINE__) if (($MEMORY_DEVICE_TYPE eq 'NOR_LPSDRAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'NO'));



#****************************************************************************
# generate custom_EMI.c
#****************************************************************************
#if ($is_existed_c == 0)
{
    if ($is_existed_c == 1)
    {
	unlink ($CUSTOM_EMI_C);
    }
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
# generate custom_emi.h -> no build after MT6575
#****************************************************************************
#if ($is_existed_h == 0)
#{
#    open (CUSTOM_EMI_H, ">$CUSTOM_EMI_H") or &error_handler("CUSTOM_EMI_H: file error!", __FILE__, __LINE__);
#
#    print CUSTOM_EMI_H &copyright_file_header();
#    print CUSTOM_EMI_H &description_file_header(                      "custom_emi.h",
#          "This Module defines the EMI (external memory interface) related setting.",
#                                                 "EMI auto generator". $EMIGEN_VERNO);
#    print CUSTOM_EMI_H &custom_EMI_h_file_body();
#    close CUSTOM_EMI_H or &error_handler("$CUSTOM_EMI_H: file error!", __FILE__, __LINE__);
#
#    print "\n$CUSTOM_EMI_H is generated\n";
#    print "\n$CUSTOM_EMI_H need not to be generated\n";
#} # if ($is_existed_h == 0)


&write_tag($PROJECT);
exit;
#****************************************************************************
# subroutine:  trim
# input:       $string:  trim string
#****************************************************************************

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler
{
	   my ($error_msg, $file, $line_no) = @_;
	   
	   my $final_error_msg = "[Error]EMIGEN ERROR: $error_msg at $file line $line_no\n";
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
    ###
    my $template = <<"__TEMPLATE";

#ifndef MT6589_EMI_H
#define MT6589_EMI_H

extern void mt6516_set_emi (void);
extern void mt6516_256M_mem_setting (void);

typedef struct
{
    int  type;                /* 0x0000 : Invalid
                                 0x0001 : Discrete DDR1
                                 0x0002 : Discrete DDR2
                                 0x0003 : Discrete DDR3
                                 0x0101 : MCP(NAND+DDR1)
                                 0x0102 : MCP(NAND+DDR2)
                                 0x0103 : MCP(NAND+DDR3)
                                 0x0201 : MCP(eMMC+DDR1)
                                 0x0202 : MCP(eMMC+DDR2)
                                 0x0203 : MCP(eMMC+DDR3)
                              */
    char  ID[12];
    int   EMI_CONA_VAL;           //@0x3000
    int   DRAMC_DRVCTL0_VAL;      //@0x40B8               -> customized TX I/O driving
    int   DRAMC_DRVCTL1_VAL;      //@0x40BC               -> customized TX I/O driving
    int   DRAMC_DLE_VAL;          //40E4[4]:407c[6:4]     -> customized
    int   DRAMC_ACTIM_VAL;        //@0x4000
    int   DRAMC_GDDR3CTL1_VAL;    //@0x40F4
    int   DRAMC_CONF1_VAL;        //@0x4004
    int   DRAMC_DDR2CTL_VAL;      //@0x407C 
    int   DRAMC_TEST2_3_VAL;      //@0x4044
    int   DRAMC_CONF2_VAL;        //@0x4008
    int   DRAMC_PD_CTRL_VAL;      //@0x41DC
    int   DRAMC_PADCTL3_VAL;      //@0x4014               -> customized TX DQS delay
    int   DRAMC_DQODLY_VAL;       //@0x4200~0x420C        -> customized TX DQ delay
    int   DRAMC_ADDR_OUTPUT_DLY;  // for E1 DDR2 only
    int   DRAMC_CLK_OUTPUT_DLY;   // for E1 DDR2 only
    int   DRAMC_ACTIM1_VAL;    // for E1 DDR2 only
    int   DRAMC_MISCTL0_VAL;   // for E1 DDR2 only
    
    int   reserved[10] ;          // reserved

    union
    {
        struct
        {
            int   DDR2_MODE_REG1;
            int   DDR2_MODE_REG2;
            int   DDR2_MODE_REG3;
            int   DDR2_MODE_REG10;
            int   DDR2_MODE_REG63;
        };
        struct
        {
            int   DDR1_MODE_REG;
            int   DDR1_EXT_MODE_REG;
        };
        struct
        {
            int   DDR3_MODE_REG0;
            int   DDR3_MODE_REG1;
            int   DDR3_MODE_REG2;
            int   DDR3_MODE_REG3;
        };
    };
} EMI_SETTINGS;

int mt_get_dram_type (void); 
/* 0: invalid */
/* 1: mDDR1 */
/* 2: mDDR2 */
/* 3: mDDR3 */
#endif

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
    my $EMI_SETTINGS_string = "" ;
    my $ddr = -1 ;

    $iter = 0 ;
    while ($iter<$TotalCustemChips)
    {
        if ($DEV_TYPE1[$iter] == "00")
        {
            $ddr = $iter ;
            print "ddr found $ddr \n" ;
            $iter = $iter + 1 ;
        }
        else
        {
            $EMI_SETTINGS_string = $EMI_SETTINGS_string . $EMI_SETTINGS[$iter] ;

            $iter = $iter + 1 ;
            if ($iter < $TotalCustemChips || $ddr != -1)
            {
                $EMI_SETTINGS_string = $EMI_SETTINGS_string . " ," ;
            }
        }
    }
	
    if ($ddr != -1)
    {
        $EMI_SETTINGS_string = $EMI_SETTINGS_string . $EMI_SETTINGS[$ddr] ;
    }
	
	
	my $template = <<"__TEMPLATE";
#include "mt_emi.h"

#define NUM_EMI_RECORD ($TotalCustemChips)

int num_of_emi_records = NUM_EMI_RECORD ;

EMI_SETTINGS emi_settings[] =
{
     $EMI_SETTINGS_string
};
__TEMPLATE
    return $template ;
}


#****************************************************************************
# subroutine:  DeviceListParser_LPSDRAM
# input:       the number in array
# return:      string contain 1 set of EMI setting for input 
#****************************************************************************
sub DeviceListParser_LPSDRAM
{
    my ($id) ;
    my ($CustCS_id) ;
    my ($PartNum) ;
    my ($iter);
    my ($num_part_found);
        
                              
    $num_part_found = 0 ;
    $CustCS_id = $_[0]; # CustCS_CustemChips_iter
    $id = $_[1] ; # total_part_number_iter 
    
    $PartNum = $CustCS_PART_NUMBER[$CustCS_id] ;
    print "\nCustCS ID num is $CustCS_id, part number:$CustCS_PART_NUMBER[$CustCS_id]\n" ;
    

    my $row        = $start_num + 0 ;                    # scan from row 2 when $MEMORY_DEVICE_TYPE eq 'LPSDRAM'
    my $col        = $COLUMN_PART_NUMBER ;               # scan column 2 for Part Number
    my $rows_part_found;                                 # stores the part numbers found in MDL
    
    
    $Sheet = get_sheet("mt6589") ;
    
    # find cell address of the specified Nand ID
    my $scan_idx = &xls_cell_value($Sheet, $row, $col) ;
    while (defined ($scan_idx) && ($eos_flag > 0))
    {
        ++$row ;
        $scan_idx = &xls_cell_value($Sheet, $row, $col) ;
        
        unless (defined $scan_idx)
        {
            print "[$row][scan_idx]No Value, $eos_flag\n" if $DebugPrint == 1 ;
            $eos_flag -- ;
            next ;
        }
        if ($scan_idx eq "")
        {
            print "[$row][scan_idx]EQ null, $eos_flag\n" if $DebugPrint == 1 ;
            $eos_flag -- ;
            next ;
        }
        

        $eos_flag   = 4 ;

        # remove leading and tailing spaces
        $scan_idx =~ s/^\s+// if $DebugPrint == 1 ;
        $scan_idx =~ s/\s+$// if $DebugPrint == 1 ;
		
		$scan_idx =~ s/^\s+// ;
		$scan_idx =~ s/\s+$// ;
		
		#print "$scan_idx ?= $PartNum\n" ;
		
        if ($scan_idx eq $PartNum) # scan column 2 for Part Number

        {
            my $boardid ;
            $boardid = &xls_cell_value($Sheet, $row, $COLUMN_BOARD_ID) ;
            if ($CustBoard_ID eq $boardid)
            {
                $rows_part_found[$num_part_found] = $row;

                print "\nPartNum($PartNum==$scan_idx) found in row $row\n" ;
                $Total_PART_NUMBER[$TotalCustemChips] = $PartNum;
                $num_part_found += 1 ;
                $TotalCustemChips += 1;
            }
        }
    }

    if ($num_part_found == 0)
    {
        die "\n[Error]unsupported part number $PartNum\n" ;
    }
    
    $iter = 0;
    while ($iter<$num_part_found)
    {

        $_ = $rows_part_found[$iter] ;
        $iter ++;

        $VENDOR[$id] = &xls_cell_value($Sheet, $_, $COLUMN_VENDOR) ;

        $DENSITY[$id] = &xls_cell_value($Sheet, $_, $COLUMN_DENSITY) ;


        # find the correct platform
        my $platform_scan_idx = $COLUMN_PLATFORM ; #First EMI controller

        my $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx) ;
        while (!($tmp_platform =~ $PLATFORM))
        {
            $platform_scan_idx++;
            $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx);
            if ($platform_scan_idx > 100)
            {
                print "[ERROR][Porting Error] It cannot find the right platform name.\n";
                exit;
            }
        }
        &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: $PLATFORM not support LPSDRAM!", __FILE__, __LINE__) if ($platform_scan_idx > $COLUMN_PLATFORM);

        $DEV_TYPE[$id]                       = &xls_cell_value($Sheet, $_, $COLUMN_TYPE) ;
        $NAND_EMMC_ID[$id]                   = &xls_cell_value($Sheet, $_, $COLUMN_NAND_EMMC_ID) ;
        $FW_ID[$id]                          = &xls_cell_value($Sheet, $_, $COLUMN_FW_ID) ;
        $NAND_PAGE_SIZE[$id]                 = &xls_cell_value($Sheet, $_, $COLUMN_NAND_PAGE_SIZE) ;

        $EMI_CONA_VAL[$id]                   = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_DRVCTL0_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_DRVCTL1_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_DLE_VAL[$id]                  = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_ACTIM_VAL[$id]                = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_GDDR3CTL1_VAL[$id]            = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_CONF1_VAL[$id]                = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_DDR2CTL_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_TEST2_3_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_CONF2_VAL[$id]                = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_PD_CTRL_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_PADCTL3_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_DQODLY_VAL[$id]               = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_ADDR_OUTPUT_DLY[$id]          = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_CLK_OUTPUT_DLY[$id]           = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_ACTIM1_VAL[$id]               = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        $DRAMC_MISCTL0_VAL[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;


        $DDR1_2_3[$id]                        = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;

        #  openoffice saved file workaround

        if ($DEV_TYPE[$id] =~ /DDR1/)
        {
            $DDR1_2_3[$id] = 'DDR1';
        }
        elsif ($DEV_TYPE[$id] =~ /DDR2/)
        {
            $DDR1_2_3[$id] = 'DDR2';
        }
        elsif ($DEV_TYPE[$id] =~ /DDR3/)
        {
            $DDR1_2_3[$id] = 'DDR3';
        }

#union
#1 DDR2
        if ($DDR1_2_3[$id] eq "DDR2")
        {
            $DDR2_MODE_REG1[$id]             = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR2_MODE_REG2[$id]             = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR2_MODE_REG3[$id]             = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR2_MODE_REG10[$id]            = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR2_MODE_REG63[$id]            = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        }
#2 DDR1
        elsif ($DDR1_2_3[$id] eq "DDR1")
        {
            $DDR1_MODE_REG[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR1_EXT_MODE_REG[$id]          = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        }
#3 DDR3    
        elsif ($DDR1_2_3[$id] eq "DDR3")
        {
            $DDR3_MODE_REG0[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR3_MODE_REG1[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR3_MODE_REG2[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
            $DDR3_MODE_REG3[$id]              = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
        }
        if ($DENSITY[$id] eq "8192+8192")
        {
            $DRAM_RANK0_SIZE[$id] = "0x40000000";
            $DRAM_RANK1_SIZE[$id] = "0x40000000";
        }
        elsif ($DENSITY[$id] eq "8192+4096")
        {
            $DRAM_RANK0_SIZE[$id] = "0x40000000";
            $DRAM_RANK1_SIZE[$id] = "0x20000000";
        }
        elsif ($DENSITY[$id] eq "4096+4096")
        {
            $DRAM_RANK0_SIZE[$id] = "0x20000000";
            $DRAM_RANK1_SIZE[$id] = "0x20000000";
        }
        elsif ($DENSITY[$id] eq "2048+2048")
        {
            $DRAM_RANK0_SIZE[$id] = "0x10000000";
            $DRAM_RANK1_SIZE[$id] = "0x10000000";
        }
        elsif ($DENSITY[$id] eq "4096+2048")
        {
            $DRAM_RANK0_SIZE[$id] = "0x20000000";
            $DRAM_RANK1_SIZE[$id] = "0x10000000";
        }
        elsif ($DENSITY[$id] eq "8192")
        {
            $DRAM_RANK0_SIZE[$id] = "0x40000000";
            $DRAM_RANK1_SIZE[$id] = "0";
        }
        elsif ($DENSITY[$id] eq "4096")
        {
            $DRAM_RANK0_SIZE[$id] = "0x20000000";
            $DRAM_RANK1_SIZE[$id] = "0";
        }
        elsif ($DENSITY[$id] eq "2048")
        {
            $DRAM_RANK0_SIZE[$id] = "0x10000000";
            $DRAM_RANK1_SIZE[$id] = "0";
        }else
        {
            die "[ERROR]Wrong Density size!!DENSITY:$DENSITY[$id]\n";
        }

        $hex_val = hex($EMI_CONA_VAL[$id]);
        if ($DENSITY[$id] =~ /\+/)
        {
            if (!($hex_val & 0x20000)) 
            {
                die "[ERROR] Wrong CONA value with dual rank:$DENSITY[$id],$EMI_CONA_VAL[$id]"
            }
        }
        else
        {
            if ($hex_val & 0x20000) 
            {
                die "[ERROR] Wrong CONA value with single rank:$DENSITY[$id],$EMI_CONA_VAL[$id]";
            }
        }

        if ($DEV_TYPE[$id] eq "Discrete DDR1")
        {
            $DEV_TYPE1[$id] = "00" ;
            $DEV_TYPE2[$id] = "01" ;
        }
        elsif ($DEV_TYPE[$id] eq "Discrete DDR2")
        {
            $DEV_TYPE1[$id] = "00" ;
            $DEV_TYPE2[$id] = "02" ;
        }
        elsif ($DEV_TYPE[$id] eq "Discrete DDR3")
        {
            $DEV_TYPE1[$id] = "00" ;
            $DEV_TYPE2[$id] = "03" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(NAND+DDR1)")
        {
            $DEV_TYPE1[$id] = "01" ;
            $DEV_TYPE2[$id] = "01" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(NAND+DDR2)")
        {
            $DEV_TYPE1[$id] = "01" ;
            $DEV_TYPE2[$id] = "02" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(NAND+DDR3)")
        {
            $DEV_TYPE1[$id] = "01" ;
            $DEV_TYPE2[$id] = "03" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(eMMC+DDR1)")
        {
            $DEV_TYPE1[$id] = "02" ;
            $DEV_TYPE2[$id] = "01" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(eMMC+DDR2)")
        {
            $DEV_TYPE1[$id] = "02" ;
            $DEV_TYPE2[$id] = "02" ;
        }
        elsif ($DEV_TYPE[$id] eq "MCP(eMMC+DDR3)")
        {
            $DEV_TYPE1[$id] = "02" ;
            $DEV_TYPE2[$id] = "03" ;
        }
        else
        {
            die "[Error]unknown mcp type $DEV_TYPE[$id] \n" ;
        }


        print "NAND_EMMC_ID:$NAND_EMMC_ID[$id]\n";
        # To parse NAND_EMMC_ID, we only support 12 bytes ID
        if (length($NAND_EMMC_ID[$id]) % 2) 
        {
            die "[Error] The wrong NAND_EMMC_ID: $NAND_EMMC_ID[$id] !!"; 
        }
        $ID_String[$id] = "{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}" ;
        $FW_ID_String[$id] = "{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}" ;
        $Sub_version[$id] = "0x0";
        $ID_Length[$id] = (length($NAND_EMMC_ID[$id])-2)/2; 
        $FW_ID_Length[$id] = (length($FW_ID[$id])-2)/2; 
        print $ID_Length[$id].$FW_ID_Length[$id];
=begin
        $_ = $NAND_EMMC_ID[$id] ;
        $ID_Length[$id] = (length($NAND_EMMC_ID[$id])-2)/2; 
        my @FW_ID_VECTOR;
        my $fw_id_len=0;
        my $nand_emmc_id_len=0;
	my $i;


        print "To parse EMMC ID & FW ID:$DEV_TYPE1[$id],$ID_Length[$id]\n";
        if ($DEV_TYPE1[$id] == "00" || $DEV_TYPE1[$id] == "01") 
        {
            # Do nothing for NAND FW ID
            $nand_emmc_id_len = $ID_Length[$id];
            $fw_id_len = 0;
            @FW_ID_VECTOR = "";
            # The sub_version will be 1 once we have id_length
            if($ID_Length[$id] == 9)
            {
                $Sub_version[$id] = "0x0";
            }
            else
            {
                $Sub_version[$id] = "0x1";
            }
        }
        elsif ($DEV_TYPE1[$id] == "02" ) 
        {
            $nand_emmc_id_len = $USE_EMMC_ID_LEN;
            $fw_id_len = $ID_Length[$id] - $nand_emmc_id_len;
            @FW_ID_VECTOR = @NAND_VECTOR[$USE_EMMC_ID_LEN...($ID_Length[$id]-1)];
            foreach(1...$fw_id_len)
            {
                pop(@NAND_VECTOR);
            } 
            print "EMMC_FW_ID:@FW_ID_VECTOR\n" ;
            # for previous emmc version only check 9 bytes.
            if($ID_Length[$id] == 9)
            {
                $Sub_version[$id] = "0x0";
            }
            else
            {
                $Sub_version[$id] = "0x1";
            }
        }
        if ($fw_id_len > $MAX_FW_ID_LEN)
        {
            die("[ERROR] The FW ID number is larger then 8.");
        }
=cut
        if ($ID_Length[$id]> 0)
        {
            my @NAND_VECTOR = ($NAND_EMMC_ID[$id] =~ m/([\dA-Fa-f]{2})/gs);
            #create NAND_EMMC_ID string
            $last = pop(@NAND_VECTOR);
            $ID_String[$id] = "{";
            foreach $a (@NAND_VECTOR)
            {
                $ID_String[$id] .= "0x$a,";
            }
            $ID_String[$id] .= "0x$last";
            # To add ZERO in the end
            if ($nand_emmc_id_len < $MAX_NAND_EMMC_ID_LEN)
            {
                for($i = 0; $i <($MAX_NAND_EMMC_ID_LEN - $ID_Length[$id]); $i++)
                {
                    $ID_String[$id].= ",0x0" 
                }
            }
            $ID_String[$id] .= "}";
        }
        else{
            $ID_Length[$id] = 0;
        }
        print "$ID_String[$id]\n" ;


        #create FW ID string
        if ($FW_ID_Length[$id] > 0)
        {
            my @FW_ID_VECTOR = ($FW_ID[$id] =~ m/([\dA-Fa-f]{2})/gs);
            $last = pop(@FW_ID_VECTOR);
            $FW_ID_String[$id] = "{";
            foreach $a (@FW_ID_VECTOR)
            {
                $FW_ID_String[$id] .= "0x$a,";
            }
            $FW_ID_String[$id] .= "0x$last";
            # To add ZERO in the end
            if ($fw_id_len < $MAX_FW_ID_LEN)
            {
                for($i = 0; $i <($MAX_FW_ID_LEN - $FW_ID_Length[$id]); $i++)
                {
                    $FW_ID_String[$id].= ",0x0" 
                }
            }
            $FW_ID_String[$id] .= "}";
        }
        else{
            $FW_ID_Length[$id] = 0;
        }
        print "$FW_ID_String[$id]\n" ;

        $EMI_SETTINGS[$id] = "\n\t//$PartNum\n\t{\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $Sub_version[$id] . ",\t\t/* sub_version */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . "0x" . $DEV_TYPE1[$id] . $DEV_TYPE2[$id] . ",\t\t/* TYPE */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $ID_Length[$id] . ",\t\t/* EMMC ID/FW ID checking length */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $FW_ID_Length[$id] . ",\t\t/* FW length */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $ID_String[$id] . ",\t\t/* NAND_EMMC_ID */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $FW_ID_String[$id] . ",\t\t/* FW_ID */\n\t\t" ;

        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONA_VAL[$id] . ",\t\t/* EMI_CONA_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_DRVCTL0_VAL[$id] . ",\t\t/* DRAMC_DRVCTL0_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_DRVCTL1_VAL[$id] . ",\t\t/* DRAMC_DRVCTL1_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_DLE_VAL[$id] . ",\t\t/* DRAMC_DLE_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_ACTIM_VAL[$id] . ",\t\t/* DRAMC_ACTIM_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_GDDR3CTL1_VAL[$id] . ",\t\t/* DRAMC_GDDR3CTL1_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_CONF1_VAL[$id] . ",\t\t/* DRAMC_CONF1_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_DDR2CTL_VAL[$id] . ",\t\t/* DRAMC_DDR2CTL_VAL */\n\t\t" ;    
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_TEST2_3_VAL[$id] . ",\t\t/* DRAMC_TEST2_3_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_CONF2_VAL[$id] . ",\t\t/* DRAMC_CONF2_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_PD_CTRL_VAL[$id] . ",\t\t/* DRAMC_PD_CTRL_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_PADCTL3_VAL[$id] . ",\t\t/* DRAMC_PADCTL3_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_DQODLY_VAL[$id] . ",\t\t/* DRAMC_DQODLY_VAL */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_ADDR_OUTPUT_DLY[$id] . ",\t\t/* DRAMC_ADDR_OUTPUT_DLY */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_CLK_OUTPUT_DLY[$id] . ",\t\t/* DRAMC_CLK_OUTPUT_DLY */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_ACTIM1_VAL[$id] . ",\t\t/* DRAMC_ACTIM1_VAL*/\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DRAMC_MISCTL0_VAL[$id] . ",\t\t/* DRAMC_MISCTL0_VAL*/\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . "{" . $DRAM_RANK0_SIZE[$id] . "," .  $DRAM_RANK1_SIZE[$id] . ",0,0},\t\t/* DRAM RANK SIZE */\n\t\t" ;
        $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . "{0,0,0,0,0,0,0,0,0,0},\t\t/* reserved 10 */\n\t\t" ;

#union
#1 DDR2
        if ($DDR1_2_3[$id] eq "DDR2")
        {    
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR2_MODE_REG1[$id] . ",\t\t/* DDR2_MODE_REG1 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR2_MODE_REG2[$id] . ",\t\t/* DDR2_MODE_REG2 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR2_MODE_REG3[$id] . ",\t\t/* DDR2_MODE_REG3 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR2_MODE_REG10[$id] . ",\t\t/* DDR2_MODE_REG10 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR2_MODE_REG63[$id] . "\t\t/* DDR2_MODE_REG63 */\n\t}" ;
        }
#2 DDR1
        elsif ($DDR1_2_3[$id] eq "DDR1")
        {
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR1_MODE_REG[$id] . ",\t\t/* DDR1_MODE_REG */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR1_EXT_MODE_REG[$id] . "\t\t/* DDR1_EXT_MODE_REG */\n\t}" ;
        }
#3 DDR3    
        elsif ($DDR1_2_3[$id] eq "DDR3")
        {
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR3_MODE_REG0[$id] . ",\t\t/* DDR3_MODE_REG0 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR3_MODE_REG1[$id] . ",\t\t/* DDR3_MODE_REG1 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR3_MODE_REG2[$id] . ",\t\t/* DDR3_MODE_REG2 */\n\t\t" ;
            $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $DDR3_MODE_REG3[$id] . ",\t\t/* DDR3_MODE_REG3 */\n\t}" ;
        }   
        print  $EMI_SETTINGS[$id] ;
        print "\n\n" ;
        $id ++;
    }

    # return total_part_number_iter
    return $id;
    
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
    die "[Error]does not support windows now!!" ;
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

  print $MEMORY_DEVICE_TYPE ;

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
    return &trim(&win_xls_cell_value($Sheet, $row, $col));
  }
  else {
      return &trim(&lin_xls_cell_value($Sheet, $row, $col));
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
  return "" unless (defined $cell);
  my $value = $cell->Value();

}

sub write_tag()
{
    my $project = lc($_[0]);
    my $filesize = 0x0 ;
    my $ddr = -1 ;

    if (-e $INFO_TAG)
    {
        unlink ($INFO_TAG);
    }
    
    open FILE,">$INFO_TAG";
    print FILE pack("a24", "MTK_BLOADER_INFO_v10");
    $filesize = $filesize + 24 ;
    seek(FILE, 0x1b, 0);
    $pre_bin = "preloader_${project}.bin";
    print "PROJECT = $project, pre_bin = $pre_bin\n";
    print FILE pack("a64", $pre_bin); 
    $filesize = $filesize + 64 ;
    seek(FILE, 0x58, 0);
    print FILE pack("H8", 56313136);
    $filesize = $filesize + 4 ;
    print FILE pack("H8", 22884433);
    $filesize = $filesize + 4 ;
    print FILE pack("H8", "90007000");
    $filesize = $filesize + 4 ;
    print FILE pack("a8", "MTK_BIN");
    $filesize = $filesize + 8 ;
    
#    print FILE pack("H8", bc000000);
    
 
    seek(FILE,0x6c, 0);
    
    print FILE pack("L", hex($TotalCustemChips));     # number of emi settings.
    $filesize = $filesize + 4 ;
    
    my $iter = 0 ;
    while ($iter < $TotalCustemChips)
    {
        if ($DEV_TYPE1[$iter] != "00")
        {
            $filesize = $filesize + &write_tag_one_element ($iter) ;
        }
        else
        {
            $ddr = $iter ;
        }
            
        $iter = $iter + 1 ;
    }
    if ($ddr != -1)
    {
        $filesize = $filesize + &write_tag_one_element ($ddr) ;
    }
#    $filesize = $filesize + 4 ;
    
#    print "2.file size is $filesize \n";

    print FILE pack("L", $filesize) ;
    
    close (FILE) ;
    print "$INFO_TAG is generated!\n";
    return ;
}

sub write_tag_one_element()
{
    my $id = $_[0];
    my $type = "0x$DEV_TYPE1[$id]$DEV_TYPE2[$id]" ;
    my $fs = 0 ;

    print FILE pack ("L", hex (lc($Sub_version[$id]))) ;        # Sub_version checking for flash tool
    $fs = $fs + 4 ;

    print FILE pack("L", hex($type));                           #type
    $fs = $fs + 4 ;

    print FILE pack ("L", scalar($ID_Length[$id])) ;        # EMMC ID checking length
    $fs = $fs + 4 ;

    print FILE pack ("L", scalar($FW_ID_Length[$id])) ;        # FW ID checking length
    $fs = $fs + 4 ;
    
    $_ = $ID_String[$id] ;
    if (/(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+)/)
    {
        print FILE pack ("C", hex ($1)) ;            #id
        print FILE pack ("C", hex ($2)) ;
        print FILE pack ("C", hex ($3)) ;
        print FILE pack ("C", hex ($4)) ;
        print FILE pack ("C", hex ($5)) ;
        print FILE pack ("C", hex ($6)) ;
        print FILE pack ("C", hex ($7)) ;
        print FILE pack ("C", hex ($8)) ;
        print FILE pack ("C", hex ($9)) ;
        print FILE pack ("C", hex ($10)) ;
        print FILE pack ("C", hex ($11)) ;
        print FILE pack ("C", hex ($12)) ;
        print FILE pack ("C", hex ($13)) ;
        print FILE pack ("C", hex ($14)) ;
        print FILE pack ("C", hex ($15)) ;
        print FILE pack ("C", hex ($16)) ;
        $fs = $fs + 16 ;
    }
    $_= $FW_ID_String[$id];
    if (/(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+),(\w+)/)
    {
        print FILE pack ("C", hex ($1)) ;            #fw id
        print FILE pack ("C", hex ($2)) ;
        print FILE pack ("C", hex ($3)) ;
        print FILE pack ("C", hex ($4)) ;
        print FILE pack ("C", hex ($5)) ;
        print FILE pack ("C", hex ($6)) ;
        print FILE pack ("C", hex ($7)) ;
        print FILE pack ("C", hex ($8)) ;
        $fs = $fs + 8 ;
    }
    
    print FILE pack ("L", hex (lc($EMI_CONA_VAL[$id]))) ;         # EMI_CONA_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_DRVCTL0_VAL[$id]))) ;    # DRAMC_DRVCTL0_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_DRVCTL1_VAL[$id]))) ;    # DRAMC_DRVCTL1_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_DLE_VAL[$id]))) ;        # DRAMC_DLE_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_ACTIM_VAL[$id]))) ;      # DRAMC_ACTIM_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_GDDR3CTL1_VAL[$id]))) ;  # DRAMC_GDDR3CTL1_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_CONF1_VAL[$id]))) ;      # DRAMC_CONF1_VAL
    $fs = $fs + 4 ;

    print FILE pack ("L", hex (lc($DRAMC_DDR2CTL_VAL[$id]))) ;    # DRAMC_DDR2CTL_VAL
    $fs = $fs + 4 ;

    print FILE pack ("L", hex (lc($DRAMC_TEST2_3_VAL[$id]))) ;    # DRAMC_TEST2_3_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_CONF2_VAL[$id]))) ;      # DRAMC_CONF2_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_PD_CTRL_VAL[$id]))) ;    # DRAMC_PD_CTRL_VAL
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_PADCTL3_VAL[$id]))) ;    # DRAMC_PADCTL3_VAL
    
    $fs = $fs + 4 ;
    print FILE pack ("L", hex (lc($DRAMC_DQODLY_VAL[$id]))) ;     # DRAMC_DQODLY_VAL
    $fs = $fs + 4 ;

    print FILE pack ("L", hex (lc($DRAMC_ADDR_OUTPUT_DLY[$id]))) ;        # DRAMC_ADDR_OUTPUT_DLY
    $fs = $fs + 4 ;

    print FILE pack ("L", hex (lc($DRAMC_CLK_OUTPUT_DLY[$id]))) ;        # DRAMC_CLK_OUTPUT_DLY
    $fs = $fs + 4 ;
    
    print FILE pack ("L", hex (lc($DRAMC_ACTIM1_VAL[$id]))) ;           # $DRAMC_ACTIM1_VAL
    $fs = $fs + 4 ;

    print FILE pack ("L", hex (lc($DRAMC_MISCTL0_VAL[$id]))) ;        # DRAMC_MISCTL0_VAL
    $fs = $fs + 4 ;

    print FILE pack ("L", hex ("0")) ;                                  #  DRAM_RANK_SIZE[4]
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;

    print FILE pack ("L", hex ("0")) ;                                  #reserved[10]
    print FILE pack ("L", hex ("0")) ;  
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    print FILE pack ("L", hex ("0")) ;
    $fs = $fs + 56 ;
    $_ = $FW_ID_String[$id] ;
        
    if ($DEV_TYPE2[$id] == "02")
    {
        #ddr2
        print FILE pack ("L", hex (lc($DDR2_MODE_REG1[$id]))) ;        # DDR2_MODE_REG1
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR2_MODE_REG2[$id]))) ;        # DDR2_MODE_REG2
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR2_MODE_REG3[$id]))) ;        # DDR2_MODE_REG3
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR2_MODE_REG10[$id]))) ;        # DDR2_MODE_REG10
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR2_MODE_REG63[$id]))) ;        # DDR2_MODE_REG63
        $fs = $fs + 4 ;
    }
    elsif ($DEV_TYPE2[$id] == "01")
    {   # ddr1
        print FILE pack ("L", hex (lc($DDR1_MODE_REG[$id]))) ;        # DDR1_MODE_REG;
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR1_EXT_MODE_REG[$id]))) ;        # DDR1_EXT_MODE_REG
        $fs = $fs + 4 ;

        print FILE pack ("L", hex ("0")) ;        
        $fs = $fs + 4 ;

        print FILE pack ("L", hex ("0")) ;        
        $fs = $fs + 4 ;

        print FILE pack ("L", hex ("0")) ;        
        $fs = $fs + 4 ;
    }
    elsif ($DEV_TYPE2[$id] == "03")
    {   # ddr3
        print FILE pack ("L", hex (lc($DDR3_MODE_REG0[$id]))) ;        # DDR3_MODE_REG0
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR3_MODE_REG1[$id]))) ;        # DDR3_MODE_REG1
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR3_MODE_REG2[$id]))) ;        # DDR3_MODE_REG2       
        $fs = $fs + 4 ;

        print FILE pack ("L", hex (lc($DDR3_MODE_REG3[$id]))) ;        # DDR3_MODE_REG3     
        $fs = $fs + 4 ;

        print FILE pack ("L", hex ("0")) ;        
        $fs = $fs + 4 ;
    }
#       print "1.file size is $fs \n";
    return $fs;

}
