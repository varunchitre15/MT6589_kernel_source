#!/usr/local/bin/perl
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


#   original design, but perl does not support array of structure, really?
#
#my $CustomChip = () ;
#
#
#       an array of following structure:
#
#       CustChip => NAND_ID
#                => CS0_PART_NUMBER
#                => CS1_PART_NUMBER
#
#
#
#
#
#




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
	'MT6573'  => 1,
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

my $COLUMN_BOARD_ID             = $start_num + 3;
my $COLUMN_NAND_ID             = $start_num + 4;

my $COLUMN_PLATFORM               = $start_num + 5;

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
    #$MEMORY_INFO_H =~ s/custom_MemoryDevice.h$/memory_info\.h/i
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

my $TotalCustemChips = 0 ;

#
#   arrays
#
#   this should be an array of structurs, but it is said perl does not support it.
#   these are input, except EMI_GEND
#
my $CustNAND_ID ;
my $CustCS0_PART_NUMBER ;
my $CustCS1_PART_NUMBER ;
my $CustCS2_PART_NUMBER ;
my $CustCS3_PART_NUMBER ;
my $Boot_Bank1_MT6573 ;
my $EMIGEND ;
my $VENDOR ;
my $DENSITY ;
my $LPSDRAM_CHIP_SELECT ;
my $Boot_Bank1_MT6573 ; # only boot from BANK1 for single memory device
  						   # 0, don't boot from BANK1
  						   # 1, boot from BANK1
my $Bank1_Swap ;

