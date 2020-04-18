# Introduction

Liblouis is an open-source braille translator and back-translator
named in honor of [Louis Braille][]. It features support for computer
and literary braille, supports contracted and uncontracted translation
for [many languages][] and has support for hyphenation. New languages
can easily be added through tables that support a rule- or dictionary
based approach. Tools for testing and debugging tables are also
included. Liblouis also supports math braille (Nemeth and Marburg).

Liblouis has features to support screen-reading programs. This has led
to its use in two open-source screenreaders, [NVDA][] and [Orca][]. It
is also used in some commercial assistive technology applications for
example by [ViewPlus][].

Liblouis is based on the translation routines in the [BRLTTY][]
screenreader for Linux. It has, however, gone far beyond these
routines. In Linux and Mac OSX it is a shared library, and in Windows
it is a DLL.

Liblouis is free software licensed under the [GNU Lesser GPL][] (see
the file COPYING.LIB).

The command line tools, are licensed under the [GNU GPL][] (see the
file COPYING).

# Documentation

For documentation, see the [liblouis documentation][] (either as info
file, html, txt or pdf) in the doc directory. For examples
of translation tables, see `en-us-g2.ctb`, `en-us-g1.ctb`,
`chardefs.cti`, and whatever other files they may include in the
tables directory. This directory contains tables for many languages.
The Nemeth files will only work with the sister library
[liblouisutdml][].

# Installation

After unpacking the distribution tarball go to the directory it creates. 
You now have the choice to compile liblouis for either 16- or 32-bit
unicode. By default it is compiled for the former. To get 32-bit Unicode
run configure with `--enable-ucs4`.

After running configure run `make` and then `make install`. You must
have root privileges for the installation step.

This will produce the liblouis library and the programs `lou_allround`
(for testing the library), `lou_checkhyphens`, `lou_checktable` (for
checking translation tables), `lou_debug` (for debugging translation
tables), `lou_translate` (for extensive testing of forward and
backwards translation) and `lou_trace` (for tracing if individual
translations). For more details see the liblouis documentation.

If you wish to have man pages for the programs you might want to
install `help2man` before running configure.

# Release Notes

For notes on the newest and older releases see the file NEWS.

# History

Liblouis was begun in 2002 largely as a business decision by
[ViewPlus][]. They believed that they could never have good braille
except as part of an open source effort and knew that John Boyer was
dying to start just such a project. So ViewPlus did start it on the
agreement that they would give a small monthly stipend to John Boyer
that allowed him to pay for sighted assistants. While ViewPlus has not
contributed much to the coding, it certainly has contributed and
continues to contribute to liblouis through that support of John
Boyer.

[Louis Braille]: http://en.wikipedia.org/wiki/Louis_Braille
[many languages]: https://github.com/liblouis/liblouis/tree/master/tables
[NVDA]: http://www.nvda-project.org/
[Orca]: http://live.gnome.org/Orca
[ViewPlus]: http://www.viewplus.com
[BRLTTY]: http://mielke.cc/brltty/
[GNU Lesser GPL]: https://www.gnu.org/licenses/lgpl.html
[GNU GPL]: https://www.gnu.org/licenses/gpl.html
[liblouisutdml]: http://www.liblouis.org/
[liblouis documentation]: http://www.liblouis.org/documentation/liblouis.html

<!-- Local Variables: -->
<!-- mode: markdown -->
<!-- End: -->
