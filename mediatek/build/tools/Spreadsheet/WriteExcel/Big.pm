# DISCLAIMER OF WARRANTY
# Because this software is licensed free of charge, there is no warranty for the software,
# to the extent permitted by applicable law. Except when otherwise stated in writing
# the copyright holders and/or other parties provide the software "as is" without
# warranty of any kind, either expressed or implied, including, but not limited to,
# the implied warranties of merchantability and fitness for a particular purpose.
# The entire risk as to the quality and performance of the software is with you.
# Should the software prove defective, you assume the cost of all necessary
# servicing, repair, or correction.

# In no event unless required by applicable law or agreed to in writing will any
# copyright holder, or any other party who may modify and/or redistribute the software
# as permitted by the above licence, be liable to you for damages, including any general,
# special, incidental, or consequential damages arising out of the use or inability
# to use the software (including but not limited to loss of data or data being rendered
# inaccurate or losses sustained by you or third parties or a failure of the software
# to operate with any other software), even if such holder or other party
# has been advised of the possibility of such damages.

# AUTHOR
# John McNamara jmcnamara@cpan.org

# COPYRIGHT
# Copyright MM-MMX, John McNamara.
# All Rights Reserved. This module is free software. It may be used, 
# redistributed and/or modified under the terms of 
# the Artistic License(full text of the Artistic License http://dev.perl.org/licenses/artistic.html).

package Spreadsheet::WriteExcel::Big;

###############################################################################
#
# WriteExcel::Big
#
# Spreadsheet::WriteExcel - Write formatted text and numbers to a
# cross-platform Excel binary file.
#
# Copyright 2000-2010, John McNamara.
#
#

require Exporter;

use strict;
use Spreadsheet::WriteExcel::Workbook;






use vars qw($VERSION @ISA);
@ISA = qw(Spreadsheet::WriteExcel::Workbook Exporter);

$VERSION = '2.37';

###############################################################################
#
# new()
#
# Constructor. Thin wrapper for a Workbook object.
#
# This module is no longer required directly and its use is deprecated. See
# the Pod documentation below.
#
sub new {

    my $class = shift;
    my $self  = Spreadsheet::WriteExcel::Workbook->new(@_);

    # Check for file creation failures before re-blessing
    bless  $self, $class if defined $self;

    return $self;
}


1;


__END__



=head1 NAME


Big - A class for creating Excel files > 7MB.


=head1 SYNOPSIS

Use of this module is deprecated. See below.


=head1 DESCRIPTION

The module was a sub-class of Spreadsheet::WriteExcel used for creating Excel files greater than 7MB. However, it is no longer required and is now deprecated.

As of version 2.17 Spreadsheet::WriteExcel can create files larger than 7MB directly if OLE::Storage_Lite is installed.

This module only exists for backwards compatibility. If your programs use ::Big you should convert them to use Spreadsheet::WritExcel directly.


=head1 REQUIREMENTS

L<OLE::Storage_Lite>.


=head1 AUTHOR


John McNamara jmcnamara@cpan.org


=head1 COPYRIGHT


© MM-MMX, John McNamara.


All Rights Reserved. This module is free software. It may be used, redistributed and/or modified under the same terms as Perl itself.