my $EMI_CONI_VAL ;
my $EMI_DRVA_VAL ;
my $EMI_DRVB_VAL ;
my $EMI_CONJ_VAL ;
my $EMI_CONK_VAL ;
my $EMI_CONL_VAL ;
my $EMI_CONN_VAL ;
my $EMI_IOCL_VAL ;
my $EMI_GENA_VAL ;
my $EMI_DRCT_VAL ;
my $EMI_PPCT_VAL ;
my $EMI_SLCT_VAL ;
my $EMI_ABCT_VAL ;
my $EMI_DUTB_VAL ;

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
    if (/^#define\s+(\w+)\[(\d+)\]\s+\((\w*)\)/ || /^#define\s+(\w+)\[(\d+)\]\s+(\w*)/ || 
        /^#define\s+(MEMORY_DEVICE_TYPE)\s+\((\w*)\)/ || /^#define\s+(MEMORY_DEVICE_TYPE)\s+(\w*)/ ||
        /^#define\s+(BOARD_ID)\s+\((\w*)\)/ || /^#define\s+(BOARD_ID)\s+(\w*)/) 
    {
#        print "\n $1, $2, $3\n" ;
        
        if ($1 eq "MEMORY_DEVICE_TYPE")
        {
            if (!defined $2)
            {
                $CUSTOM_MEM_DEV_OPTIONS{$1} = 'TRUE' ;
            }
            else
            {
              $CUSTOM_MEM_DEV_OPTIONS{$1} = $2 ;
            }
            #print $CUSTOM_MEM_DEV_OPTIONS{$1} ;
        }
        elsif ($1 eq "BOARD_ID")
        {
            $CustBoard_ID = $2 ;
        }
        elsif ($1 eq "NAND_ID")
        {
            #print "\nNAND_ID $2, $3\n" ;
            $CustNAND_ID[$2] = $3 ;
            $TotalCustemChips = $TotalCustemChips + 1 ;
        }
        elsif ($1 eq "CS0_PART_NUMBER")
        {
            #print "\nCS0 $2, $3\n" ;
            $CustCS0_PART_NUMBER[$2] = $3 ;
            $LPSDRAM_CHIP_SELECT[$2] = 0 ;
            if ($CustCS1_PART_NUMBER[$2] eq undef && $CustCS2_PART_NUMBER[$2] eq undef && $CustCS3_PART_NUMBER[$2] eq undef)
            {
                $TotalCustemChips = $TotalCustemChips + 1 ;    
            }
        }
        elsif ($1 eq "CS1_PART_NUMBER")
        {
            print "\nCS1 $1, $2, $3\n" ;
            $CustCS1_PART_NUMBER[$2] = $3 ;
            $LPSDRAM_CHIP_SELECT[$2] = 1 ;
            if ($CustCS0_PART_NUMBER[$2] eq undef && $CustCS2_PART_NUMBER[$2] eq undef && $CustCS3_PART_NUMBER[$2] eq undef)
            {
                $TotalCustemChips = $TotalCustemChips + 1 ;    
            }
        }
        elsif ($1 eq "CS2_PART_NUMBER")
        {
            #print "\nCS2 $2, $3\n" ;
            $CustCS2_PART_NUMBER[$2] = $3 ;
            #$LPSDRAM_CHIP_SELECT[$2] = 1 ;
            if ($CustCS0_PART_NUMBER[$2] eq undef && $CustCS1_PART_NUMBER[$2] eq undef && $CustCS3_PART_NUMBER[$2] eq undef)
            {
                $TotalCustemChips = $TotalCustemChips + 1 ;    
            }
        }
        elsif ($1 eq "CS3_PART_NUMBER")
        {
            #print "\nCS3 $2, $3\n" ;
            $CustCS3_PART_NUMBER[$2] = $3 ;
            #$LPSDRAM_CHIP_SELECT[$2] = 1 ;
            if ($CustCS0_PART_NUMBER[$2] eq undef && $CustCS1_PART_NUMBER[$2] eq undef && $CustCS2_PART_NUMBER[$2] eq undef)
            {
                $TotalCustemChips = $TotalCustemChips + 1 ;    
            }
        }
    }
}
print "\n$TotalCustemChips\n" if ($DebugPrint ==1);
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
if ($TotalCustemChips > 30)
{
    die "\nTotalCustemChips($TotalCustemChips) > 30\n" ;
}
my $iter = 0 ;
while ($iter<$TotalCustemChips)
{
    print "\niter:$iter\n" if ($DebugPrint ==1);
    print "\nCustNAND_ID[$iter]:$CustNAND_ID[$iter]\n" if ($DebugPrint ==1);
    #if ($CustCS0_PART_NUMBER[$iter] eq undef && $CustCS1_PART_NUMBER[$iter] eq undef)
    #{
    #    die "\nundefined CS0 and CS1 for NAND ID$iter\n" ;
    #}
# generate EMI_GEND
    $Bank1_Swap[$iter] = "" ;
    $bank_num[$iter] = 0 ;
    $EMIGEND[$iter] = "" ;
    my $emigendtmp = 0 ;
    if (defined($CustCS0_PART_NUMBER[$iter]))
    {
        print "\n$iter, cs0\n"; 
        $bank_num[$iter] = $bank_num[$iter] + 1 ;
        $Boot_Bank1_MT6573[$iter] = 0;
        $emigendtmp = $emigendtmp | (1 << (0+16)) ; 
    }
    if (defined($CustCS1_PART_NUMBER[$iter]))
    {
        print "\n$iter, cs1:$CustCS1_PART_NUMBER[$iter]\n"; 
        $bank_num[$iter] = $bank_num[$iter] + 1 ;
        $emigendtmp = $emigendtmp | (1 << (1+16)) ; 
        $Boot_Bank1_MT6573[$iter] = 1;
        $Bank1_Swap[$iter] = &bank1_swap_func();
    }
    if (defined($CustCS2_PART_NUMBER[$iter]))
    {
        print "\n$iter, cs2\n"; 
        $bank_num[$iter] = $bank_num[$iter] + 1 ;
        $emigendtmp = $emigendtmp | (1 << (2+16)) ; 
    }
    if (defined($CustCS3_PART_NUMBER[$iter]))
    {
        print "\n$iter, cs3\n"; 
        $bank_num[$iter] = $bank_num[$iter] + 1 ;
        $emigendtmp = $emigendtmp | (1 << (3+16)) ; 
    }
    if ((defined($CustCS0_PART_NUMBER[$iter])) && (defined($CustCS1_PART_NUMBER[$iter])))
    {
        $Boot_Bank1_MT6573[$iter] = 0 ;
        $Bank1_Swap[$iter] = "" ;
        if ($CustCS0_PART_NUMBER[$iter] ne $CustCS1_PART_NUMBER[$iter])
        {
            die "$PLATFORM: need to use the same memory device for two banks!!\n" ;
        }
    }
    $emigendtmp = (~($emigendtmp >> 16)&0xF) | $emigendtmp ;
    $EMIGEND[$iter] = sprintf ("0x%x", $emigendtmp) ;
    
    print "\nemigen:$EMIGEND[$iter]\n" ;
    
    my $tempstr ;
    if (defined($CustCS0_PART_NUMBER[$iter]))
    {
        $tempstr = $CustCS0_PART_NUMBER[$iter] ;
    }
    elsif(defined($CustCS1_PART_NUMBER[$iter]))
    {
        $tempstr = $CustCS1_PART_NUMBER[$iter] ;
    }
    elsif(defined($CustCS2_PART_NUMBER[$iter]))
    {
        $tempstr = $CustCS2_PART_NUMBER[$iter] ;
    }
    elsif(defined($CustCS3_PART_NUMBER[$iter]))
    {
        $tempstr = $CustCS3_PART_NUMBER[$iter] ;
    }
    else
    {
        die "\none ofCS0,1,2,3 must defined!!!\n" ;
    }
    
    if(defined($CustCS1_PART_NUMBER[$iter]))
    {
        if ($tempstr ne $CustCS1_PART_NUMBER[$iter])
        {
            die "\ndifferent part number!!!\n" ;
        }
    }

    if(defined($CustCS2_PART_NUMBER[$iter]))
    {
        if ($tempstr ne $CustCS2_PART_NUMBER[$iter])
        {
            die "\ndifferent part number!!!\n" ;
        }
    }

    if(defined($CustCS3_PART_NUMBER[$iter]))
    {
        if ($tempstr ne $CustCS3_PART_NUMBER[$iter])
        {
            die "\ndifferent part number!!!\n" ;
        }
    }

  
    print "EMI_GEND is $EMIGEND[$iter]\n" if ($DebugPrint ==1);
    if ($DebugPrint == 1)
    {
        print "$CustCS0_PART_NUMBER[$iter] , $CustCS1_PART_NUMBER[$iter] , $CustCS2_PART_NUMBER[$iter] , $CustCS3_PART_NUMBER[$iter] \n";
    }
    $iter = $iter + 1 ;
}
 
