#!/usr/bin/perl
my $DEBUG = 0; # global debug switch

&Usage if ($#ARGV < 0);
#my $workingDir = $ARGV[0];
my $workingDir = "mediatek/cgen/";
die "Can NOT find $workingDir\n" if (!-d $workingDir);
chdir($workingDir);

$workingDir = "./";
#die "Log directory $workingDir does NOT exist!\n" if (!-d $workingDir);

my $RelativePath = "./"; #"mediatek/cgen/";
my $CustomPath = "../custom/"; #"mediatek/custom/";
my $CommonPath = "../custom/common/cgen/"; #"mediatek/custom/common/cgen/";

my $PlatformVersion = $ENV{"MTK_PLATFORM"};
my $ChipSwVersion = $ENV{"MTK_CHIP_VER"};
my $ProjectVersion = $ENV{"MTK_PROJECT"};
my $SWMajorVersion = $ENV{"MTK_BRANCH"};
my $SWMinorVersion = $ENV{"MTK_WEEK_NO"};

my $logDir = "out/target/product/${ProjectVersion}/cgen/";

my $dir_include = $CommonPath . "cfgfileinc";
my $file_name_include = $CommonPath . "inc/cfg_module_file.h";
my $dir_APEditor = $RelativePath . "apeditor/";
my $AP_Temp_CL = $dir_APEditor . "app_temp_db";

my $dir_default_include = $CommonPath . "cfgdefault";
my $file_DefaultN_include = $CommonPath . "inc/cfg_module_default.h";

my $Output_Dir = "../cfgfileinc/";
my $Output_Default_Dir = "../cfgdefault/";
my $Custom_Output_Dir = "../cfgfileinc/";
my $Custom_Output_Defualt_Dir = "../cfgdefault/";

#my $Custom_dir_include = $workingDir . "custom/cfgfileinc";
#my $Custom_file_name_include = $workingDir . "custom/inc/custom_cfg_module_file.h";
#
#my $Custom_dir_default_include = $workingDir . "custom/cfgdefault";
#my $Custom_file_DefaultN_include = $workingDir . "custom/inc/custom_cfg_module_default.h";
my $Custom_dir_include = $CustomPath . $ProjectVersion ."/cgen/cfgfileinc";
my $Custom_file_name_include = $CustomPath . $ProjectVersion . "/cgen/inc/custom_cfg_module_file.h";

my $Custom_dir_default_include = $CustomPath . $ProjectVersion . "/cgen/cfgdefault";
my $Custom_file_DefaultN_include = $CustomPath . $ProjectVersion . "/cgen/inc/custom_cfg_module_default.h";

my $AP_Parse_File = $dir_APEditor . "app_parse_db.c";

my $Err_file = "${workingDir}Cgen_db.log";
my $Log_file = "${workingDir}Cgen_db.err";

my $dir_Codegen = $workingDir;

my $Tgt_Cnf = $dir_Codegen . "cgencfg/tgt_cnf";
my $Pc_Cnf = $dir_Codegen . "cgencfg/pc_cnf";

my $dir_APDatabase = $workingDir;
my $AP_Editor_DB = $dir_APDatabase . "APDB_$PlatformVersion"."_"."$ChipSwVersion"."_"."$SWMajorVersion"."_"."$SWMinorVersion";
my $AP_Editor2_Temp_DB = $dir_APDatabase . "APDB2_$PlatformVersion"."_"."$ChipSwVersion"."_"."$SWMajorVersion"."_"."$SWMinorVersion";
my $AP_Editor_DB_Enum_File = $dir_APDatabase . "APDB_$PlatformVersion"."_"."$ChipSwVersion"."_"."$SWMajorVersion"."_"."$SWMinorVersion"."_"."ENUM";

my $Cgen = $dir_Codegen . "Cgen";

# auto include the Custom_NvRam_data_item.h into ap_parse_db.c
my $APDB_SourceFile = $workingDir . "apeditor/app_parse_db.c";
AutoIncludeDatabase( $APDB_SourceFile);

# delete the err & log at first.
if (-f $Err_file) {   
    system( "rm -f " . $Err_file );    
}

if (-f $Log_file) {
    system( "rm -f " . $Log_file);
}

open ERR_FILE, ">$Err_file" or die "Can't create the file $Err_file";

# make sure *.h does exist!
if (!-d $dir_include) {
    print ERR_FILE "$dir_include directory does not exist!\n"; 
    goto Err;
}

# Step2. call cl.exe
if (!-d $dir_APEditor) {
    print ERR_FILE "$dir_APEditor does not exist!\n";
    goto Err;
}

# Step1. Auto generated the relative .h
&AutoGenHeaderFile( $dir_include, $file_name_include, $Output_Dir);
&AutoGenHeaderFile( $dir_default_include, $file_DefaultN_include, $Output_Default_Dir);
# Step2. Auto generated the relative .h
#&AutoGenHeaderFile( $Project_dir_include, $Project_file_name_include,$Project_Output_Dir);

&AutoGenHeaderFile( $Custom_dir_include, $Custom_file_name_include, $Custom_Output_Dir);
&AutoGenHeaderFile( $Custom_dir_default_include, $Custom_file_DefaultN_include, $Custom_Output_Defualt_Dir);

# customized folder will be used first, if header NOT exist in customized folder, common folder will be used
my $flags = "";
foreach ($ENV{AUTO_ADD_GLOBAL_DEFINE_BY_NAME})
{
   my @defArray = split(/\s+/);
   foreach my $def (@defArray)
   {
       $flags .= "-D$def " if ($ENV{$def} ne "" and $ENV{$def} ne "no");
   }
}
foreach ($ENV{AUTO_ADD_GLOBAL_DEFINE_BY_VALUE})
{
   my @defArray = split(/\s+/);
   foreach my $def (@defArray)
   {
       my @valueArray = split(/\s+/,$ENV{$def});
       foreach my $value (@valueArray)
       {   
           $flags .= "-D$value " if ($value ne "" and $value ne "no");
       }
   }
}
foreach ($ENV{AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE})
{
   my @defArray = split(/\s+/);
   foreach my $def (@defArray)
   {
       $flags .= "-D$def=\"$ENV{$def}\" " if ($ENV{$def} ne "" and $ENV{$def} ne "no");
   }
}
my $flagLog = "${workingDir}/flagsInfo";
if (-f $flagLog) {
    system( "rm -f " . $flagLog);
}

system("echo MTK_CDEFS = $flags > $flagLog");

system("gcc -E $flags -I${CustomPath}${ProjectVersion}/cgen/inc -I../custom/common/cgen/inc " . $AP_Parse_File . ">$AP_Temp_CL");

# Step3. call Cgen
# Delete the old db files at fist
# Then codegen generate the new database
if (-f $AP_Editor_DB) {
    system("rm -f " . $AP_Editor_DB);
}
if (-f $AP_Editor2_Temp_DB) {
    system("rm -f " . $AP_Editor2_Temp_DB);
}
if (-f $AP_Editor_DB_Enum_File) {
    system("rm -f " . $AP_Editor_DB_Enum_File);
}

#generate the base database
print("$Cgen -c $AP_Temp_CL $Tgt_Cnf $Pc_Cnf $AP_Editor2_Temp_DB $AP_Editor_DB_Enum_File >$Log_file\n");
my $cgenRslt1 = system("$Cgen -c $AP_Temp_CL $Tgt_Cnf $Pc_Cnf $AP_Editor2_Temp_DB $AP_Editor_DB_Enum_File >$Log_file");
print "Generating base database failed!\n" if ($cgenRslt1);
#add the software version into database
print("$Cgen -cm $AP_Editor_DB $AP_Editor2_Temp_DB $AP_Temp_CL $AP_Editor_DB_Enum_File >$Log_file\n");
my $cgenRslt2 = system("$Cgen -cm $AP_Editor_DB $AP_Editor2_Temp_DB $AP_Temp_CL $AP_Editor_DB_Enum_File >$Log_file");
print "Adding SW version failed!\n" if ($cgenRslt2);
if (!$cgenRslt1 && !$cgenRslt2)
{
    print "codegen process is DONE successfully!\n";
}
else
{
   print "codegen failed!\n";
    goto Err;
}
#delete the temp file
system("rm -f ".$AP_Editor2_Temp_DB);
system("rm -f ".$AP_Temp_CL);

close ERR_FILE;

if (-f $Err_file) 
{
    system( "rm -f " . $Err_file);
}

exit 0;

Err:
exit -1;

sub AutoGenHeaderFile
# $dir_path, the searched directory
# $fileName, the generated header file name
{
    my ($dirPath, $fileName, $out_dir) = @_;
    my $num_include_file = 0;
    
    
	# delete the .h at first.
	if (-f $fileName) {   
    	system( "rm -f " . $fileName );
	}
 
    open OUTPUT_FILE, ">$fileName" or die "Can't open the file $fileName";
    
    opendir DH, $dirPath or die "Can't open $dirPath: $!";
	
    foreach $file (readdir DH) 
	{
        if ( $file =~ /(\.h)$/) 
		{
            $num_include_file++;
            # format: relative path is needed, because can't get the paths in sources.cmn
            #         for just "cl", not "build -c"
            # #include "../cfgfileinc/xxx.h"
            print OUTPUT_FILE "#include " . "\"$out_dir$file\"" . "\n";
        }
    }
    closedir DH;
    close OUTPUT_FILE;
    
    # make sure the include diretory should has *.h in it.
    if ( $num_include_file == 0 ) 
	{
        print ERR_FILE "The $dir_include has no *.h in it!\n";
        if (-f $fileName) 
		{
            system("rm -f " . $fileName);
        }
        exit 0; 
    }   
}

sub AutoIncludeDatabase
# $dir_path, the searched directory
# $fileName, the generated header file name
{
    my ($fileName) = @_;    
    
# delete the .h at first.
	if (-f $fileName) {   
    	system( "rm -f " . $fileName );
	}
 
    open OUTPUT_FILE, ">$fileName" or die "Can't open the file $fileName";
    
    print OUTPUT_FILE "#include "."\"tst_assert_header_file.h\"". "\n" ;
    
    print OUTPUT_FILE "#include "."\"ap_editor_data_item.h\"". "\n" ;

    print OUTPUT_FILE "#include "."\"Custom_NvRam_data_item.h\"" . "\n" ;

    close OUTPUT_FILE;
       
}

sub Usage {
  warn << "__END_OF_USAGE";
Usage: $myCmd LOG_PATH
__END_OF_USAGE
  exit 1;
}

