#!/usr/bin/perl
# Programmer: Shengkai Lin (2009-08-05)

use Cwd;

#We have to know how to get $(MTK_PATH_SOURCE) from build system
$MTK_PATH_SOURCE = "mediatek/source";
$IS_AUTOGEN = 1;
$CURRENT_PATH = getcwd();
$BT_CODEGEN_PATH = "$MTK_PATH_SOURCE/external/bluetooth/blueangel/_bt_scripts";
$CGEN_PATH = "$CURRENT_PATH/$MTK_PATH_SOURCE/cgen/Cgen";
$CGEN_PC_CNF_PATH = "$CURRENT_PATH/$MTK_PATH_SOURCE/cgen/cgencfg/pc_cnf";
$CGEN_TGT_CNF_PATH = "$CURRENT_PATH/$MTK_PATH_SOURCE/cgen/cgencfg/tgt_cnf";
$CGEN_COMPILER = "gcc";
#$GLOBAL_OPTIONS = genGlobalOptions();
$GLOBAL_OPTIONS;
$rel_db_path;

if ($#ARGV != 4)
{
	usage();
}
else
{
	my $project;
	
	$BT_CODEGEN_PATH = $ARGV[0];
	$CGEN_PATH = "$CURRENT_PATH/$ARGV[1]";
	$CGEN_PC_CNF_PATH = "$CURRENT_PATH/$ARGV[2]";
	$CGEN_TGT_CNF_PATH = "$CURRENT_PATH/$ARGV[3]";
	$project = $ARGV[4];
	
	$BT_CODEGEN_PATH =~ s/[\/,\\]$//;
	#if this is customer codebase, there will no BT folder. Just exit.
	exit(1) if (!-d $BT_CODEGEN_PATH);
	die "$CGEN_PATH does NOT exist!\n" if (!-e $CGEN_PATH);
	die "$CGEN_PC_CNF_PATH does NOT exist!\n" if (!-e $CGEN_PC_CNF_PATH);
	die "$CGEN_TGT_CNF_PATH does NOT exist!\n" if (!-e $CGEN_TGT_CNF_PATH);
}

print "GLOBAL_OPTIONS = $GLOBAL_OPTIONS\n";
print "include $BT_CODEGEN_PATH/BTCodegen.pl\n";
chdir($BT_CODEGEN_PATH);
require ("BTCodegen.pl");
print "Return from require\n";
chdir($CURRENT_PATH);

sub
genGlobalOptions
{
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
	       $flags .= "-D$ENV{$def} " if ($ENV{$def} ne "" and $ENV{$def} ne "no");
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
	return $flags;
}

sub
usage
{
  print "Usage: BTCodegen.pl bt_path cgen_path pc_cnf_path tgt_cnf_path project\n";
  print "       bt_path == BT codegen working path.\n";
  print "       cgen_path == path of Cgen.\n";
  print "       pc_cnf_path == BT codegen working path.\n";
  print "       tgt_cnf_path == BT codegen working path.\n";
  print "       project == Project name.\n";
  print "       E.g: BTCodegen.pl mediatek/source/external/bluetooth/blueangel/_bt_scripts mediatek/source/cgen/Cgen mediatek/source/cgen/cgencfg/pc_cnf mediatek/source/cgen/cgencfg/tgt_cnf zte73v1_2\n";
	exit(1);
}