$MEMORY_DEVICE_TYPE              = $CUSTOM_MEM_DEV_OPTIONS{MEMORY_DEVICE_TYPE};
if ($DebugPrint == 1)
{
    print "Device Type : $MEMORY_DEVICE_TYPE  , ";
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
    $iter = 0 ;
    while ($iter<$TotalCustemChips)
    {
        &DeviceListParser_LPSDRAM($iter);
        $iter = $iter + 1 ;
    }
    
    #check duplicated NAND ID
    $iter = 0 ;
    while ($iter < $TotalCustemChips)
    {
        my $iter2 = $iter + 1 ;
        while ($iter2 < $TotalCustemChips)
        {
            if ($CustNAND_ID[$iter] eq $CustNAND_ID[$iter2])
            {
                die "\n duplicated nand id part $iter and part $iter2!!\n" ;
            }
            $iter2 = $iter2 + 1 ;
        }
        $iter = $iter + 1 ;
    }
}
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
#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: part number not supported!", __FILE__, __LINE__)    if ($is_part_found <= 0);

#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_RAM_MCP type!", __FILE__, __LINE__)     if (($MEMORY_DEVICE_TYPE eq 'NOR_RAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'YES'));
#&error_handler("$CUSTOM_MEMORY_DEVICE_HDR: Device not allowed to set NOR_LPSDRAM_MCP type!", __FILE__, __LINE__) if (($MEMORY_DEVICE_TYPE eq 'NOR_LPSDRAM_MCP') && ($part_info->{CS}->{1}->{DRAM} eq 'NO'));



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
#    print "\n$CUSTOM_EMI_H need not to be generated\n";
} # if ($is_existed_h == 0)

