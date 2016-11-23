#!/usr/bin/perl

use strict;
use warnings;
use CGI qw/:standard/;
use CGI::FormBuilder;
use File::Basename;

my @fields = qw(example file describe);
my $exdir = '/var/www/libecbufr/ex';
my @examples = (map { basename($_) } <${exdir}/*>);
$ENV{BUFR_TABLES} = '/usr/share/libecbufr/';

my $query = CGI->new();
my $form = CGI::FormBuilder->new(
	method => 'post',
	params => $query,
	fields => \@fields,
	submit => [qw/Decode/],
	enctype => 'multipart/form-data',
	#reset => 1,
);
$form->field(
	name => 'example',
	type => 'select',
	options => \@examples,
);
$form->field(
	name => 'file',
	type => 'file',
);
$form->field(
	name => 'describe',
	type => 'checkbox',
	label => 'Include descriptions of BUFR descriptors',
	options => ['Yes'],
);

print $query->header(-cache_control => 'no-cache'),
	$query->start_html('title' => 'Pressure Template'),
	$form->render();

if( $form->submitted ) {
	my $data;
	my $file = $form->field('file');
	if( defined $file and length($file)>0 ) {
		print $query->h3($file);
		$data .= $_ while(<$file>);
	} else {
		my $example = $form->field('example');
		print $query->h3($example);
		my $fn = $exdir . '/' . $example;
		if( -f $fn and $fn !~ /\.\./ ) {
			$data = '';
			open (my $f,'<',$fn) || die;
			$data .= $_ while(<$f>);
			close $f;
		}
	}

	if( defined $data and length($data)>0 ) {
		my $d = $form->field('describe');
		my $decoded = do_decode( $data,
			(defined $d and $d eq 'Yes') );
		print $query->pre($decoded) if $decoded;
	}
}

print $query->end_html();
	
exit 0;

sub do_decode {
	my $data = shift;
	my $describe = shift;

	my $bn = basename($0);
	my $tmpdata = "/tmp/${bn}-$$.bufr";
	my $tmpout = "/tmp/${bn}-$$.out";

	open(my $f, '>', $tmpdata) || die;
	print $f $data;
	close $f;

	open (my $olderr, ">&", \*STDERR);
	open STDERR, ">&STDOUT";
	select STDERR; $| = 1;
	my $errs = '';

	my @extra;
	push @extra, '-describe' if $describe;

	push @extra, '-ltableb', $ENV{BUFR_TABLES} . '/table_b_bufr-cmc'
		if -f $ENV{BUFR_TABLES} . '/table_b_bufr-cmc';
	push @extra, '-ltabled', $ENV{BUFR_TABLES} . '/table_d_bufr-cmc'
		if -f $ENV{BUFR_TABLES} . '/table_d_bufr-cmc';

	open (my $output, '-|', '/usr/bin/bufr_decoder',
		'-inbufr' => $tmpdata,
		'-output' => $tmpout,
		'-loctime',
		@extra,
		) || die;
	$errs .= $_ while(<$output>);
	close $output;
	open STDERR, ">&$olderr";

	unlink $tmpdata;

	my $d = '';
	if( -f $tmpout ) {
		open($f, '<', $tmpout);
		$d .= $_ while(<$f>);
		close $f;
		unlink $tmpout;
	}
	return $errs . "\n" . $d;
}
