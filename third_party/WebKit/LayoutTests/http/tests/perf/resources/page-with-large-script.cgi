#!/usr/bin/perl -wT
use strict;

my (@out, $i, $n, $line);

push(@out, "Content-type: text/html\n");
push(@out, "\n");
push(@out, "<html>\n");
push(@out, "  <head>\n");
push(@out, "    <script>\n");

push(@out, "function foo() {\n");
push(@out, "  var q;\n");
$n = int($ENV{'QUERY_STRING'});
$line = "  q = 0; q = 1; q = 2; q = 3; q = 4; q = 5; q = 6; q = 7; q = 8; q = 9;\n";
for ($i = 0; $i < $n; $i++) {
    push(@out, $line);
}
push(@out, "}\n");

push(@out, "    </script>\n");
push(@out, "  </head>\n");
push(@out, "  <body onload='parent._iframeLoaded();'>\n");
push(@out, "Bar\n");
push(@out, "  </body>\n");
push(@out, "</html>\n");
print join("", @out);