##****************************************************************************
## generate memory_info.h
##****************************************************************************
#if ($is_existed_h == 0)
{
#    print "\n\n $MEMORY_INFO_H \n\n" ;
    
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
    ###
    my $template = <<"__TEMPLATE";
#ifndef __CUSTOM_EMI__
#define __CUSTOM_EMI__

struct EMI_SETTINGS{
  unsigned short  NAND_ID;
  unsigned int  EMI_CONI_VAL;
  unsigned int  EMI_DRVA_VAL; 
  unsigned int  EMI_DRVB_VAL;      
  unsigned int  EMI_CONJ_VAL;      
  unsigned int  EMI_CONK_VAL;      
  unsigned int  EMI_CONL_VAL;      
  unsigned int  EMI_IOCL_VAL;      
  unsigned int  EMI_GENA_VAL;      
  unsigned int  EMI_GEND_VAL;      
  unsigned int  EMI_DRCT_VAL;      
  unsigned int  EMI_PPCT_VAL;      
  unsigned int  EMI_SLCT_VAL;      
  unsigned int  EMI_ABCT_VAL;      
  unsigned int  EMI_DUTB_VAL;
  unsigned int  EMI_CONN_VAL;
};


typedef struct 
{
    unsigned long EMI_CONN_regval;
        
    unsigned long EMI_DQSA_regval;
    unsigned long EMI_DQSB_regval;
    unsigned long EMI_DQSC_regval;
    unsigned long EMI_DQSD_regval;
    unsigned long EMI_DQSE_regval;
    
    unsigned long EMI_IDLA_regval;
    unsigned long EMI_IDLB_regval;
    unsigned long EMI_IDLC_regval;
    unsigned long EMI_IDLD_regval;
    unsigned long EMI_IDLE_regval;
    unsigned long EMI_IDLF_regval;
    unsigned long EMI_IDLG_regval;
    unsigned long EMI_IDLH_regval;
    unsigned long EMI_IDLI_regval;
    
    unsigned long EMI_CALA_regval;
    unsigned long EMI_CALB_regval;
    unsigned long EMI_CALE_regval;
    unsigned long EMI_CALF_regval;
    unsigned long EMI_CALI_regval;
    unsigned long EMI_CALJ_regval;
    unsigned long EMI_CALP_regval;
    
} EMI_DATA_TRAIN_REG_t;

#endif /* __CUSTOM_EMI__ */

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
	my $BankSwap = "" ;
	my $temp = "" ;
    $iter = 0 ;
    while ($iter<$TotalCustemChips)
    {
        $EMI_SETTINGS_string = $EMI_SETTINGS_string . $EMI_SETTINGS[$iter] ;
#       for 6573 bank swap.
#        if ($Bank1_Swap[$iter] ne "")
#        {
#            $BankSwap = $BankSwap. "if (index == $iter) {\n$Bank1_Swap[$iter]\t\t} ;\n"
#        }
        $iter = $iter + 1 ;
        if ($iter < $TotalCustemChips)
        {
            $EMI_SETTINGS_string = $EMI_SETTINGS_string . " ," ;
        }
    }
	
	
	
	my $template = <<"__TEMPLATE";

#include "mt6573.h"
#include "mt6573_typedefs.h"
#include "mt6573_emi_reg.h"
#include "custom_emi.h"

#define NUM_EMI_RECORD $TotalCustemChips

struct EMI_SETTINGS emi_settings[15]=
{
    $EMI_SETTINGS_string
};

extern int _EmiDataTrain( EMI_DATA_TRAIN_REG_t* pResult, int SDRAM_CS);


void Custom_EMI_InitDDR(unsigned short nand_id)
{
  UINT32 i, index, num_record, chose;
  EMI_DATA_TRAIN_REG_t DataTrainResultCS[4];

  num_record = sizeof(emi_settings) /sizeof(struct EMI_SETTINGS);		

  dbg_print("[EMI] num_record = %d\\n", num_record);

  dbg_print("[EMI] NAND ID = %x\\n", nand_id);
  
  for(i=0; i<num_record; i++)
  {
    dbg_print("[EMI] emi_settings[%d].NAND_ID = %x\\n", i, emi_settings[i].NAND_ID);
    if(emi_settings[i].NAND_ID == nand_id)
    {
     index = i;
     break;
    }
  }
  
  if(i == num_record)
  {   
    dbg_print("[ERROR] Can not find specified EMI setting !!! (NAND_ID=0x%x)\\n",nand_id);     
    while(1);          
  }

  
  DRV_WriteReg32(0x70026320, 0x00000000);
  DRV_WriteReg32(EMI_CONI, emi_settings[index].EMI_CONI_VAL);
  
  DRV_WriteReg32(EMI_DRVA, emi_settings[index].EMI_DRVA_VAL); //Need ETT result
  DRV_WriteReg32(EMI_DRVB, emi_settings[index].EMI_DRVB_VAL); //Need ETT result
  
  DRV_WriteReg32(EMI_CONJ, emi_settings[index].EMI_CONJ_VAL);
  DRV_WriteReg32(EMI_CONK, emi_settings[index].EMI_CONK_VAL);
  DRV_WriteReg32(EMI_CONL, emi_settings[index].EMI_CONL_VAL);
  DRV_WriteReg32(EMI_IOCL, emi_settings[index].EMI_IOCL_VAL); //Setup swap function for LPDDR EVB
  DRV_WriteReg32(EMI_GENA, emi_settings[index].EMI_GENA_VAL); //Enable clocks, pause-start signal, external boot
  DRV_WriteReg32(EMI_GEND, emi_settings[index].EMI_GEND_VAL); //Enable DDR CS0 and CS1
  
  DRV_WriteReg32(EMI_CONN, emi_settings[index].EMI_CONN_VAL);
  /* Initialization of DDR devices (steps followed per DDR device datasheet) */
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x10000001)); //Single Pre-charge All
  gpt4_busy_wait_ms (1);
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x08000001)); //Single Auto-refresh 1 Enable
  gpt4_busy_wait_ms (1);
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x04000001)); //Single Auto-refresh 2 Enable
  gpt4_busy_wait_ms (1);
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x02000001)); //Single Load Mode Register
  gpt4_busy_wait_ms (1);
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x01000001)); //Single Extended Mode Register Enable
  gpt4_busy_wait_ms (1);
  DRV_WriteReg32(EMI_CONN, (emi_settings[index].EMI_CONN_VAL | 0x00001117)); //Update EMI_CONN to enable PDN_EN, CAL_EN (data auto-tracking disabled), AP, and Concurrent AP
  gpt4_busy_wait_ms (1);

  DRV_WriteReg32(EMI_DUTB, emi_settings[index].EMI_DUTB_VAL); //This is to resolve EMI IO TX problem in LT_1.90_1.08 (Temp_Vmem_Vcore), and only in LPDDR1 case.

  dbg_print("[EMI] EMI_GEND = %x\\n", DRV_Reg32(EMI_GEND));
  
  // process all DRAM bank 
  chose = DRV_Reg32(EMI_GEND) >> 16;
  
  *EMI_DQSE = 0x0; //Disable auto tracking function
  
  for(i=0; i<4; i++)
  {
    dbg_print("[EMI]choice = %x\\n", chose);
    if((chose & 0x00000001) == 0)
    {
      chose = chose >> 1;
      continue;
    }
    
    if( _EmiDataTrain(&DataTrainResultCS[i], i) == 0)
    {
      dbg_print("[EMI]data training fail = %x\\n", chose);
    }
    
    *EMI_CONN |= DataTrainResultCS[i].EMI_CONN_regval; //Apply DataTrain result for CSi
    *EMI_CONN |= CAL_EN;
  
    *EMI_DQSE |= DataTrainResultCS[i].EMI_DQSE_regval;
    
    *EMI_IDLA = DataTrainResultCS[i].EMI_IDLA_regval;
    *EMI_IDLB = DataTrainResultCS[i].EMI_IDLB_regval;
    *EMI_IDLC = DataTrainResultCS[i].EMI_IDLC_regval;
    *EMI_IDLD = DataTrainResultCS[i].EMI_IDLD_regval;
    *EMI_IDLE = DataTrainResultCS[i].EMI_IDLE_regval;
    *EMI_IDLF = DataTrainResultCS[i].EMI_IDLF_regval;
    *EMI_IDLG = DataTrainResultCS[i].EMI_IDLG_regval;
    *EMI_IDLH = DataTrainResultCS[i].EMI_IDLH_regval;
    
    *EMI_CALA = DataTrainResultCS[i].EMI_CALA_regval;
    *EMI_CALB = DataTrainResultCS[i].EMI_CALB_regval; 
    *EMI_CALE = DataTrainResultCS[i].EMI_CALE_regval;
    *EMI_CALF = DataTrainResultCS[i].EMI_CALF_regval; 
    *EMI_CALI = DataTrainResultCS[i].EMI_CALI_regval;
    *EMI_CALJ = DataTrainResultCS[i].EMI_CALJ_regval; 
    *EMI_CALP = DataTrainResultCS[i].EMI_CALP_regval;

    switch(i)
    {
      case 0:
        dbg_print("[EMI] Set up DQSA\\n");
        *EMI_DQSA = DataTrainResultCS[i].EMI_DQSA_regval; //Apply DataTrain result for CS0
        *EMI_DQSE |= 0x000F;
        break;
      case 1:
        dbg_print("[EMI] Set up DQSB\\n");
        *EMI_DQSB = DataTrainResultCS[i].EMI_DQSB_regval; //Apply DataTrain result for CS1
        *EMI_DQSE |= 0x00F0;
        break;
      case 2:  
        dbg_print("[EMI] Set up DQSC\\n");
        *EMI_DQSC = DataTrainResultCS[i].EMI_DQSC_regval; //Apply DataTrain result for CS2
        *EMI_DQSE |= 0x0F00;
        break;
      case 3:  
        dbg_print("[EMI] Set up DQSD\\n");
        *EMI_DQSD = DataTrainResultCS[i].EMI_DQSD_regval; //Apply DataTrain result for CS3
        *EMI_DQSE |= 0xF000;
        break;
      default:
        ;
    }
    
    *EMI_IDLI = 0x0; 
    
    chose = chose >> 1;
  }

  dbg_print("EMI_DLLV = %x\\n", DRV_Reg32(EMI_DLLV));
  dbg_print("[EMI] EMI_CONN = %x\\n", DRV_Reg32(EMI_CONN));

  DRV_WriteReg32(EMI_PPCT, emi_settings[index].EMI_PPCT_VAL); // Enable EMI_PPCT performance and power control
  DRV_WriteReg32(EMI_SLCT, emi_settings[index].EMI_SLCT_VAL); // EMI_SLCT - Enable R/W command favor for all masters
  DRV_WriteReg32(EMI_ABCT, emi_settings[index].EMI_ABCT_VAL);// Enable 1/32 freq for HWDCM mode and enable arbitration controls (lower_rw, higher_ph, lower_rc)
  gpt4_busy_wait_ms (100);
  
  DRV_WriteReg32(0x70026320, 0x00000000);
  DRV_WriteReg32(0x700FAFB0, 0xC5ACCE55);
  DRV_WriteReg32(0x700FA034, 0x00000001);
  

  if((DRV_Reg32(EMI_GEND)) == 0x2000D)  
  {
    dbg_print("EMI CS remapping... change cs0, c1\\n");
    DRV_WriteReg32(EMI_GENA, 0x30B); //Enable clocks, pause-start signal, external boot 
    DRV_WriteReg32(EMI_DRCT, 0x1); //Enable clocks, pause-start signal, external boot 
  }
  else
  {
    DRV_WriteReg32(EMI_DRCT, emi_settings[index].EMI_DRCT_VAL); // Enable Dummy Read (required for HW DQS auto-tracking)
  }

}

