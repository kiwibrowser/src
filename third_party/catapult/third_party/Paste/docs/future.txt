The Future Of Paste
===================

Introduction
------------

Paste has been under development for a while, and has lots of code in it.  Too much code!  The code is largely decoupled except for some core functions shared by many parts of the code.  Those core functions are largely replaced in `WebOb <http://pythonpaste.org/webob/>`_, and replaced with better implementations.

The future of these pieces is to split them into independent packages, and refactor the internal Paste dependencies to rely instead on WebOb.

Already Extracted
-----------------

paste.fixture:
  WebTest
  ScriptTest

paste.lint:
  wsgiref.validate

paste.exceptions and paste.evalexception:
  WebError

paste.util.template:
  Tempita


To Be Separated
---------------

paste.httpserver and paste.debug.watchthreads:
  Not sure what to call this.

paste.cascade and paste.errordocuments:
  Not sure; Ben has an implementation of errordocuments in ``pylons.middleware.StatusCodeRedirect``

paste.urlmap, paste.deploy.config.PrefixMiddleware:
  In... some routing thing?  Together with the previous package?

paste.proxy:
  WSGIProxy (needs lots of cleanup though)

paste.fileapp, paste.urlparser.StaticURLParser, paste.urlparser.PkgResourcesParser:
  In some new file-serving package.

paste.cgiapp, wphp.fcgi_app:
  Some proxyish app... maybe WSGIProxy?

paste.translogger, paste.debug.prints, paste.util.threadedprint, wsgifilter.proxyapp.DebugHeaders:
  Some... other place.  Something loggy.

paste.registry, paste.config:
  Not sure.  Alberto Valverde expressed interest in splitting out paste.registry.

paste.cgitb_catcher:
  Move to WebError?  Not sure if it matters.  For some reason people use this, though.


To Deprecate
------------

(In that, I won't extract these anywhere; I'm not going to do any big deletes anytime soon, though)

paste.recursive
  Better to do it manually (with webob.Request.get_response)

paste.wsgiwrappers, paste.request, paste.response, paste.wsgilib, paste.httpheaders, paste.httpexceptions:
  All the functionality is already in WebOb.

paste.urlparser.URLParser:
  Really this is tied to paste.webkit more than anything.

paste.auth.*:
  Well, these all need to be refactored, and replacements exist in AuthKit and repoze.who.  Some pieces might still have utility.

paste.debug.profile:
  I think repoze.profile supersedes this.

paste.debug.wdg_validator:
  It could get reimplemented with more options for validators, but I'm not really that interested at the moment.  The code is nothing fancy.

paste.transaction:
  More general in repoze.tm

paste.url:
  No one uses this


Undecided
---------

paste.debug.fsdiff:
  Maybe ScriptTest?

paste.session:
  It's an okay naive session system.  But maybe Beaker makes it irrelevant (Beaker does seem slightly more complex to setup).  But then, this can just live here indefinitely.

paste.gzipper:
  I'm a little uncomfortable with this in concept.  It's largely in WebOb right now, but not as middleware.

paste.reloader:
  Maybe this should be moved to paste.script (i.e., paster serve)

paste.debug.debugapp, paste.script.testapp:
  Alongside other debugging tools, I guess

paste.progress:
  Not sure this works.
