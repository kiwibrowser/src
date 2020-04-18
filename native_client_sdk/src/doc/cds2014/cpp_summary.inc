Learn the basics of using PPAPI to do 2D graphics from
a C++ program running in Native Client.
Modify our sample to turn fire into water.
Develop inside Google Chrome, using our NaCl Development Environment
Chrome App.
While this codelab currently targets conventional Native Client using our
GCC + GlibC based toolchain, the techniques involved are generally
applicable.

Requirements:
  * An x86 (sorry no arm) Desktop / Laptop
    Windows, Mac, Linux, or ChromeOS browser
    |cpp_check|
  * A fast broadband connection (500MB download)
  * Can read and write C++

.. |cpp_check| raw:: html

  <br/><span id="cpp_compat" style="color: #cccc00"
      >Checking browser compatibility...</span><br/>
  <i id="cpp_compat2"></i>
  <script>
  var tag = document.getElementById('cpp_compat');
  var tag2 = document.getElementById('cpp_compat2');
  if (!('application/x-nacl' in navigator.mimeTypes)) {
    tag.innerHTML =
        'This codelab does not appear to be supported by your browser.';
    tag.style.color = '#cc0000';
    tag2.innerHTML =
        'You do not appear to be running a browser such as ' +
        'Google Chrome which supports Native Client ' +
        'or you have disabled Native Client.';
  } else if (navigator.userAgent.search(' arm') >= 0) {
    tag.innerHTML =
        'This codelab does not appear to be supported by your browser.';
    tag.style.color = '#cc0000';
    tag2.innerHTML =
        'You appear to be running on an ARM based CPU. ' +
        'While Native Client does support ARM, ' +
        "our developer environment's GCC + GlibC based toolchain " +
        'does not, at this time. Sorry.';
  } else {
    tag.innerHTML = 'This codelab appears to be supported by your browser.';
    tag.style.color = '#00cc00';
  }
  </script>
