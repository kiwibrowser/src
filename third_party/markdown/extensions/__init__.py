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
Extensions
-----------------------------------------------------------------------------
"""

from __future__ import unicode_literals

class Extension(object):
    """ Base class for extensions to subclass. """
    def __init__(self, configs = {}):
        """Create an instance of an Extention.

        Keyword arguments:

        * configs: A dict of configuration setting used by an Extension.
        """
        self.config = configs

    def getConfig(self, key, default=''):
        """ Return a setting for the given key or an empty string. """
        if key in self.config:
            return self.config[key][0]
        else:
            return default

    def getConfigs(self):
        """ Return all configs settings as a dict. """
        return dict([(key, self.getConfig(key)) for key in self.config.keys()])

    def getConfigInfo(self):
        """ Return all config descriptions as a list of tuples. """
        return [(key, self.config[key][1]) for key in self.config.keys()]

    def setConfig(self, key, value):
        """ Set a config setting for `key` with the given `value`. """
        self.config[key][0] = value

    def extendMarkdown(self, md, md_globals):
        """
        Add the various proccesors and patterns to the Markdown Instance.

        This method must be overriden by every extension.

        Keyword arguments:

        * md: The Markdown instance.

        * md_globals: Global variables in the markdown module namespace.

        """
        raise NotImplementedError('Extension "%s.%s" must define an "extendMarkdown"' \
            'method.' % (self.__class__.__module__, self.__class__.__name__))

