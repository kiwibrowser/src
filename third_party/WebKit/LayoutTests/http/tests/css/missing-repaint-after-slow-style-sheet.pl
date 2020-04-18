#!/usr/bin/perl -wT

my $pre_chunked = ($ENV{"SERVER_SOFTWARE"} && index($ENV{"SERVER_SOFTWARE"}, "LightTPD") != -1);

sub print_maybe_chunked
{
    my $string = shift;
    if ($pre_chunked) {
        print $string;
        return;
    }
    printf "%lx\r\n", length($string);
    print "$string\r\n";
}

print "Content-type: text/html\r\n";
if (!$pre_chunked) {
    print "Transfer-encoding: chunked\r\n";
}

select (STDOUT);
$| = 1;

print "\r\n";
print_maybe_chunked "<!DOCTYPE html><style>h2 { background-color: green; }</style>";
print_maybe_chunked "<link rel='stylesheet' onload='start()'>";
print_maybe_chunked "<body><script>document.getElementsByTagName('link')[0].href='resources/slow-loading-sheet.php?color=green&sleep=500000'; document.body.offsetTop; requestAnimationFrame(function() { console.log('requestAnimationFrame ran'); });";
print_maybe_chunked "function start() { console.log('Stylesheet loaded'); } console.log('Inline script done');</script>";
print_maybe_chunked "<h1>Styled by external stylesheet</h1><div style='height: 200px'></div><h2>Noncomposited</h2><h2 style='transform:translateZ(0)'>Composited</h2>";
sleep(1);
print_maybe_chunked "";
