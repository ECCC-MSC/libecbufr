#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

my $title = 'BUFR Workshop Examples and Exercises';

open STDOUT, "> index.html" || die;

print <<END ;
<html>
<head>
<title>$title</title>
</head>
<frameset cols="25%,*">
	<frame src="nav.html">
	<frame name="_content" src="home.html">
</frameset>
</html>
END

open STDOUT, "> home.html" || die;
print <<END ;
<html>
<head>
<title>$title</title>
</head>
<body>
Welcome. Bienvenue.
</body>
</html>
END

open STDOUT, "> nav.html" || die;

print <<END ;
<html>
<body>
<h3>Templates:</h3>
<ul>
<li><a href="/cgi-bin/libecbufr/template.pl?exercise=pressure" target="_content">
Pressure</a></li>
<li><a href="/cgi-bin/libecbufr/template.pl?exercise=wind" target="_content">
Wind</a></li>
<li><a href="/cgi-bin/libecbufr/template.pl?exercise=increment" target="_content">
Increments</a></li>
<li><a href="/cgi-bin/libecbufr/template.pl?exercise=freeform" target="_content">
Free Form</a></li>
</ul>
<h3>
<a href="/cgi-bin/libecbufr/decode.pl?" target="_content">Decoder</a></h3>
<ul>
END

sub size {
	my $f = shift;
	my $s = -s $f;
	return ($s<1024) ? "${s}b" : (int($s/1024) . "k");
}

for( @ARGV ) {
	print qq(<li><a href="/cgi-bin/libecbufr/decode.pl?example=),
		basename($_), qq(" target="_content">),
		basename($_), qq(</a> ),
		size($_),
		qq( <a href="$_" target="_content">get</a>),
		qq(</li>);
}

print <<END ;
	</ul>
	</body>
	</html>
END

exit 0;
