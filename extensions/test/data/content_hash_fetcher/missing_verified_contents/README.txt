The files in this directory came from downloading a test extension from the
webstore* that had properly signed verified_contents.json file, taking the
regular extension files (manifest and content script) from the .crx and putting
them into a source.zip file, and pulling out the
_metadata/verified_contents.json file to be standalone in this directory.

invalid_verified_contents.json is a file copied from verified_contents.json and
has one char modified in it so that it isn't a valid verified_contents anymore.

* https://chrome.google.com/webstore/detail/jmllhlobpjcnnomjlipadejplhmheiif
