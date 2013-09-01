#!/usr/local/bin/perl
#
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
#*   This script will ...
#*        
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
use File::Basename;

my $os = &OsName();

my $LOCAL_PATH;

BEGIN
{
  $LOCAL_PATH = dirname($0);
}

if ($os eq "linux")
{
   print "Os = linux\n";

   use lib "$LOCAL_PATH/../../Spreadsheet";
   use lib "$LOCAL_PATH/../../";
   require 'ParseExcel.pm';
   $start_num = 0; 
}
else
{
  die "Only linux is support now!\n";
}

my $DebugPrint    = 1; # 1 for debug; 0 for non-debug
#****************************************************************************
# Now for nand device list gen
#****************************************************************************

my $VENDOR_FIELD ;
my $NAME_FIELD ;
my $PROJECT_FIELD ;
my $ID_FIELD ;
my $EXTID_FIELD ;
my $ID_FIELD1 ;
my $ID_FIELD2 ;
my $ID_FIELD3 ;
my $ID_FIELD4 ;
my $ID_FIELD5 ;
my $ADDRCYCLE_FIELD ;
my $IOWIDTH_FIELD ;
my $TOTALSIZE_FIELD ;
my $BLOCKSIZE_FIELD ;
my $PAGESIZE_FIELD ;
my $TIMING_FIELD ;
my $CACHEREAD_FIELD ;
my $RANDOMREAD_FIELD ;

# define for columns
my $COLUMN_VENDOR                   = 0 ;
my $COLUMN_NAME                     = $COLUMN_VENDOR + 1 ;
my $COLUMN_PROJECT                  = $COLUMN_NAME + 1 ;
my $COLUMN_ID1                      = $COLUMN_PROJECT + 1 ;
my $COLUMN_ID2                      = $COLUMN_ID1 + 1 ;
my $COLUMN_ID3                      = $COLUMN_ID2 + 1 ;
my $COLUMN_ID4                      = $COLUMN_ID3 + 1 ;
my $COLUMN_ID5                      = $COLUMN_ID4 + 1 ;
my $COLUMN_ADDRCYCLE                = $COLUMN_ID5 + 1 ;
my $COLUMN_IOWIDTH                  = $COLUMN_ADDRCYCLE + 1 ;
my $COLUMN_TOTALSIZE                = $COLUMN_IOWIDTH + 1 ;
my $COLUMN_BLOCKSIZE                = $COLUMN_TOTALSIZE + 1 ;
my $COLUMN_PAGESIZE                 = $COLUMN_BLOCKSIZE + 1 ;
my $COLUMN_TIMING                   = $COLUMN_PAGESIZE + 1 ;
my $COLUMN_CACHEREAD                = $COLUMN_TIMING + 1 ;
my $COLUMN_RANDOMREAD               = $COLUMN_CACHEREAD + 1 ;

my $NAND_LIST_DEFINE_H_NAME         = $ARGV[0] ;
my $MEMORY_DEVICE_LIST_XLS          = $ARGV[1];
my $PLATFORM                        = $ARGV[2]; # MTxxxx
my $PROJECT                         = $ARGV[3];
my $PAGE_SIZE                       = $ARGV[4] ;

my $STORAGE_TYPE                    = "NAND" ;
my $start_row = 3;

print "header: $NAND_LIST_DEFINE_H_NAME, excel: $MEMORY_DEVICE_LIST_XLS, PLATFORM: $PLATFORM, PROJECT: $PROJECT, page size: $PAGE_SIZE\n" ;

$parser = Spreadsheet::ParseExcel->new() ;
$Book = $parser->Parse($MEMORY_DEVICE_LIST_XLS) ; 

&ReadNANDExcelFile () ;

&GenNANDHeaderFile () ;

print "nandgen done\n" ;
exit ;

