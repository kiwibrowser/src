.. _publications_and_presentations:

.. include:: /migration/deprecation.inc

##############################
Publications and Presentations
##############################

This page lists Native Client and Portable Native Client talks, demos,
and publications from various conferences and academic symposiums.

Recent talks and demos
----------------------

.. list-table::
   :header-rows: 1

   * - Date
     - Event
     - Talk
   * - 2013/05/16
     - `Google I/O 2013 <https://developers.google.com/events/io/2013>`_
     - `Introduction to Portable Native Client
       <https://www.youtube.com/watch?v=5RFjOec-TI0>`_
   * - 2012/12/11
     - `Google Developers Live <https://developers.google.com/live/>`_
     - `Native Client Acceleration Modules
       <https://developers.google.com/live/shows/7320022-5002/>`_ Learn
       how to use Native Client to deliver performance where it counts
       (`source code <https://github.com/johnmccutchan/NaClAMBase/>`__)
   * - 2012/07/26
     - `Casual Connect Seattle 2012
       <http://casualconnect.org/seattle/content.html>`_
     - `Take your C++ To the Web with Native Client
       <https://www.youtube.com/watch?v=RV7SMC3IJNo>`_ Includes an
       overview of Native Client technology, porting legacy applications
       from the Windows desktop, and current third-party use of Native
       Client in middleware and games
   * - 2012/06/28
     - `Google I/O 2012 <https://developers.google.com/events/io/2012>`_
     - `Native Client Live
       <https://www.youtube.com/watch?v=1zvhs5FR0X8>`_ Demonstrates how
       to port an existing C application to Native Client using a Visual
       Studio add-in that lets developers debug their code as a trusted
       Chrome plugin
   * - 2012/06/28
     - `Google I/O 2012 <https://developers.google.com/events/io/2012>`_
     - `The Life of a Native Client Instruction
       <https://www.youtube.com/watch?v=KOsJIhmeXoc>`_ (`slides
       <https://nacl-instruction-io12.appspot.com>`__)
   * - 2012/03/05
     - `GDC 2012 <http://www.gdcvault.com/free/gdc-12>`_
     - `Get Your Port On <https://www.youtube.com/watch?v=R281PhQufHo>`_
       Porting Your C++ Game to Native Client
   * - 2011/08/12
     - ---
     - `Native Client Update and Showcase
       <https://www.youtube.com/watch?v=g3aBfkFbPWk>`_
   * - 2010/11/04
     - `2010 LLVM Developers' Meeting
       <http://llvm.org/devmtg/2010-11/>`_
     - `Portable Native Client
       <http://llvm.org/devmtg/2010-11/videos/Sehr_NativeClient-desktop.mp4>`_

Publications
------------

.. list-table::
   :header-rows: 1

   * - Title
     - Authors
     - Published in
   * - `Language-Independent Sandboxing of Just-In-Time Compilation and
       Self-Modifying Code
       <http://research.google.com/pubs/archive/37204.pdf>`_
     - Jason Ansel, Petr Marchenko, `Úlfar Erlingsson
       <http://research.google.com/pubs/ulfar.html>`_, Elijah Taylor,
       `Brad Chen <http://research.google.com/pubs/author37895.html>`_,
       Derek Schuff, David Sehr, `Cliff L. Biffle
       <http://research.google.com/pubs/author38542.html>`_, Bennet
       S. Yee
     - ACM SIGPLAN Conference on Programming Language Design and
       Implementation (PLDI), 2011
   * - `Adapting Software Fault Isolation to Contemporary CPU
       Architectures <http://research.google.com/pubs/pub35649.html>`_
     - David Sehr, Robert Muth, `Cliff L. Biffle
       <http://research.google.com/pubs/author38542.html>`_, Victor
       Khimenko, Egor Pasko, Bennet S. Yee, Karl Schimpf, `Brad Chen
       <http://research.google.com/pubs/author37895.html>`_
     - 19th USENIX Security Symposium, 2010, pp. 1-11
   * - `Native Client: A Sandbox for Portable, Untrusted x86 Native Code
       <http://research.google.com/pubs/pub34913.html>`_
     - Bennet S. Yee, David Sehr, Greg Dardyk, `Brad Chen
       <http://research.google.com/pubs/author37895.html>`_, Robert
       Muth, Tavis Ormandy, Shiki Okasaka, Neha Narula, Nicholas
       Fullagar
     - IEEE Symposium on Security and Privacy (Oakland '09), 2009
   * - `PNaCl: Portable Native Client Executables
       <http://nativeclient.googlecode.com/svn/data/site/pnacl.pdf>`_
     - Alan Donovan, Robert Muth, Brad Chen, David Sehr
     - February 2010

External Publications
---------------------

In these articles outside developers and Google engineers describe their
experience porting libraries and applications to Native Client and
Portable Native Client. They share their insights and provide some tips
and instructions for how to port your own code.

Porting Nebula3 to Portable Native Client
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Andre Weissflog ported his Nebula3 engine to Portable Native Client (see
`his demos <http://www.flohofwoe.net/demos.html>`__). He discusses
`build systems
<http://flohofwoe.blogspot.de/2013/08/emscripten-and-pnacl-build-systems.html>`__
and `app entry
<http://flohofwoe.blogspot.de/2013/09/emscripten-and-pnacl-app-entry-in-pnacl.html>`__.

Porting Go Home Dinosaurs
^^^^^^^^^^^^^^^^^^^^^^^^^

`Fire Hose Games <http://firehosegames.com>`_ developed a new webgame
`Go Home Dinosaurs
<https://chrome.google.com/webstore/detail/icefnknicgejiphafapflechfoeelbeo>`_.
It features tower defense, dinosaurs, and good old fashioned BBQ. This
article explains their experiences developing for Native Client
including useful lessons learned to help you get started.

`Read more <http://www.gamasutra.com/view/feature/175210/the_ins_and_outs_of_native_client.php>`__

Porting Zombie Track Meat
^^^^^^^^^^^^^^^^^^^^^^^^^

`Fuzzycube Software <http://www.fuzzycubesoftware.com>`_, traditionally
a mobile game development studio, talks about their adventure into the
web, porting the undead decathlon `Zombie Track Meat
<https://chrome.google.com/webstore/detail/jmfhnfnjfdoplkgbkmibfkdjolnemfdk/reviews>`_
from Objective C to Native Client.

`Read more <http://fuzzycube.blogspot.com/2012/04/zombie-track-meat-post-mortem.html>`__

Porting AirMech
^^^^^^^^^^^^^^^

`Carbon Games <http://carbongames.com/>`_ chose Native Client as a
solution for cross-platform delivery of their PC game `AirMech
<https://chrome.google.com/webstore/detail/hdahlabpinmfcemhcbcfoijcpoalfgdn>`_
to Linux and Macintosh in lieu of native ports. They describe the
porting process on their blog.

`Read more <http://carbongames.com/2012/01/Native-Client>`__
