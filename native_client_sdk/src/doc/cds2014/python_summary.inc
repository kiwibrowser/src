Learn how you can use client-side Python on your webpage.
Use the python |difflib| module to generate attractive online
textual diffs.
Develop inside Google Chrome, using a hot off the presses version
of our NaCl Dev Environment, running on a webpage,
powered by Portable Native Client.
The techniques in this codelab also apply to other interpreted languages
we've ported to PNaCl, including: Python, Lua, Ruby, Tcl, Bash, and Forth.

Requirements:
  * A Desktop / Laptop Windows, Mac, Linux, or ChromeOS browser
    |python_check|
  * A fast broadband connection (500MB download)
  * Can read and write Python


.. |difflib| raw:: html

  <a href="https://docs.python.org/2/library/difflib.html"
   target="_blank">difflib</a>


.. |python_check| raw:: html

  <br/><span id="python_compat" style="color: #cccc00"
      >Checking browser compatibility...</span><br/>
  <i id="python_compat2"></i>
  <script>
  var tag = document.getElementById('python_compat');
  var tag2 = document.getElementById('python_compat2');
  if (!('application/x-pnacl' in navigator.mimeTypes)) {
    tag.innerHTML =
        'This codelab does not appear to be supported by your browser.';
    tag.style.color = '#cc0000';
    tag2.innerHTML =
        'You do not appear to be running a browser such as ' +
        'Google Chrome which supports Portable Native Client ' +
        'or you have disabled Portable Native Client.';
  } else {
    tag.innerHTML = 'This codelab appears to be supported by your browser.';
    tag.style.color = '#00cc00';
  }
  </script>
