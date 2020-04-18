# markdown is released under the BSD license
# Copyright 2007, 2008 The Python Markdown Project (v. 1.7 and later)
# Copyright 2004, 2005, 2006 Yuri Takhteyev (v. 0.2-1.6b)
# Copyright 2004 Manfred Stienstra (the original version)
# 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# *   Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# *   Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# *   Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE PYTHON MARKDOWN PROJECT ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL ANY CONTRIBUTORS TO THE PYTHON MARKDOWN PROJECT
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


'''
Abbreviation Extension for Python-Markdown
==========================================

This extension adds abbreviation handling to Python-Markdown.

Simple Usage:

    >>> import markdown
    >>> text = """
    ... Some text with an ABBR and a REF. Ignore REFERENCE and ref.
    ...
    ... *[ABBR]: Abbreviation
    ... *[REF]: Abbreviation Reference
    ... """
    >>> print markdown.markdown(text, ['abbr'])
    <p>Some text with an <abbr title="Abbreviation">ABBR</abbr> and a <abbr title="Abbreviation Reference">REF</abbr>. Ignore REFERENCE and ref.</p>

Copyright 2007-2008
* [Waylan Limberg](http://achinghead.com/)
* [Seemant Kulleen](http://www.kulleen.org/)
	

'''

from __future__ import absolute_import
from __future__ import unicode_literals
from . import Extension
from ..preprocessors import Preprocessor
from ..inlinepatterns import Pattern
from ..util import etree
import re

# Global Vars
ABBR_REF_RE = re.compile(r'[*]\[(?P<abbr>[^\]]*)\][ ]?:\s*(?P<title>.*)')

class AbbrExtension(Extension):
    """ Abbreviation Extension for Python-Markdown. """

    def extendMarkdown(self, md, md_globals):
        """ Insert AbbrPreprocessor before ReferencePreprocessor. """
        md.preprocessors.add('abbr', AbbrPreprocessor(md), '<reference')
        
           
class AbbrPreprocessor(Preprocessor):
    """ Abbreviation Preprocessor - parse text for abbr references. """

    def run(self, lines):
        '''
        Find and remove all Abbreviation references from the text.
        Each reference is set as a new AbbrPattern in the markdown instance.
        
        '''
        new_text = []
        for line in lines:
            m = ABBR_REF_RE.match(line)
            if m:
                abbr = m.group('abbr').strip()
                title = m.group('title').strip()
                self.markdown.inlinePatterns['abbr-%s'%abbr] = \
                    AbbrPattern(self._generate_pattern(abbr), title)
            else:
                new_text.append(line)
        return new_text
    
    def _generate_pattern(self, text):
        '''
        Given a string, returns an regex pattern to match that string. 
        
        'HTML' -> r'(?P<abbr>[H][T][M][L])' 
        
        Note: we force each char as a literal match (in brackets) as we don't 
        know what they will be beforehand.

        '''
        chars = list(text)
        for i in range(len(chars)):
            chars[i] = r'[%s]' % chars[i]
        return r'(?P<abbr>\b%s\b)' % (r''.join(chars))


class AbbrPattern(Pattern):
    """ Abbreviation inline pattern. """

    def __init__(self, pattern, title):
        super(AbbrPattern, self).__init__(pattern)
        self.title = title

    def handleMatch(self, m):
        abbr = etree.Element('abbr')
        abbr.text = m.group('abbr')
        abbr.set('title', self.title)
        return abbr

def makeExtension(configs=None):
    return AbbrExtension(configs=configs)
