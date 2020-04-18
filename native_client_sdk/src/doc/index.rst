.. include:: /migration/deprecation.inc

########################
Welcome to Native Client
########################

.. raw:: html

  <div id="home">
  <div class="pull-quote">To get the SDK and<br/>installation instructions<br/>
  <a href="/native-client/sdk/download.html">visit the SDK Download page</a>.
  </div>
  <div class="big-intro">

**Native Client** is a sandbox for running compiled C and C++ code in the
browser efficiently and securely, independent of the user's operating system.
**Portable Native Client** extends that technology with
architecture independence, letting developers compile their code once to run
in any website and on any architecture with ahead-of-time (AOT) translation.

In short, Native Client brings the **performance** and **low-level control**
of native code to modern web browsers, without sacrificing the **security** and
**portability** of the web. Watch the video below for an overview of
Native Client, including its goals, how it works, and how
Portable Native Client lets developers run native compiled code on the web.

.. Note::
  :class: note

  This site uses several examples of Native Client. For the best experience,
  consider downloading the `latest version of Chrome
  <https://www.google.com/chrome/>`_. When you come back, be sure to `check out
  our demos <https://gonativeclient.appspot.com/demo>`_.

.. raw:: html

  </div>

  <iframe class="video" width="600" height="337"
  src="//www.youtube.com/embed/MvKEomoiKBA?rel=0" frameborder="0"></iframe>
  <div class="big-intro">

Two Types of Modules
====================

Native Client comes in two flavors.

* **Portable Native Client (PNaCl)**: Pronounced 'pinnacle', PNaCl runs single,
  portable (**pexe**) executables and is available in most implementations of
  Chrome. A translator built into Chrome translates the pexe into native code
  for the client hardware. The entire module is translated before any code is
  executed rather than as the code is executed. PNaCl modules can be hosted from
  any web server.
* **Native Client (NaCl)**: Also called traditional or non-portable Native
  Client, NaCl runs architecture-dependent (**nexe**) modules, which are
  packaged into an application. At runtime, the browser decides which nexe to
  load based on the architecture of the client machine.
  Apps and Extensions installed via the `Chrome Web Store (CWS)
  <https://chrome.google.com/webstore/category/apps>`_ can use NaCl
  modules without additional prompting.
  NaCl apps can also be installed from chrome://extensions or
  the command-line during development,
  however, this is not a recommended distribution mechanism.

These flavors are described in more depth in :doc:`PNaCl and NaCl
<nacl-and-pnacl>`

.. raw:: html

  <div class="left-side">
  <div class="left-side-inner">
  <h2>Hello World</h2>
  <div class="big-intro">

To jump right in :doc:`take the tutorial <devguide/tutorial/tutorial-part1>`
that walks you through a basic web application for Portable Native Client
(PNaCl). This is a client-side application that uses HTML, JavaScript, and a
Native Client module written in C++.

.. raw:: html

  </div>
  </div>
  </div>
  <h2>A Little More Advanced</h2>
  <div class="big-intro">

If you've already got the basics down, you're probably trying to get a real
application ready for production. You're :doc:`building
<devguide/devcycle/building>`, :doc:`debugging <devguide/devcycle/debugging>`
or :doc:`ready to distribute <devguide/distributing>`.

.. raw:: html

  </div>

  <div class="left-side">
  <div class="left-side-inner">
  <h2>Nuts and Bolts</h2>
  <div class="big-intro">

You've been working on a Native Client module for a while now and you've run
into an arcane problem. You may need to refer to the :doc:`PNaCl Bitcode
Reference <reference/pnacl-bitcode-abi>` or the :doc:`Sandbox internals
<reference/sandbox_internals/index>`.

.. raw:: html

  </div>
  </div>
  </div>

I Want to Know Everything
=========================

So, you like to read now and try later. Start with our :doc:`Technical Overview
<overview>`

.. raw:: html

  <div class="big-intro" style="clear: both;">

Send us comments and feedback on the `native-client-discuss
<https://groups.google.com/forum/#!forum/native-client-discuss>`_ mailing list,
or ask questions using Stack Overflow's `google-nativeclient
<https://stackoverflow.com/questions/tagged/google-nativeclient>`_ tag.

.. raw:: html

  </div>
