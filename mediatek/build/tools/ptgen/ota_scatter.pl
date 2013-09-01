#!/usr/local/bin/perl
#
#
use strict;

my $ORG_SCATTER = $ARGV[0];
my $NEW_SCATTER = $ARGV[1];
open(READ_FP, "< $ORG_SCATTER") or die "Can not open $ORG_SCATTER";
open(WRITE_FP, "> $NEW_SCATTER") or die "Can not open $NEW_SCATTER";

while (<READ_FP>) {
    if ($_ =~ /^{/) {
        next;
    }
    if ($_ =~ /^}/) {
        next;
    }
    if ($_ =~ /^__NODL_BMTPOOL /) {
        next;
    }
    if ($_ =~ /^__NODL_RSV_BMTPOOL /) {
        next;
    }
    if ($_ =~ /^__NODL_RSV_OTP /) {
        next;
    }
    if ($_ =~ /^\n/) {
        next;
    }
    print WRITE_FP "$_";
}

close(READ_FP);
close(WRITE_FP);

