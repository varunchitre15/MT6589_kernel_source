#!usr/bin/perl
my $prj = $ARGV[0];
my $cfg = "mediatek/custom/$prj/security/image_encrypt/IMG_ENCRYPT_CFG.ini";
my $key = "mediatek/custom/$prj/security/image_encrypt/IMG_ENCRYPT_KEY.ini";
my $BUILD_UBOOT = $ENV{"BUILD_UBOOT"};
#my $ENC_SUPPORT = "yes";
my $ENC_SUPPORT = "no";

##########################################################
# Dump Parameter
##########################################################
print "\n\n";
print "********************************************\n";
print " Dump Paramter \n";
print "********************************************\n";
print " BUILD_UBOOT      : $BUILD_UBOOT\n";
print " ENC_SUPPORT      : $ENC_SUPPORT\n";

################################################################
# ENCRYPT PLAINTEXT IMAGE (GENERAL FILES)
################################################################
#print "\n********************************************\n";
#print " PLAINTEXT IMAGE \n";
#print "********************************************\n\n\n";

#my $dir = "out/target/product/$prj";

#`mkdir $dir/enc_bin` if ( ! -d "$dir/enc_bin" );
#print "Image Dir '$dir'\n";
#my $command = "mediatek/build/tools/CipherTool/CipherTool";

#my @imgs_need_enc = ("boot.img", "logo.bin", "recovery.img", "secro.img", "system.img", "userdata.img", "lk.bin");
#if(${BUILD_UBOOT} eq "yes") {
#        push (@imgs_need_enc, "uboot_${prj}.bin");
#}

#foreach my $img (@imgs_need_enc) {
#	push (@miss_img, $img) if ( ! -e "$dir/$img");
#}
#die "@miss_img\nall the imgs above is NOT exsit\n" if (@miss_img > 0);

#foreach my $img (@imgs_need_enc) {
#	if ( ! -e "$dir/$img") {
#		warn "the $img is NOT exsit, please check\n";
#		next;
#	}
#	my $enc_img = $img;
#	$enc_img =~ s/\./-enc\./;
#	print "Encrypt Image '$dir/$img' ...\n";
#	&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
#	&print_system("$command ENC_RESET $key $cfg $dir/$img $dir/enc_bin/$enc_img");
#}

##########################################################
# ENCRYPT PLAINTEXT IMAGE (EMMC FILES)
##########################################################
#print "\n\n";
#print "********************************************\n";
#print " ENCRYPT EMMC IMAGE \n";
#print "********************************************\n";

#my @imgs_need_enc = ("MBR", "EBR1", "EBR2");
#foreach my $img (@imgs_need_enc) {
#	if (-e "$dir/$img") {	
#		my $enc_img = $img."-enc";
#		print "Encrypt Image '$dir/$img' ...\n";
#		&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
#	}
#}

#my @imgs_need_enc = ("cache.img");
#foreach my $img (@imgs_need_enc) {
#	if (-e "$dir/$img") {		
#		my $enc_img = $img;
#		$enc_img =~ s/\./-enc\./;
#		print "Encrypt Image '$dir/$img' ...\n";
#		&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
#	}
#}

################################################################
# ENCRYPT SIGNED IMAGE IF NEEDED (GENRAL FILES)
################################################################
print "\n\n\n********************************************\n";
print " SIGNED IMAGE\n";
print "********************************************\n\n\n";
my $dir = "out/target/product/$prj/signed_bin";

`mkdir $dir/enc_bin` if ( ! -d "$dir/enc_bin" );
print "Image Dir '$dir'\n";
my $command = "mediatek/build/tools/CipherTool/CipherTool";

my @imgs_need_enc = ("boot-sign.img", "logo-sign.bin", "recovery-sign.img", "secro-sign.img", "system-sign.img", "userdata-sign.img", "lk-sign.bin");
if(${BUILD_UBOOT} eq "yes") {
        push (@imgs_need_enc, "uboot_${prj}-sign.bin");
}

foreach my $img (@imgs_need_encrypt) {
	push (@miss_img, $img) if ( ! -e "$dir/$img");
}
die "@miss_img\nall the imgs above is NOT exsit\n" if (@miss_img > 0);

foreach my $img (@imgs_need_enc) {
	if ( ! -e "$dir/$img") {
		warn "the $img is NOT exsit, please check\n";
		next;
	}
	my $enc_img = $img;
	$enc_img =~ s/\./-enc\./;
	print "Encrypt Image '$dir/$img' ...\n";
	&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
	&print_system("$command ENC_RESET_SIGNED_BIN $key $cfg $dir/$img $dir/enc_bin/$enc_img");
}

##########################################################
# ENCRYPT SIGNED IMAGE IF NEEDED (EMMC FILES)
##########################################################
print "\n\n";
print "********************************************\n";
print " ENCRYPT EMMC IMAGE \n";
print "********************************************\n";

my @imgs_need_enc = ("MBR-sign", "EBR1-sign", "EBR2-sign");
foreach my $img (@imgs_need_enc) {
	if (-e "$dir/$img") {		
	        my $enc_img = $img."-enc";
		print "Encrypt Image '$dir/$img' ...\n";		
		&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
	}
}

my @imgs_need_enc = ("cache-sign.img");
foreach my $img (@imgs_need_enc) {
	if (-e "$dir/$img") {		
		my $enc_img = $img;
	        $enc_img =~ s/\./-enc\./;
		print "Encrypt Image '$dir/$img' ...\n";
		&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
	}
}

##########################################################
# ENCRYPT SIGNED IMAGE IF NEEDED (SECRO FILES)
##########################################################
my @imgs_need_enc = ("sro-lock-sign.img", "sro-unlock-sign.img");
foreach my $img (@imgs_need_enc) {
	if (-e "$dir/$img") {		
		my $enc_img = $img;
        	$enc_img =~ s/\./-enc\./;
		print "Encrypt Image '$dir/$img' ...\n";
		&cp_system("cp -f $dir/$img $dir/enc_bin/$enc_img");
	}
}

################################################################
# UTIL
################################################################
sub print_system {
	my $command = $_[0];
	my $rslt = 0;
	if(${ENC_SUPPORT} eq "yes") {
		$rslt = system($command);
	}
	print "$command: $rslt\n";
	die "Failed to execute $command" if ($rslt != 0);
}
sub cp_system {
	my $command = $_[0];
	my $rslt = system($command);
	print "$command: $rslt\n";
	die "Failed to execute $command" if ($rslt != 0);
}