sub GenNANDHeaderFile ()
{
    my $iter = 0 ;
    my $i ;
    my $temp ;
    my $advance_option ;
    my $nand_count = 0;
    open (NAND_LIST_DEFINE_H_NAME, ">$NAND_LIST_DEFINE_H_NAME") or &error_handler("$NAND_LIST_DEFINE_H_NAME: file error!", __FILE__, __LINE__);
    print NAND_LIST_DEFINE_H_NAME &copyright_file_header();
    
    print NAND_LIST_DEFINE_H_NAME "\n#ifndef __NAND_DEVICE_LIST_H__\n#define __NAND_DEVICE_LIST_H__\n\n" ;
        
	print NAND_LIST_DEFINE_H_NAME "static const flashdev_info gen_FlashTable[]={\n" ;
   
    $PROJECT = uc($PROJECT);
	
    for ($iter=0; $iter<$total_rows; $iter++)
    {
        $advance_option = 0;
        # if ($PROJECT_FIELD[$iter] eq $PROJECT)
        {
            print "$PAGE_SIZE $PAGESIZE_FIELD[$iter]\n" ;
            if (($PAGE_SIZE eq "4K" && $PAGESIZE_FIELD[$iter] eq 4096) || ($PAGE_SIZE eq "2K" && $PAGESIZE_FIELD[$iter] eq 2048))
            {
                for ($i=0; $i<$iter; $i++)
                {
                    if ( ($ID_FIELD[$iter] eq $ID_FIELD[$i]) && ($EXTID_FIELD[$iter] eq $EXTID_FIELD[$i]) )
                    {
                        print "Device $ID_FIELD[$iter] $EXTID_FIELD[$iter] already exists\n" ;
                        last;
                    }
                }

                print "i=$i iter=$iter\n" ;

                if ($i eq $iter)
                {
                    print NAND_LIST_DEFINE_H_NAME "\t{$ID_FIELD[$iter], $EXTID_FIELD[$iter], $ADDRCYCLE_FIELD[$iter], $IOWIDTH_FIELD[$iter], $TOTALSIZE_FIELD[$iter], $BLOCKSIZE_FIELD[$iter], $PAGESIZE_FIELD[$iter], $TIMING_FIELD[$iter], " ; 
                    print "\t{$ID_FIELD[$iter], $EXTID_FIELD[$iter], $ADDRCYCLE_FIELD[$iter], $IOWIDTH_FIELD[$iter], $TOTALSIZE_FIELD[$iter], $BLOCKSIZE_FIELD[$iter], $PAGESIZE_FIELD[$iter], $TIMING_FIELD[$iter], " ; 
                    printf NAND_LIST_DEFINE_H_NAME "\"%.13s\", ",$NAME_FIELD[$iter] ;
                    if ($CACHEREAD_FIELD[$iter] eq "YES")
                    {
                        $advance_option += 2 ;
                    }
                    if ($RANDOMREAD_FIELD[$iter] eq "YES")
                    {
                        $advance_option += 1 ;
                    }
                    print NAND_LIST_DEFINE_H_NAME "$advance_option},\n" ;
    
                    $nand_count += 1;
                }
            }
            else
            {
                print "page size not match\n" ;
            }
        }
    }

    if ($nand_count eq 0)
    {
        print "Platform: $PLATFORM, project: $PROJECT, page size: $PAGE_SIZE\n" ;
        my $message = "ERROR: no nand device is generated into device list, please refer: $NAND_LIST_XLS and error log\n";
        # print $message ;
        # die $message ;
    }
    
    print NAND_LIST_DEFINE_H_NAME "\t{0x0000, 0x000000, 0, 0, 0, 0, 0, 0, \"xxxxxxxxxx\", 0},\n" ;
    print NAND_LIST_DEFINE_H_NAME "};\n" ;
    
    print NAND_LIST_DEFINE_H_NAME "\n\n" ;
    print NAND_LIST_DEFINE_H_NAME "#endif\n" ;
    
    close NAND_LIST_DEFINE_H_NAME ;
}

sub ReadNANDExcelFile
{
    my $sheet;
    my $read       = 1; # if this flag counts to '0', it means End Of Sheet
    
    my $row = 1;
    my $read_row = $start_row;

    $sheet = get_sheet($STORAGE_TYPE) ;
    
    if ($sheet eq undef)
    {
        print "get_sheet failed? $STORAGE_TYPE\n" ;
    }

    print "begin read\n" ;
    
    while ($read)
    {
        $VENDOR_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_VENDOR) ;
        if ($VENDOR_FIELD[$row-1] eq $undefined)
        {
            print "meet END.\n";
            $read = 0 ;
        }
       
        if ($read)
        {  
            $NAME_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_NAME) ;
            $PROJECT_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_PROJECT) ;
            $PROJECT_FIELD[$row-1] = uc($PROJECT_FIELD[$row-1]) ;
            $ID_FIELD1[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ID1) ;
            $ID_FIELD2[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ID2) ;
            $ID_FIELD3[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ID3) ;
            $ID_FIELD4[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ID4) ;
            $ID_FIELD5[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ID5) ;
            $ID_FIELD[$row-1] = sprintf("0x%2s%2s", substr($ID_FIELD1[$row-1], 2, 2), substr($ID_FIELD2[$row-1], 2, 2));
            $EXTID_FIELD[$row-1] = sprintf("0x%2s%2s%2s", substr($ID_FIELD3[$row-1], 2, 2), 
                substr($ID_FIELD4[$row-1], 2, 2), substr($ID_FIELD5[$row-1], 2, 2));
            $ADDRCYCLE_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_ADDRCYCLE) ;
            $IOWIDTH_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_IOWIDTH) ;
            $TOTALSIZE_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_TOTALSIZE) ;
            $BLOCKSIZE_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_BLOCKSIZE) ;
            $PAGESIZE_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_PAGESIZE) ;
            $TIMING_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_TIMING) ;
            $CACHEREAD_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_CACHEREAD) ;
            $RANDOMREAD_FIELD[$row-1] = &xls_cell_value($sheet, $read_row, $COLUMN_RANDOMREAD) ;
	        # debug
            print "$NAME_FIELD[$row-1], $PROJECT_FIELD[$row-1] $ID_FIELD1[$row-1] $ID_FIELD2[$row-1] $ID_FIELD[$row-1]\n" ;
            # debug
            $row ++ ;
            $read_row++;
        }
    }
    
    if ($row eq $start_row)
    {
        die "error in excel file no data!\n" ;
    }
    
    $total_rows = $row - 1;
    
    print "$total_rows read.\n" ;
}

#****************************************************************************
# subroutine:  copyright_file_header
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header
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

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler
{
	   my ($error_msg, $file, $line_no) = @_;
	   
	   my $final_error_msg = "scatgen ERROR: $error_msg at $file line $line_no\n";
	   print $final_error_msg;
	   die $final_error_msg;
}