void mt6573_set_emi ()
{
  unsigned short nand_id;
  
  if(NUM_EMI_RECORD <= 0)
  {
      dbg_print("[EMI] There is no EMI settings to initial EMI\\n");	
      while(1);	
  }
  
  if((NUM_EMI_RECORD == 1) && (emi_settings[0].NAND_ID == 0x0))//Have no NAND
  {
    dbg_print("[EMI] Device without NAND\\n");
    nand_id = 0x0;
  }
  else
  {
    getflashid(&nand_id);
  }
  
  dbg_print("[EMI] MT6573 EMI initialize\\n");
  Custom_EMI_InitDDR (nand_id);
}




__TEMPLATE
    return $template ;
}

#****************************************************************************
# subroutine:  memory_info_h_file_body
# return:      
#****************************************************************************
sub memory_info_h_file_body
{
    my $iter = 0 ;
    my $dram_settings = "" ;
    while ($iter < $TotalCustemChips)
    {
        my $density = $DENSITY[$iter] / 8;
        my $size;
        my $size0;
        my $size1;
        my $start1;
#        print "memory_info_h_file_body : $DENSITY[$iter]" ;
        if($density == 128)
        {
            $size = "SZ_128M";
        }
        elsif($density == 256)
        {
            $size = "SZ_256M";
        }
        elsif($density == 384)
        {
            $size = "SZ_256M + SZ_128M";
        }
        
        if($bank_num[$iter] == 1)
        {
            $size0 = $size;
            $size1 = 0;
            $start1 = 0;
        }
        elsif($bank_num[$iter] == 2)
        {
            $size0 = $size;
            $size1 = $size;
            $start1 = "0x10000000";
        }
        $dram_settings = $dram_settings . "\n\t{\n\t\t$CustNAND_ID[$iter],\t\t//NAND_ID" ;
        $dram_settings = $dram_settings . "\n\t\t$bank_num[$iter],\t\t//DRAM_BANKS_NR" ;
        $dram_settings = $dram_settings . "\n\t\t($size0 - RIL_SIZE),\t\t//CFG_PHYS_SDRAM_0_SIZE" ;
        $dram_settings = $dram_settings . "\n\t\tRIL_SIZE,\t\t//CFG_PHYS_SDRAM_0_START" ;
        $dram_settings = $dram_settings . "\n\t\t($size1),\t\t//CFG_PHYS_SDRAM_1_SIZE" ;
        $dram_settings = $dram_settings . "\n\t\t($start1),\t\t//CFG_PHYS_SDRAM_1_START" ;
        $dram_settings = $dram_settings . "\n\t\t0x0,\t\t//CFG_PHYS_SDRAM_2_SIZE" ;
        $dram_settings = $dram_settings . "\n\t\t0x0,\t\t//CFG_PHYS_SDRAM_2_START" ;
        $dram_settings = $dram_settings . "\n\t\t0x0,\t\t//CFG_PHYS_SDRAM_3_SIZE" ;
        $dram_settings = $dram_settings . "\n\t\t0x0,\t\t//CFG_PHYS_SDRAM_3_START\n\t}" ;
        
        $iter = $iter + 1 ;
        
        if ($iter < $TotalCustemChips)
        {
            $dram_settings = $dram_settings . "," ;
        }
    }    

#    {
#        0x20BC,                                 //NAND_ID
#        2,                                      //DRAM_BANKS_NR
#        SZ_128M - RIL_SIZE,                     //CFG_PHYS_SDRAM_0_SIZE
#        RIL_SIZE,                               //CFG_PHYS_SDRAM_0_START
#        SZ_128M,                                //CFG_PHYS_SDRAM_1_SIZE
#        0x10000000,                             //CFG_PHYS_SDRAM_1_START
#        0,                                      //CFG_PHYS_SDRAM_2_SIZE
#        0,                                      //CFG_PHYS_SDRAM_2_START
#        0,                                      //CFG_PHYS_SDRAM_3_SIZE
#        0,                                      //CFG_PHYS_SDRAM_3_START
#    }
#        
    ###        
	my $template = <<"__TEMPLATE";
#ifndef __MT65XX_MEM_INFO_H__
#define __MT65XX_MEM_INFO_H__

struct DRAM_SETTINGS{
  unsigned short  NAND_ID;
  unsigned int BANKS_NR;
  unsigned int CFG_PHYS_SDRAM_0_SIZE;
  unsigned int CFG_PHYS_SDRAM_0_START;
  unsigned int CFG_PHYS_SDRAM_1_SIZE;
  unsigned int CFG_PHYS_SDRAM_1_START;
  unsigned int CFG_PHYS_SDRAM_2_SIZE;
  unsigned int CFG_PHYS_SDRAM_2_START;
  unsigned int CFG_PHYS_SDRAM_3_SIZE;
  unsigned int CFG_PHYS_SDRAM_3_START;
};

struct DRAM_SETTINGS dram_settings[]=
{
$dram_settings
};

#endif


__TEMPLATE
    return $template;  
}
	
