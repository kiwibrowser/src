Cleanup
-------

The Web-based version of the Chrome Dev Environment installs
package data (>800MB) into the HTML5 Filesystem storage associated
with naclports.storage.googleapis.com .
To recover this storage (i.e. uninstall),
enter **chrome://settings/cookies** in the Chrome URL bar.
In the cookies UI, search for "naclports".
Then click the "x" next to the naclports entry.
