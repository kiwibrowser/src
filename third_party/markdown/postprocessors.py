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


"""
POST-PROCESSORS
=============================================================================

Markdown also allows post-processors, which are similar to preprocessors in
that they need to implement a "run" method. However, they are run after core
processing.

"""

from __future__ import absolute_import
from __future__ import unicode_literals
from . import util
from . import odict
import re


def build_postprocessors(md_instance, **kwargs):
    """ Build the default postprocessors for Markdown. """
    postprocessors = odict.OrderedDict()
    postprocessors["raw_html"] = RawHtmlPostprocessor(md_instance)
    postprocessors["amp_substitute"] = AndSubstitutePostprocessor()
    postprocessors["unescape"] = UnescapePostprocessor()
    return postprocessors


class Postprocessor(util.Processor):
    """
    Postprocessors are run after the ElementTree it converted back into text.

    Each Postprocessor implements a "run" method that takes a pointer to a
    text string, modifies it as necessary and returns a text string.

    Postprocessors must extend markdown.Postprocessor.

    """

    def run(self, text):
        """
        Subclasses of Postprocessor should implement a `run` method, which
        takes the html document as a single text string and returns a
        (possibly modified) string.

        """
        pass


class RawHtmlPostprocessor(Postprocessor):
    """ Restore raw html to the document. """

    def run(self, text):
        """ Iterate over html stash and restore "safe" html. """
        for i in range(self.markdown.htmlStash.html_counter):
            html, safe  = self.markdown.htmlStash.rawHtmlBlocks[i]
            if self.markdown.safeMode and not safe:
                if str(self.markdown.safeMode).lower() == 'escape':
                    html = self.escape(html)
                elif str(self.markdown.safeMode).lower() == 'remove':
                    html = ''
                else:
                    html = self.markdown.html_replacement_text
            if self.isblocklevel(html) and (safe or not self.markdown.safeMode):
                text = text.replace("<p>%s</p>" % 
                            (self.markdown.htmlStash.get_placeholder(i)),
                            html + "\n")
            text =  text.replace(self.markdown.htmlStash.get_placeholder(i), 
                                 html)
        return text

    def escape(self, html):
        """ Basic html escaping """
        html = html.replace('&', '&amp;')
        html = html.replace('<', '&lt;')
        html = html.replace('>', '&gt;')
        return html.replace('"', '&quot;')

    def isblocklevel(self, html):
        m = re.match(r'^\<\/?([^ >]+)', html)
        if m:
            if m.group(1)[0] in ('!', '?', '@', '%'):
                # Comment, php etc...
                return True
            return util.isBlockLevel(m.group(1))
        return False


class AndSubstitutePostprocessor(Postprocessor):
    """ Restore valid entities """

    def run(self, text):
        text =  text.replace(util.AMP_SUBSTITUTE, "&")
        return text


class UnescapePostprocessor(Postprocessor):
    """ Restore escaped chars """

    RE = re.compile('%s(\d+)%s' % (util.STX, util.ETX))

    def unescape(self, m):
        return util.int2str(int(m.group(1)))

    def run(self, text):
        return self.RE.sub(self.unescape, text)