#****************************************************************************
# subroutine:  DeviceListParser_LPSDRAM
# input:       the number in array
# return:      string contain 1 set of EMI setting for input 
#****************************************************************************
sub DeviceListParser_LPSDRAM
{
    my ($id) ;
    my ($PartNum) ;
    my ($NANDID) ;
    
    my ($is_part_found) ;     #  0: No part number is found
                              #  1: 1 part number is found
    $is_part_found = 0 ;
    $id = $_[0] ;
    
    $NANDID = $CustNAND_ID[$id] ;
    
    print "\nnum is $id, nandID is $CustNAND_ID[$id]\n" ;
    if (defined($CustCS0_PART_NUMBER[$id]))
    {
        $PartNum = $CustCS0_PART_NUMBER[$id] ;
    }
    elsif (defined($CustCS1_PART_NUMBER[$id]))
    {
        $PartNum = $CustCS1_PART_NUMBER[$id] ;
    }
    elsif (defined($CustCS2_PART_NUMBER[$id]))
    {
        $PartNum = $CustCS2_PART_NUMBER[$id] ;
    }
    elsif (defined($CustCS3_PART_NUMBER[$id]))
    {
        $PartNum = $CustCS3_PART_NUMBER[$id] ;
    }
    else
    {
        die "\nNo chip defined of $NANDID\n"
    }

    my $row        = $start_num + 0 ;                    # scan from row 2 when $MEMORY_DEVICE_TYPE eq 'LPSDRAM'
    my $col        = $COLUMN_PART_NUMBER ;               # scan column 2 for Part Number
    my $EXCEL_ROW ;
    
    $Sheet = get_sheet($MEMORY_DEVICE_TYPE) ;
    
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
        

        $eos_flag   = 7 ;

        # remove leading and tailing spaces
        $scan_idx =~ s/^\s+// if $DebugPrint == 1 ;
        $scan_idx =~ s/\s+$// if $DebugPrint == 1 ;
		
		$scan_idx =~ s/^\s+// ;
		$scan_idx =~ s/\s+$// ;
		
        if ($scan_idx eq $PartNum) # scan column 2 for Part Number

        {
            my $boardid ;
            $boardid = &xls_cell_value($Sheet, $row, $COLUMN_BOARD_ID) ;
            if ($CustBoard_ID eq $boardid)
            {
                $EXCEL_ROW = $row;
                print "\nPartNum($PartNum==$scan_idx) found in row $row\n" ;
                $is_part_found = 1 ;
                last ;
            }
        }
    }
    
    if ($is_part_found != 1)
    {
        die "\n[Error]unsupported part number $PartNum\n" ;
    }
    
    $_ = $row ;
    $VENDOR[$id] = &xls_cell_value($Sheet, $_, $COLUMN_VENDOR) ;
    $DENSITY[$id] = &xls_cell_value($Sheet, $_, $COLUMN_DENSITY) ;
    
    $NANDID = &xls_cell_value($Sheet, $_, $COLUMN_NAND_ID) ;
    print "\n$VENDOR[$id],$DENSITY[$id], NANDID is $NANDID\n" ;
    if ($NANDID eq undef || $NANDID eq "")
    {
        $NANDID = "0" ;
        if ($TotalCustemChips > 1)
        {
            die "\nundefined NAND ID $iter, and total chips > 1\n" ;
        }
    }
   
    
    $CustNAND_ID[$id] = $NANDID ;
