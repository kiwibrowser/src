# The Official Khronos WebGL Repository

This is the official home of the Khronos
WebGL repository for the WebGL specifications
and the WebGL conformance test suite.

Before adding a new test or editing an existing test
[please read these guidelines](sdk/tests/test-guidelines.md).

You can find live versions of the specifications at
https://www.khronos.org/webgl/

The newest work in progress WebGL conformance test suite
for the next version of the spec can be accessed at.
https://www.khronos.org/registry/webgl/sdk/tests/webgl-conformance-tests.html

Official live versions of the conformance test suite can be found at
https://www.khronos.org/registry/webgl/conformance-suites/

The WebGL Wiki can be found here
https://www.khronos.org/webgl/wiki/

## Cloning this repository

When cloning this repository please pass the --recursive flag to git:

    git clone --recursive [URL]

This will properly install the [WebGLDeveloperTools](https://github.com/KhronosGroup/WebGLDeveloperTools)
repository as a git submodule under sdk/devtools/.

The last edited date in several specifications is automatically updated
via a smudge filter. To benefit from this you must issue the following
commands in the root of your clone.

On Unix (Linux, Mac OS X, etc.) platforms and Windows using Git for Windows'
Git Bash or Cygwin's bash terminal:

```bash
./install-gitconfig.sh
rm specs/latest/*/index.html
git checkout !$
```

On Windows with the Command Prompt (requires `git.exe` in a directory
on your %PATH%):

```cmd
install-gitconfig.bat
del specs/latest/1.0/index.html specs/latest/2.0/index.html 
git checkout specs/latest/1.0/index.html specs/latest/2.0/index.html 
```

The first command adds an [include] of the repo's `.gitconfig` to the local git config file`.git/config` in your clone of the repo. `.gitconfig` contains the config of the "dater" filter. The remaining commands force a new checkout of the index.html files to smudge them with the date. These two are unnecessary if you plan to edit these files. All are unecessary if you do not care about having the dates shown.
