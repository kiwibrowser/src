#!perl

use Data::Dumper;

my @test_files = split('\s+',`ls generic/t.* xdg-*/t.*`);

my $cvs_pre = "http://webcvs.freedesktop.org/portland/portland/xdg-utils/tests/";
my $cvs_post = '?view=markup';
my $assert_doc = "assertions.html";
my $now = scalar localtime;
my $style = "<style type=\"text/css\" media=\"all\">@import url(\"layout.css\");</style></head>\n"; 
my $root_header = qq{| <a href="index.html">Tests</a> | <a href="$assert_doc">Assertions</a> | <a href="http://portland.freedesktop.org/wiki/WritingXdgTests">Overview</a> |<hr/>\n};
my $group_header = qq{| <a href="../index.html">Tests</a> | <a href="../$assert_doc">Assertions</a> | <a href="http://portland.freedesktop.org/wiki/WritingXdgTests">Overview</a> |<hr/>\n};
my $footer = "<hr><font size=\"-1\">xdg-utils test documentation generated $now</font>\n";

my %fcns;

my %group;
my %shortdesc;

## Read assertion file
open IN, 'include/testassertions.sh' or die "Failed to open assertion file: $!\n";
my $state = 'NULL';
my %assertions;
while ( <IN> ) {
	if ( m/(\w+)\s*\(\)/ ) {
		$state = $1;
		$assertions{$state} = ();
	}
	elsif ( $state ne 'NULL' and m/^#(.*)/ ) {
		my $txt = $1;
		chomp $txt;
		push @{ $assertions{$state} }, $txt;
	}
	else {
		$state = 'NULL';
	}
}
close IN;

if ( ! -d 'doc' ) { mkdir 'doc'; }

open OUT, ">doc/$assert_doc" or die "Failed to open $assert_doc: $!\n";
print OUT "<html><head><title>xdg-utils test assertions</title>$style</head><body>\n$root_header";

my @s_assert = sort keys %assertions ;
print OUT qq{<h2>Defined Assertions in <a href="$cvs_pre}.qq{include/testassertions.sh$cvs_post">include/testassertions.sh</a></h2>\n};
for $a ( @s_assert ) {
	print OUT qq{<a href="#$a">$a</a><br>\n};
}
for $a ( @s_assert ) {
	print OUT qq{<hr><h2><a name="$a">$a</a></h2>\n};
	print OUT "<pre>", join("\n",@{ $assertions{$a} } ), "</pre>\n";
}
print OUT "$footer</body></html>";


## Read test files
for $f ( @test_files ) {
	open IN, $f or die "Failed to open $f: $!\n";
	$f =~ m{(.+)/t\.(.+)};
	my $dir = $1;
	my $test = $2;
	`mkdir -p doc/$dir`;
	my $o = "doc/$dir/$test.html";

	push @{ $group{$dir} }, $test;

	open HTM, ">$o" or die "Failed to open '$o': $!\n";
	print HTM "<html><head><title>xdg-utils test: $f</title>\n";
	print HTM $style;
	
	print HTM "<body>$group_header<h1>Test: <a href=\"$cvs_pre$f$cvs_post\">$f</a></h1><hr/>\n";
	my $fcn = '';
	my $state = 'BEGIN';
	while ( <IN> ) {
		#find the test function
		if ( m/(\w+)\s*\(\)/ ) {
			$fcn = $1;
			if (defined $fcns{$fcn} ){
				print "WARNING in $f: $fcn already exists in $fcns{$fcn}!\n"
			}
			$fcns{$fcn} = $f;
			$state = 'FUNCTION';
		}
		#find test_start
		elsif ( m/test_start (.*)/ ) {
			print HTM "<h2>Purpose of $fcn</h2>";
			my $txt = $1;
			$txt =~ s/\$FUNCNAME:*\s*//;
			$txt =~ s/\"//g;
			$shortdesc{ $test } = $txt;
			print HTM "<p>$txt</p>\n";
			$state = 'START';
		}
		#find test_purpose
		elsif ( m/test_purpose (.*)/ ) {
			print HTM "<h2>Description</h2>";
			my $txt = $1;
			$txt =~ s/\"//g;
			print HTM "<p>$txt</p>\n";
		}
		#find initilization
		elsif ( m/test_init/ ) {
			print HTM "<h2>Depencencies</h2>\n";
			$state = 'INIT';
			next;
		}
		elsif ( m/test_procedure/ ) {
			print HTM "<h2>Test Procedure</h2>\n";
			$state = 'TEST';
			next;
		}
		elsif ( m/test_note (.*)/ ) {
			print HTM "<h2>Note</h2><p>$1</p>\n";
			next;
		}
		elsif ( m/test_result/ ) {
			$state = 'DONE';
		}
		if ( m/^#/ ) {
			next;
		}
		if ( $state eq 'INIT' or $state eq 'TEST' ) {
			$line = $_;
			$line =~ s/^\s*(\w+)/<a href="\.\.\/$assert_doc#$1">$1<\/a>/;
			if ( $assertions{$1} ) {
				print HTM "<p>$line</p>\n";
				#print "$f:\t'$1' found\n";
			}
			else {
				#print "$f:\t'$1' not found\n";
				print HTM "<p>$_</p>\n";
			}
			#print HTM "<p>$_</p>\n";
		}
	}
	print HTM "$footer</body></html>\n";
	close HTM;
	close IN;
}

open INDEX, ">doc/index.html" or die "Could not open index: $!";
print INDEX "<html><head><title>xdg-utils test suite</title>\n";
print INDEX $style;

print INDEX "<body>$root_header<h1>xdg-utils test documentation</h1>";

my @s_groups = sort keys %group;

for $g ( @s_groups ) {
	print INDEX qq{<a href="#$g">$g</a> \n};
}
print INDEX "<table border=0>\n";
for $k ( @s_groups ) {
	print INDEX qq{<tr><td colspan=2><hr><h2><a name="$k">$k</a></h2></td></tr>\n};
	for $i ( @{ $group{$k} } ) {
		print INDEX "<tr><td><a href=\"$k/$i.html\">$i</a></td><td>$shortdesc{$i}</td></tr>\n";
	}
}
print INDEX "</table>$footer</body></html>\n";
close INDEX;

#print Dumper keys %assertions;
