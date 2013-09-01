#!/usr/bin/perl
use strict;

# one of "ldpi,mdpi,hdpi,xdpi";
my $resource_select = $ARGV[0];

# the path of package;
my $path            = $ARGV[1];

# the platform sdk version;
my $platform_sdk_version = $ARGV[2];

# the path of pre-assign resource path;
my $res_path            = $ARGV[3];

# project name
my $proj = $ARGV[4];


# all log of optimization will be dumper to this file;
die "can not create log file:$!" if (! open OUTPUT, ">>out/target/product/${proj}_AppResource_Optimization.log");

my $temp_dir = "out/target/product/${proj}_AppResource";

# Ignore resource optimization if resource directory NOT exist
if (defined $res_path)
{
  if (!-d $res_path)
  {
    print "$res_path";
    print OUTPUT "!!!Skip $path: resource_directory NOT exist!\n";
    exit 0;
  }
}
else
{
    print OUTPUT "!!!Skip $path: NO resource_directory specified!\n";
    exit 0;
}
# skip package located under "out folder"
#if ($res_path =~ /^out/)
#{
#    print "res path in out folder\n";
#    print OUTPUT "current resource path in output folder \n";
#    print "$res_path";
#    exit   
#}

# parser Android.mk to decide if the package can be optimizated;
die "can not open $path/Android.mk:$!" if ( ! open ANDROID_MAKE, "$path/Android.mk");
while(<ANDROID_MAKE>)
{
    if (/LOCAL_AAPT_FLAGS/)
    {
        print OUTPUT "!!!Skip $path: with LOCAL_AAPT_FLAGS set in Android.mk\n";
        print "$res_path";
        exit;        
    }   
}
close ANDROID_MAKE;

# parser AndroidManifest.xml to decide if the package can be optimizated;
die "can not open $path/AndroidManifest.xml:$!" if ( ! open XML, "$path/AndroidManifest.xml");

