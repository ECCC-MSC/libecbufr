#!/usr/bin/perl

use strict;
use warnings;
use CGI qw/:standard/;
use CGI::FormBuilder;
use File::Basename;

$ENV{BUFR_TABLES} = '/usr/share/libecbufr/';

sub fix {
	my $string = shift;
	$string =~ s/^\s+//gm;
	return $string;
}

my %exercises = (
	freeform => {
		name => 'freeform',
		title => 'Free Form Template Exercise',
		description => '',
		template => '',
	},
	pressure => {
		name => 'pressure',
		title => 'Pressure Template Exercise',
		template => '',
		description => fix(<<"END"),
			Design a BUFR template that will satisfy the following data
			requirements:
			<ul>
			<li>Be based on the WMO synoptic template (distributed)</li>
			<li>Include pressure requirements that are currently reported
			in Canada in
			the METAR and six-hour synoptic report, either in the body of the
			message or in remarks. These are station pressure, altimeter, MSL
			pressure, pressure tendency, and three-hour pressure change.</li>
			<li>Contain a "barometer as read" value suitable for diagnosing
			errors in
			barometry constants files.</li>
			<li>Assume that the station has already been identified elsewhere.</li>
			<li>Use the excerpted BUFR tables B and D provided for the
			exercise.</li>
			</ul>
END
		tableb => [qw/002002 002003 002030 004025 004032 007031
			007032 008021 008023 010004 010051 010052 010060 010061
			010063 011001 011002 011010 011011 011012 011040 011041
			011042 011043 011046 011047 010194 010197 010210 010211 011195/],
		tabled => [qw/302001 302031 302042 302070/ ],
	},
	wind => {
		name => 'wind',
		title => 'Wind Template Exercise',
		template => '',
		description => fix(<<"END"),
			Design a BUFR template that will satisfy the following data
			requirements:
			<ul>
			<li>Be based on the WMO synoptic template (distributed)</li>

			<li>Include 10-minute wind speed and direction,
			gusts within the last hour, and gusts over the
			last six hours.</li>

			<li>Assume that the station has already been
			identified elsewhere</li>
			</ul>
END
		tableb => [qw/002002 002003 002030 004025 004032 007031
			007032 008021 008023 010004 010051 010052 010060 010061
			010063 011001 011002 011010 011011 011012 011040 011041
			011042 011043 011046 011047 010194 010197 010210 010211 011195/],
		tabled => [qw/302001 302031 302042 302070/ ],
	},
	increment => {
		name => 'increment',
		title => 'Location/Time Increment Encoding Exercise',
		template => fix(<<"END"),
			BUFR_EDITION=4
			004024,VALUE=-6
			004014,VALUE=6
			110000
			# GLOBAL:13  REGIONALE:9
			031001,VALUE=13
			106003
			001032
			033045
			241000
			033042,VALUE=1
			013011,VALUE=0.2
			241255
			001032,VALUE=19
			206007
			014203
			# GLOBAL:-75  REGIONALE:-51
			004024,VALUE=-75
			004014,VALUE=3
			105000
			# GLOBAL:25  REGIONALE:17
			031001,VALUE=25
			001032,VALUE=10
			012104
			020010
			011011
			011012
END
		description => fix(<<"END"),
<ul>
<li>
Change the current time increment values from Global to Regional,
re-encode to see what happens.</li>
<li>Make changes so that  Forecast time now range from
0 to 144 with stepping=3.</li>
</ul>
END
	},
);

my @fields = qw(exercise template template_file describe);

my $query = CGI->new();
my $form = CGI::FormBuilder->new(
	method => 'post',
	params => $query,
	fields => \@fields,
	submit => [qw/Encode Download/],
	reset  => 0,         # turn off reset button
);

my $exname = $form->field('exercise');
my $exercise = $exercises{$exname} || $exercises{freeform};

$form->field( name => 'exercise',
					type => 'hidden',
					options => [ sort(keys %exercises) ],
					);
$form->field(name  => 'template',
				 type  => 'textarea',
				 rows  => 24,
				 cols  => 80,
				 value => $exercise->{template},
				 );
$form->field(
	name => 'template_file',
	type => 'file',
);
$form->field(
	name => 'describe',
	type => 'checkbox',
	label => 'Include descriptions of BUFR descriptors',
	options => ['Yes'],
);