#    print ("\n$VENDOR[$id],$DENSITY[$id] \n") ;

    # find the correct platform
    my $platform_scan_idx = $COLUMN_PLATFORM ; #First EMI controller
    my $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx) ;

    while (!($tmp_platform =~ $PLATFORM))
    {
        $platform_scan_idx++;
        $tmp_platform = &xls_cell_value($Sheet, $start_num, $platform_scan_idx);
    }
    &error_handler("$CUSTOM_MEMORY_DEVICE_HDR: $PLATFORM not support LPSDRAM!", __FILE__, __LINE__) if ($platform_scan_idx > $COLUMN_PLATFORM);

    $EMI_CONI_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_DRVA_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_DRVB_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_CONJ_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_CONK_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_CONL_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_IOCL_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_GENA_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_DRCT_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_PPCT_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_SLCT_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_ABCT_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_DUTB_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    $EMI_CONN_VAL[$id] = &xls_cell_value($Sheet, $_, $platform_scan_idx++) ;
    
    $EMI_SETTINGS[$id] = "\n\t//$PartNum\n\t{\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $NANDID . ",\t\t/* NAND ID */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONI_VAL[$id] . ",\t\t/* EMI_CONI_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_DRVA_VAL[$id] . ",\t\t/* EMI_DRVA_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_DRVB_VAL[$id] . ",\t\t/* EMI_DRVB_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONJ_VAL[$id] . ",\t\t/* EMI_CONJ_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONK_VAL[$id] . ",\t\t/* EMI_CONK_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONL_VAL[$id] . ",\t\t/* EMI_CONL_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_IOCL_VAL[$id] . ",\t\t/* EMI_IOCL_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_GENA_VAL[$id] . ",\t\t/* EMI_GENA_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMIGEND[$id] . ",\t\t/* EMI_GEND_VAL */\n\t\t" ;    
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_DRCT_VAL[$id] . ",\t\t/* EMI_DRCT_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_PPCT_VAL[$id] . ",\t\t/* EMI_PPCT_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_SLCT_VAL[$id] . ",\t\t/* EMI_SLCT_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_ABCT_VAL[$id] . ",\t\t/* EMI_ABCT_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_DUTB_VAL[$id] . ",\t\t/* EMI_DUTB_VAL */\n\t\t" ;
    $EMI_SETTINGS[$id] = $EMI_SETTINGS[$id] . $EMI_CONN_VAL[$id] . "\t\t/* EMI_CONN_VAL */\n\t}" ;
    
