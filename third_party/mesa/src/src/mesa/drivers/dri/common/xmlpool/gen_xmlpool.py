#!/usr/bin/python

import sys
import gettext
import re

# List of supported languages
languages = sys.argv[1:]

# Escape special characters in C strings
def escapeCString (s):
    escapeSeqs = {'\a' : '\\a', '\b' : '\\b', '\f' : '\\f', '\n' : '\\n',
                  '\r' : '\\r', '\t' : '\\t', '\v' : '\\v', '\\' : '\\\\'}
    # " -> '' is a hack. Quotes (") aren't possible in XML attributes.
    # Better use Unicode characters for typographic quotes in option
    # descriptions and translations.
    i = 0
    r = ''
    while i < len(s):
        # Special case: escape double quote with \u201c or \u201d, depending
        # on whether it's an open or close quote. This is needed because plain
        # double quotes are not possible in XML attributes.
        if s[i] == '"':
            if i == len(s)-1 or s[i+1].isspace():
                # close quote
                q = u'\u201c'
            else:
                # open quote
                q = u'\u201d'
            r = r + q
        elif escapeSeqs.has_key(s[i]):
            r = r + escapeSeqs[s[i]]
        else:
            r = r + s[i]
        i = i + 1
    return r

# Expand escape sequences in C strings (needed for gettext lookup)
def expandCString (s):
    escapeSeqs = {'a' : '\a', 'b' : '\b', 'f' : '\f', 'n' : '\n',
                  'r' : '\r', 't' : '\t', 'v' : '\v',
                  '"' : '"', '\\' : '\\'}
    i = 0
    escape = False
    hexa = False
    octa = False
    num = 0
    digits = 0
    r = ''
    while i < len(s):
        if not escape:
            if s[i] == '\\':
                escape = True
            else:
                r = r + s[i]
        elif hexa:
            if (s[i] >= '0' and s[i] <= '9') or \
               (s[i] >= 'a' and s[i] <= 'f') or \
               (s[i] >= 'A' and s[i] <= 'F'):
                num = num * 16 + int(s[i],16)
                digits = digits + 1
            else:
                digits = 2
            if digits >= 2:
                hexa = False
                escape = False
                r = r + chr(num)
        elif octa:
            if s[i] >= '0' and s[i] <= '7':
                num = num * 8 + int(s[i],8)
                digits = digits + 1
            else:
                digits = 3
            if digits >= 3:
                octa = False
                escape = False
                r = r + chr(num)
        else:
            if escapeSeqs.has_key(s[i]):
                r = r + escapeSeqs[s[i]]
                escape = False
            elif s[i] >= '0' and s[i] <= '7':
                octa = True
                num = int(s[i],8)
                if num <= 3:
                    digits = 1
                else:
                    digits = 2
            elif s[i] == 'x' or s[i] == 'X':
                hexa = True
                num = 0
                digits = 0
            else:
                r = r + s[i]
                escape = False
        i = i + 1
    return r

# Expand matches. The first match is always a DESC or DESC_BEGIN match.
# Subsequent matches are ENUM matches.
#
# DESC, DESC_BEGIN format: \1 \2=<lang> \3 \4=gettext(" \5=<text> \6=") \7
# ENUM format:             \1 \2=gettext(" \3=<text> \4=") \5
def expandMatches (matches, translations, end=None):
    assert len(matches) > 0
    nTranslations = len(translations)
    i = 0
    # Expand the description+enums for all translations
    for lang,trans in translations:
        i = i + 1
        # Make sure that all but the last line of a simple description
        # are extended with a backslash.
        suffix = ''
        if len(matches) == 1 and i < len(translations) and \
               not matches[0].expand (r'\7').endswith('\\'):
            suffix = ' \\'
        # Expand the description line. Need to use ugettext in order to allow
        # non-ascii unicode chars in the original English descriptions.
        text = escapeCString (trans.ugettext (unicode (expandCString (
            matches[0].expand (r'\5')), "utf-8"))).encode("utf-8")
        print matches[0].expand (r'\1' + lang + r'\3"' + text + r'"\7') + suffix
        # Expand any subsequent enum lines
        for match in matches[1:]:
            text = escapeCString (trans.ugettext (unicode (expandCString (
                match.expand (r'\3')), "utf-8"))).encode("utf-8")
            print match.expand (r'\1"' + text + r'"\5')

        # Expand description end
        if end:
            print end,

# Compile a list of translation classes to all supported languages.
# The first translation is always a NullTranslations.
translations = [("en", gettext.NullTranslations())]
for lang in languages:
    try:
        trans = gettext.translation ("options", ".", [lang])
    except IOError:
        sys.stderr.write ("Warning: language '%s' not found.\n" % lang)
        continue
    translations.append ((lang, trans))

# Regular expressions:
reLibintl_h  = re.compile (r'#\s*include\s*<libintl.h>')
reDESC       = re.compile (r'(\s*DRI_CONF_DESC\s*\(\s*)([a-z]+)(\s*,\s*)(gettext\s*\(\s*")(.*)("\s*\))(\s*\)[ \t]*\\?)$')
reDESC_BEGIN = re.compile (r'(\s*DRI_CONF_DESC_BEGIN\s*\(\s*)([a-z]+)(\s*,\s*)(gettext\s*\(\s*")(.*)("\s*\))(\s*\)[ \t]*\\?)$')
reENUM       = re.compile (r'(\s*DRI_CONF_ENUM\s*\([^,]+,\s*)(gettext\s*\(\s*")(.*)("\s*\))(\s*\)[ \t]*\\?)$')
reDESC_END   = re.compile (r'\s*DRI_CONF_DESC_END')

# Print a header
print \
"/***********************************************************************\n" \
" ***        THIS FILE IS GENERATED AUTOMATICALLY. DON'T EDIT!        ***\n" \
" ***********************************************************************/"

# Process the options template and generate options.h with all
# translations.
template = file ("t_options.h", "r")
descMatches = []
for line in template:
    if len(descMatches) > 0:
        matchENUM     = reENUM    .match (line)
        matchDESC_END = reDESC_END.match (line)
        if matchENUM:
            descMatches.append (matchENUM)
        elif matchDESC_END:
            expandMatches (descMatches, translations, line)
            descMatches = []
        else:
            sys.stderr.write (
                "Warning: unexpected line inside description dropped:\n%s\n" \
                % line)
        continue
    if reLibintl_h.search (line):
        # Ignore (comment out) #include <libintl.h>
        print "/* %s * commented out by gen_xmlpool.py */" % line
        continue
    matchDESC       = reDESC      .match (line)
    matchDESC_BEGIN = reDESC_BEGIN.match (line)
    if matchDESC:
        assert len(descMatches) == 0
        expandMatches ([matchDESC], translations)
    elif matchDESC_BEGIN:
        assert len(descMatches) == 0
        descMatches = [matchDESC_BEGIN]
    else:
        print line,

if len(descMatches) > 0:
    sys.stderr.write ("Warning: unterminated description at end of file.\n")
    expandMatches (descMatches, translations)
