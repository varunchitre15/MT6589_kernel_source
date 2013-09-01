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
# Current maintainer 0.40+: John McNamara jmcnamara@cpan.org
# Maintainer 0.27-0.33: Gabor Szabo szabgab@cpan.org
# Original author: Kawai Takanori (Hippo2000) kwitknr@cpan.org

# COPYRIGHT
# Copyright (c) 2009-2010 John McNamara
# Copyright (c) 2006-2008 Gabor Szabo
# Copyright (c) 2000-2006 Kawai Takanori
# All rights reserved. This is free software. You may distribute under the terms of
# the Artistic License(full text of the Artistic License http://dev.perl.org/licenses/artistic.html).

package Spreadsheet::ParseExcel::SaveParser;

###############################################################################
#
# Spreadsheet::ParseExcel::SaveParser - Rewrite an existing Excel file.
#
# Used in conjunction with Spreadsheet::ParseExcel.
#
# Copyright (c) 2009      John McNamara
# Copyright (c) 2006-2008 Gabor Szabo
# Copyright (c) 2000-2006 Kawai Takanori
#
# perltidy with standard settings.
#
# Documentation after __END__
#

use strict;
use warnings;

use Spreadsheet::ParseExcel;
use Spreadsheet::ParseExcel::SaveParser::Workbook;
use Spreadsheet::ParseExcel::SaveParser::Worksheet;
use Spreadsheet::WriteExcel;
use base 'Spreadsheet::ParseExcel';

our $VERSION = '0.57';

###############################################################################
#
# new()
#
sub new {

    my ( $package, %params ) = @_;
    $package->SUPER::new(%params);
}

###############################################################################
#
# Create()
#
sub Create {

    my ( $self, $formatter ) = @_;

    #0. New $workbook
    my $workbook = Spreadsheet::ParseExcel::Workbook->new();
    $workbook->{SheetCount} = 0;

    # User specified formater class.
    if ($formatter) {
        $workbook->{FmtClass} = $formatter;
    }
    else {
        $workbook->{FmtClass} = Spreadsheet::ParseExcel::FmtDefault->new();
    }

    return Spreadsheet::ParseExcel::SaveParser::Workbook->new($workbook);
}

###############################################################################
#
# Parse()
#
sub Parse {

    my ( $self, $sFile, $formatter ) = @_;

    my $workbook = $self->SUPER::Parse( $sFile, $formatter );

    return undef unless defined $workbook;
    return Spreadsheet::ParseExcel::SaveParser::Workbook->new($workbook);
}

###############################################################################
#
# SaveAs()
#
sub SaveAs {

    my ( $self, $workbook, $filename ) = @_;

    $workbook->SaveAs($filename);
}

1;

__END__

=head1 NAME

Spreadsheet::ParseExcel::SaveParser - Rewrite an existing Excel file.

=head1 SYNOPSIS



Say we start with an Excel file that looks like this:

    -----------------------------------------------------
   |   |      A      |      B      |      C      |
    -----------------------------------------------------
   | 1 | Hello       | ...         | ...         |  ...
   | 2 | World       | ...         | ...         |  ...
   | 3 | *Bold text* | ...         | ...         |  ...
   | 4 | ...         | ...         | ...         |  ...
   | 5 | ...         | ...         | ...         |  ...


Then we process it with the following program:

    #!/usr/bin/perl

    use strict;
    use warnings;

    use Spreadsheet::ParseExcel;
    use Spreadsheet::ParseExcel::SaveParser;


    # Open an existing file with SaveParser
    my $parser   = Spreadsheet::ParseExcel::SaveParser->new();
    my $template = $parser->Parse('template.xls');


    # Get the first worksheet.
    my $worksheet = $template->worksheet(0);
    my $row  = 0;
    my $col  = 0;


    # Overwrite the string in cell A1
    $worksheet->AddCell( $row, $col, 'New string' );


    # Add a new string in cell B1
    $worksheet->AddCell( $row, $col + 1, 'Newer' );


    # Add a new string in cell C1 with the format from cell A3.
    my $cell = $worksheet->get_cell( $row + 2, $col );
    my $format_number = $cell->{FormatNo};

    $worksheet->AddCell( $row, $col + 2, 'Newest', $format_number );


    # Write over the existing file or write a new file.
    $template->SaveAs('newfile.xls');


