use Net::SMTP;
require 5.001;

chomp($whichOS = `set OS`);
$isWin = 0;

if ($whichOS eq "OS=Windows_NT") {
  $dirDlm = "\\";
  $catCmd = 'type';
  $isWin = 1;
} else {
  $dirDlm = "/";
  $catCmd = 'cat';
}

sub CurrDateStr {
  my($sec, $min, $hour, $mday, $mon, $year)  = localtime(time);
  return (sprintf "%4.4d_%2.2d_%2.2d", $year+1900, $mon+1, $mday);
}

sub CurrTimeStr {
  my($sec, $min, $hour, $mday, $mon) = localtime(time);
  return (sprintf "%2.2d/%2.2d %2.2d:%2.2d:%2.2d", $mon+1, $mday, $hour, $min, $sec);
}

sub SendAdmEmail {
	return;
}

sub SendMail {
}

sub EndOfMail {
  my $msg;

  return $msg;
}

sub HeadOfMail {
  my $msg;

  chomp($serverName = `uname -n`);
  chomp($fullPath = `pwd`);
  $msg .= "Server name: $serverName\nPath:$fullPath\n\n" ;
  return $msg;
}

1;

