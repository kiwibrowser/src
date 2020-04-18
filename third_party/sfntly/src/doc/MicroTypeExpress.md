# MicroType Express in sfntly

This page describes sfntly’s implementation of MicroType Express
compression.

The MicroType Express format is [documented](http://www.w3.org/Submission/MTX/),
and is recently available under license terms we believe are friendly to
open source (as well as proprietary) implementation. The most popular
implementation of a decoder is Internet Explorer, as a compression
option in its Embedded OpenType (EOT) format. Since MicroType Express
gives an approximate 15% gain in compression over gzip, a web font
service striving for maximum performance can benefit from implementing
EOT compression.

The current codebase in sfntly is for compression only (this is by far
the most useful for web serving). The easiest way to get started is with
the command line tool, sfnttool:

```
sfnttool -e -x source.ttf source.eot
```

The code has been tested extensively against IE, and is currently in
production in Google Web Fonts, both for full fonts and for subsetting
(using the text= parameter). This serving path also uses sfntly to
compute the subsets, and is essentially the same as the -s parameter to
sfnttool.

If you’re interested in the code and details of the compression
algorithm, here’s a bit of a guide:

The top-level MicroType Express compression code is in MtxWriter.java
(in the tools/conversion/eot directory). This code implements almost all
of the MicroType Express format, including the hdmx tables, push
sequences, and jump coding. The main feature missing is the VDMX table
(vertical device metrics), which is very rarely used in web fonts.

Patches are welcome -- possible areas include: implementing the VDMX
table, speeding up the LZCOMP entropy coder (the match finding code is a
straightforward adaptation of the algorithm in the format document),
implementing a decoder in addition to an encoder, and porting the Java
implementation to C++.