We should now have an Excel file that looks like this:

    -----------------------------------------------------
   |   |      A      |      B      |      C      |
    -----------------------------------------------------
   | 1 | New string  | Newer       | *Newest*    |  ...
   | 2 | World       | ...         | ...         |  ...
   | 3 | *Bold text* | ...         | ...         |  ...
   | 4 | ...         | ...         | ...         |  ...
   | 5 | ...         | ...         | ...         |  ...



=head1 DESCRIPTION

The C<Spreadsheet::ParseExcel::SaveParser> module rewrite an existing Excel file by reading it with C<Spreadsheet::ParseExcel> and rewriting it with C<Spreadsheet::WriteExcel>.

=head1 METHODS

=head1 Parser

=head2 new()

    $parse = new Spreadsheet::ParseExcel::SaveParser();

Constructor.

=head2 Parse()

    $workbook = $parse->Parse($sFileName);

    $workbook = $parse->Parse($sFileName , $formatter);

Returns a L</Workbook> object. If an error occurs, returns undef.

The optional C<$formatter> is a Formatter Class to format the value of cells.


=head1 Workbook

The C<Parse()> method returns a C<Spreadsheet::ParseExcel::SaveParser::Workbook> object.

This is a subclass of the L<Spreadsheet::ParseExcel::Workbook> and has the following methods:

=head2 worksheets()

Returns an array of L</Worksheet> objects. This was most commonly used to iterate over the worksheets in a workbook:

    for my $worksheet ( $workbook->worksheets() ) {
        ...
    }

=head2 worksheet()

The C<worksheet()> method returns a single C<Worksheet> object using either its name or index:

    $worksheet = $workbook->worksheet('Sheet1');
    $worksheet = $workbook->worksheet(0);

Returns C<undef> if the sheet name or index doesn't exist.


=head2 AddWorksheet()

    $workbook = $workbook->AddWorksheet($name, %properties);

Create a new Worksheet object of type C<Spreadsheet::ParseExcel::Worksheet>.

The C<%properties> hash contains the properties of new Worksheet.


=head2 AddFont

    $workbook = $workbook->AddFont(%properties);

Create new Font object of type C<Spreadsheet::ParseExcel::Font>.

The C<%properties> hash contains the properties of new Font.


=head2 AddFormat

    $workbook = $workbook->AddFormat(%properties);

The C<%properties> hash contains the properties of new Font.


=head1 Worksheet

Spreadsheet::ParseExcel::SaveParser::Worksheet

Worksheet is a subclass of Spreadsheet::ParseExcel::Worksheet.
And has these methods :


The C<Worksbook::worksheet()> method returns a C<Spreadsheet::ParseExcel::SaveParser::Worksheet> object.

This is a subclass of the L<Spreadsheet::ParseExcel::Worksheet> and has the following methods:


=head1 AddCell

    $workbook = $worksheet->AddCell($row, $col, $value, $format [$encoding]);

Create new Cell object of type C<Spreadsheet::ParseExcel::Cell>.

The C<$format> parameter is the format number rather than a full format object.

To specify just same as another cell,
you can set it like below:

    $row            = 0;
    $col            = 0;
    $worksheet      = $template->worksheet(0);
    $cell           = $worksheet->get_cell( $row, $col );
    $format_number  = $cell->{FormatNo};

    $worksheet->AddCell($row +1, $coll, 'New data', $format_number);




=head1 TODO

Please note that this module is currently (versions 0.50-0.60) undergoing a major
restructuring and rewriting.

=head1 Known Problems


You can only rewrite the features that Spreadsheet::WriteExcel supports so
macros, graphs and some other features in the original Excel file will be lost.
Also, formulas aren't rewritten, only the result of a formula is written.

Only last print area will remain. (Others will be removed)


=head1 AUTHOR

Maintainer 0.40+: John McNamara jmcnamara@cpan.org

Maintainer 0.27-0.33: Gabor Szabo szabgab@cpan.org

Original author: Kawai Takanori kwitknr@cpan.org

=head1 COPYRIGHT

Copyright (c) 2009-2010 John McNamara

Copyright (c) 2006-2008 Gabor Szabo

Copyright (c) 2000-2002 Kawai Takanori and Nippon-RAD Co. OP Division

All rights reserved.

You may distribute under the terms of either the GNU General Public License or the Artistic License, as specified in the Perl README file.


=cut