#    print  $EMI_SETTINGS[$id] ;
    print "\n\n" ;
    
    
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
    die "does not support windows now!!" ;
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
    my $filesize = 0x0 ;
    
    open FILE,">$INFO_TAG";
    print FILE pack("a27", "MTK_BLOADER_INFO_v07");
    $filesize = $filesize + 0x1b ;
    seek(FILE, 0x1b, 0);
    $pre_bin = "preloader_${project}.bin";
    print "PROJECT = $project, pre_bin = $pre_bin\n";
    print FILE pack("a61", $pre_bin); 
    $filesize = $filesize + 61 ;
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
        print FILE pack("L", hex($CustNAND_ID[$iter]));     #nand id
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_CONI_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_DRVA_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_DRVB_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_CONJ_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_CONK_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_CONL_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_IOCL_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_GENA_VAL[$iter])) ;
        $filesize = $filesize + 4 ;

        print FILE pack ("L", hex ($EMIGEND[$iter])) ;
        $filesize = $filesize + 4 ;

        print FILE pack ("L", hex ($EMI_DRCT_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_PPCT_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_SLCT_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_ABCT_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        print FILE pack ("L", hex ($EMI_DUTB_VAL[$iter])) ;
        $filesize = $filesize + 4 ;

        print FILE pack ("L", hex ($EMI_CONN_VAL[$iter])) ;
        $filesize = $filesize + 4 ;
        
#        print "file size is $filesize \n";

        $iter = $iter + 1 ;
    }
#    $filesize = $filesize + 4 ;
    
#    print "file size is $filesize \n";

    print FILE pack("L", $filesize) ;
    
    close (FILE) ;
    print "$INFO_TAG is generated!\n";
    return ;
}
