Setup
-----

For this codelab, you will need to point your web browser at
the Beta preview of our in-browser NaCl Dev Environment.

.. raw:: html

  <a href="https://naclports.storage.googleapis.com/builds/pepper_41/trunk-176-g9b9e342/publish/devenv/pnacl/app/bash.html"
  target="_blank">Click here to open the environment.</a>

While no installation is needed, the first load of the page will
require you to agree to allow the page unlimited storage
(Click "Ok" in the bar at the top of your browser window.)

The first run requires a large download. Please be patient.
Also, as PNaCl translates executables to native code on demand,
each program will also have a noticeable load delay on first run.
We're actively working to improve this.

NOTE: This environment will leave a substantial (800MB) payload in your
browser. See the cleanup section below to learn how to recover this space.

Once the download is complete, you should see a bash prompt (ignore
the gcc warning, our web C compiler is coming soon!)::

  WARNING: gcc not yet available for your platform (coming soon).
  bash.nmf-4.3$

.. include:: ../nacldev/web_tools_note.inc

.. include:: ../nacldev/navigating.inc

.. include:: ../nacldev/clipboard_web.inc

.. include:: ../nacldev/editing.inc

.. include:: ../nacldev/git.inc
