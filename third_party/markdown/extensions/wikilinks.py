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
WikiLinks Extension for Python-Markdown
======================================

Converts [[WikiLinks]] to relative links.  Requires Python-Markdown 2.0+

Basic usage:

    >>> import markdown
    >>> text = "Some text with a [[WikiLink]]."
    >>> html = markdown.markdown(text, ['wikilinks'])
    >>> print html
    <p>Some text with a <a class="wikilink" href="/WikiLink/">WikiLink</a>.</p>

Whitespace behavior:

    >>> print markdown.markdown('[[ foo bar_baz ]]', ['wikilinks'])
    <p><a class="wikilink" href="/foo_bar_baz/">foo bar_baz</a></p>
    >>> print markdown.markdown('foo [[ ]] bar', ['wikilinks'])
    <p>foo  bar</p>

To define custom settings the simple way:

    >>> print markdown.markdown(text, 
    ...     ['wikilinks(base_url=/wiki/,end_url=.html,html_class=foo)']
    ... )
    <p>Some text with a <a class="foo" href="/wiki/WikiLink.html">WikiLink</a>.</p>
    
Custom settings the complex way:

    >>> md = markdown.Markdown(
    ...     extensions = ['wikilinks'], 
    ...     extension_configs = {'wikilinks': [
    ...                                 ('base_url', 'http://example.com/'), 
    ...                                 ('end_url', '.html'),
    ...                                 ('html_class', '') ]},
    ...     safe_mode = True)
    >>> print md.convert(text)
    <p>Some text with a <a href="http://example.com/WikiLink.html">WikiLink</a>.</p>

Use MetaData with mdx_meta.py (Note the blank html_class in MetaData):

    >>> text = """wiki_base_url: http://example.com/
    ... wiki_end_url:   .html
    ... wiki_html_class:
    ...
    ... Some text with a [[WikiLink]]."""
    >>> md = markdown.Markdown(extensions=['meta', 'wikilinks'])
    >>> print md.convert(text)
    <p>Some text with a <a href="http://example.com/WikiLink.html">WikiLink</a>.</p>

MetaData should not carry over to next document:

    >>> print md.convert("No [[MetaData]] here.")
    <p>No <a class="wikilink" href="/MetaData/">MetaData</a> here.</p>

Define a custom URL builder:

    >>> def my_url_builder(label, base, end):
    ...     return '/bar/'
    >>> md = markdown.Markdown(extensions=['wikilinks'], 
    ...         extension_configs={'wikilinks' : [('build_url', my_url_builder)]})
    >>> print md.convert('[[foo]]')
    <p><a class="wikilink" href="/bar/">foo</a></p>

From the command line:

    python markdown.py -x wikilinks(base_url=http://example.com/,end_url=.html,html_class=foo) src.txt

By [Waylan Limberg](http://achinghead.com/).

License: [BSD](http://www.opensource.org/licenses/bsd-license.php) 

Dependencies:
* [Python 2.3+](http://python.org)
* [Markdown 2.0+](http://packages.python.org/Markdown/)
'''

from __future__ import absolute_import
from __future__ import unicode_literals
from . import Extension
from ..inlinepatterns import Pattern
from ..util import etree
import re

def build_url(label, base, end):
    """ Build a url from the label, a base, and an end. """
    clean_label = re.sub(r'([ ]+_)|(_[ ]+)|([ ]+)', '_', label)
    return '%s%s%s'% (base, clean_label, end)


class WikiLinkExtension(Extension):
    def __init__(self, configs):
        # set extension defaults
        self.config = {
                        'base_url' : ['/', 'String to append to beginning or URL.'],
                        'end_url' : ['/', 'String to append to end of URL.'],
                        'html_class' : ['wikilink', 'CSS hook. Leave blank for none.'],
                        'build_url' : [build_url, 'Callable formats URL from label.'],
        }
        
        # Override defaults with user settings
        for key, value in configs :
            self.setConfig(key, value)
        
    def extendMarkdown(self, md, md_globals):
        self.md = md
    
        # append to end of inline patterns
        WIKILINK_RE = r'\[\[([\w0-9_ -]+)\]\]'
        wikilinkPattern = WikiLinks(WIKILINK_RE, self.getConfigs())
        wikilinkPattern.md = md
        md.inlinePatterns.add('wikilink', wikilinkPattern, "<not_strong")


class WikiLinks(Pattern):
    def __init__(self, pattern, config):
        super(WikiLinks, self).__init__(pattern)
        self.config = config
  
    def handleMatch(self, m):
        if m.group(2).strip():
            base_url, end_url, html_class = self._getMeta()
            label = m.group(2).strip()
            url = self.config['build_url'](label, base_url, end_url)
            a = etree.Element('a')
            a.text = label 
            a.set('href', url)
            if html_class:
                a.set('class', html_class)
        else:
            a = ''
        return a

    def _getMeta(self):
        """ Return meta data or config data. """
        base_url = self.config['base_url']
        end_url = self.config['end_url']
        html_class = self.config['html_class']
        if hasattr(self.md, 'Meta'):
            if 'wiki_base_url' in self.md.Meta:
                base_url = self.md.Meta['wiki_base_url'][0]
            if 'wiki_end_url' in self.md.Meta:
                end_url = self.md.Meta['wiki_end_url'][0]
            if 'wiki_html_class' in self.md.Meta:
                html_class = self.md.Meta['wiki_html_class'][0]
        return base_url, end_url, html_class
    

def makeExtension(configs=None) :
    return WikiLinkExtension(configs=configs)