# if the flag == 0,skip the package; else optimizate it
my $flag = 0;
my $targetSdk_assign = 0;
while (<XML>)
{
    if (/android:anyDensity\s*=\s*\"(.*)\"/)
    {
        my $value = $1;
        if ($value eq "true")
        {
            $flag = 1; # need optimizate
            print OUTPUT "!!!Optimization $path: android:anyDensity=true\n";
        }
        if ($value eq "false")
        {
            print OUTPUT "!!!Skip $path: Use \"android:anyDensity=false\" in AndroidManifest.xml\n";
            print "$res_path";
            exit;
        }
        last;
    }
    elsif (/android:minSdkVersion\s*=\s*\"(.*)\"/)
    {
        if ($1 >= 4)
        {
            $flag = 1;
            print OUTPUT "!!!Optimization $path: android:minSdkVersion >= 4\n";
            last;
        }
    }
    elsif (/android:targetSdkVersion\s*=\s*\"(.*)\"/)
    {
        if ($1 >= 4)
        {
            $flag = 1;
            print OUTPUT "!!!Optimization $path: android:targetSdkVersion >= 4\n";
            last;
        }
        $targetSdk_assign = 1;        
    }
}
close XML;

# this section for target Sdk version not define but platform sdk version define case.
if (($flag != 1) && ($targetSdk_assign != 1) && (defined $platform_sdk_version))
{
    print OUTPUT "platform_sdk_version : $platform_sdk_version\n";
    if ($platform_sdk_version >= 4)
    {
        $flag = 1;
        print OUTPUT "!!!Optimization $path: android:platform_sdk_version >= 4\n";
    }
}

if ($flag == 1)
{
    my $merge_order = "ldpi,mdpi,hdpi,xdpi,"; # all resource can be used

    # according $resource_select to rearrange $merge_order
    $merge_order =~ s/(.*)$resource_select,(.*)/$1$2/;
    $merge_order .= "$resource_select";
    my @merge = split(/,/,$merge_order);
    
    # all resource after merging will be placed under these dirs
    my @dirs = qw/drawable drawable-land/;
    &check_with_drawable_folder("$res_path");
    &p_system("rm -rf $temp_dir/$path") if (-d "$temp_dir/$path");
    &p_system("mkdir -p $temp_dir/$path/");
    &p_system("cp -a $res_path $temp_dir/$path");
    &check_duplicate_sub("$temp_dir/$res_path", $resource_select);
    print "$temp_dir/$res_path";
}
else
{
    print OUTPUT "!!!Skip $path: Without set \"android:anyDensity=true\" or \"android:minSdkVersion >= 4\" or \"android:targetSdkVersion >= 4\"\n";
    print "$res_path";
    exit;
}

#################################################################
# subroutine: check_with_drawable_folder
# input:      the path of files needed to check
# function:   check if exist drawable folder
#################################################################

sub check_with_drawable_folder
{
    my $check_path = $_[0];
    my %file_hash;
    my $with_drawable_folder = 0;
    die "can not open $check_path" if (! opendir DIR, "$check_path");
    foreach my $f (readdir DIR)
    {
        next if ($f =~ /^\.$/);
        next if ($f =~ /^\.\.$/);
        if ($f =~ /^drawable/)
        {
            $with_drawable_folder = 1;
        }
    }

    if ($with_drawable_folder == 0)
    {
        #without any drawable folder, keep original process flow
        print OUTPUT "!!!Skip $path: Can't find any drawable related folder, Don't need to optimize!\n";
        print "$res_path";
        exit;
    }
}


#################################################################
# subroutine: check_duplicate_sub
# input:      the path of files needed to check
# function:   check if exist have same file name but different extension name fi              les, (if yes will skip this package,else optimizate it
#################################################################
sub check_duplicate_sub
{
    my $check_path = $_[0];
    my $resource_select = $_[1];
    my %file_hash;
    
    my $merge_order = "ldpi,mdpi,hdpi,xdpi,"; # all resource can be used

    # according $resource_select to rearrange $merge_order
    my @merge = split(/,/,$merge_order);
    
    # all resource after merging will be placed under these dirs
    my @dirs = qw/drawable drawable-land/;
    
    foreach my $dir (@dirs)
    {
        # initial file hash table
        %file_hash = ();
                
        # establish file hash table from drawable-$resource_select / drawable-land-$resource_select
        if (-d "$check_path/$dir-$resource_select")
    {
            die "can not open $check_path/$dir-$resource_select:$!" if (! opendir DIR, "$check_path/$dir-$resource_select");
            #establish hash table
        foreach my $f (readdir DIR)
        {
            next if ($f =~ /^\.$/);
            next if ($f =~ /^\.\.$/);
                if (lc($f) =~ /png|jpg|bmp|gif/)
                {
            my $basename = $f;
            if (!$file_hash{$basename})
            {
                        $file_hash{$basename} = "$dir-$resource_select";
                    }
                }
            }
            closedir(DIR);              
       }
#      else
#      {
#           print OUTPUT "1: $dir-$resource_select not exist \n";
#           next;
#       }           
        
        # check drawable or drawable-land folder
        # check if already exist in hash table, if yes, delete duplicate file
        &remove_duplicate(\%file_hash, $check_path, $resource_select, "$dir");
        
        # list each folder and check the drawable image exist in file hash table, if already exist, we can remove the duplicate file
        foreach my $select (@merge)
        {
            if ($select eq $resource_select)
            {
                next;
            }else
            {
                # check if already exist in hash table, if yes, delete duplicate file
                &remove_duplicate(\%file_hash, $check_path, $resource_select, "$dir-$select");
            }
        }
    }   
}


#################################################################
# subroutine: remove_duplicate
# input:      the path of files needed to check
# function:   check if exist have same file name 
#################################################################
sub remove_duplicate(%, $, $, $)
{
    my(%hash) = %{$_[0]};
    my $check_path = $_[1];
    my $resource_select = $_[2];
    my $res_folder = $_[3]; 
    
    # check drawable or drawable-land folder
    # check if already exist in hash table, if yes, delete duplicate file
    print OUTPUT "check folder $check_path/$res_folder \n";
    if (-d "$check_path/$res_folder")
    {
        die "can not open $check_path/$res_folder:$!" if (! opendir DIR, "$check_path/$res_folder");
        foreach my $f (readdir DIR)
            {
            next if ($f =~ /^\.$/);
            next if ($f =~ /^\.\.$/);                       
            if (lc($f) =~ /png|jpg|bmp|gif/)
            {
                my $basename = $f;                  
                if (exists $hash{$basename})
                {
                    print OUTPUT "file $basename already in $hash{$basename}, remove it \n";
                    &p_system("rm -rf $check_path/$res_folder/$basename");
                }
            }
        }
        closedir(DIR);
    }
#   else
#   {
#      print OUTPUT "$check_path/$res_folder not exist!! \n";
#   }
    
}

sub p_system
{
    my $rslt = system("$_[0]");
    print OUTPUT "$_[0]\n" if ($rslt == 0);
}