if( defined $exercise ) {
	my $tableb = extract_table($exercise->{tableb},
			$ENV{BUFR_TABLES} . '/table_b_bufr');
	my $tabled = extract_table($exercise->{tabled},
			$ENV{BUFR_TABLES} . '/table_d_bufr');

	my $template;
	if( $form->submitted && $form->validate) {
		my $file = $form->field('template_file');
		if( defined $file and length($file)>0 ) {
			while(<$file>) {
				$template .= $_ if m/^(BUFR_EDITION|[0-9#]+)/o;
			}

			# so it's in the next dialog
			$form->field(name => 'template', value => $template, force => 1);
		} else {
			$template = $form->field('template');
		}

		# strip out unencodable stuff
		my @lines = split /\n+/, $template;
		$template = join("\n", grep { m/^(BUFR_EDITION|[0-9#]+)/o } @lines);
	}

	if( $form->submitted eq 'Download' && $form->validate ) {
		print $query->header(-type => 'octet/stream', 
									-cache_control => 'no-cache');
		my $encoded = do_encode( $template, $tableb, $tabled );

		print $encoded;
		exit 0;
	}

	print $query->header(-cache_control => 'no-cache');
	print $query->start_html('title' => $exercise->{title});
	print $query->h1($exercise->{title}), $exercise->{description};

	print $form->render();

	if( $template ) {
		print $query->h2('Encoder Results');

		my $encoded = do_encode( $template, $tableb, $tabled );
		my $d = $form->field('describe');
		my $decoded = do_decode( $encoded,
			(defined $d and $d eq 'Yes') );

		print $query->pre($decoded);
	}

	# output table B, table D examples
	print $query->h2('Table B'), $query->pre($tableb);
	print $query->h2('Table D'), $query->pre($tabled);

} else {
	print $query->start_html('title' => 'Template Exercise');
	$query->h1('Template Exercise');

	print $form->render();
}

print $query->end_html();

sub extract_table {
	my $descriptors = shift;
	my $table;
	my $tablefile = shift;
	if( defined $descriptors ) {
		my $re = '^(' . join('|',@{$descriptors}) . ')\s+';
		my $qre = qr($re);

		open( my $b, '<', $tablefile) || die;
		while( <$b> ) {
			$table .= $_ if $_ =~ $qre;
		}
		close $b;
	} else {
		open( my $b, '<', $tablefile) || die;
		$table .= $_ while( <$b> );
		close $b;
	}

	return $table;
}

sub do_decode {
	my $data = shift;
	my $describe = shift;

	my $bn = basename($0);
	my $tmpdata = "/tmp/${bn}-$$.bufr";
	my $tmpout = "/tmp/${bn}-$$.out";

	open(my $f, '>', $tmpdata) || die;
	print $f $data;
	close $f;

	my @extra;
	push @extra, '-describe', '-loctime' if $describe;
	push @extra, '-ltableb', $ENV{BUFR_TABLES} . '/table_b_bufr-cmc'
		if -f $ENV{BUFR_TABLES} . '/table_b_bufr-cmc';
	push @extra, '-ltabled', $ENV{BUFR_TABLES} . '/table_d_bufr-cmc'
		if -f $ENV{BUFR_TABLES} . '/table_d_bufr-cmc';

	open( my $olderr, ">&", \*STDERR );
	open STDERR, ">&STDOUT";
	select STDERR; $| = 1;
	select STDOUT;
	my $errs = '';
	open (my $output, '-|', '/usr/bin/bufr_decoder',
		'-inbufr' => $tmpdata,
		'-output' => $tmpout,
		@extra,
		) || die;
	$errs .= $_ while(<$output>);
	close $output;
	open STDERR, ">&$olderr";

	my $d = '';
	if( -f $tmpout ) {
		open($f, '<', $tmpout);
		$d .= $_ while(<$f>);
		close $f;
		unlink $tmpout;
	}
	return $errs . "\n" . $d;
}

sub do_encode {
	my $template = shift;
	my $tableb = shift;
	my $tabled = shift;

	my $bn = basename($0);
	my $tmptmpl = "/tmp/${bn}-$$.template";
	my $tmpbufr = "/tmp/${bn}-$$.bufr";
	my $tmptableb = "/tmp/${bn}-$$.tableb";
	my $tmptabled = "/tmp/${bn}-$$.tabled";

	open(my $f, '>', $tmptmpl) || die;
	print $f $template;
	close $f;

	open($f, '>', $tmptableb) || die;
	print $f $tableb;
	close $f;

	open($f, '>', $tmptabled) || die;
	print $f $tabled;
	close $f;

	print $query->start_pre();
	open( my $olderr,     ">&", \*STDERR );
	open STDERR, ">&STDOUT";
	select STDERR; $| = 1;
	system( '/usr/bin/bufr_encoder',
		'-template' => $tmptmpl,
		'-ltableb' => $tmptableb,
		'-ltabled' => $tmptabled,
		'-outbufr' => $tmpbufr,
		'-loctime', '-nolocal',
	);
	open STDERR, ">&$olderr";
	print $query->end_pre();

	unlink $tmptmpl;
	unlink $tmptableb;
	unlink $tmptabled;
	
	my $encoded;

	if( -f $tmpbufr ) {
		$encoded = '';
		open(my $f, '<', $tmpbufr) || die;
		$encoded .= $_ while(<$f>);
		close $f;
		unlink $tmpbufr;
	}

	return $encoded;
}

1;
