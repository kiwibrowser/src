This directory contains semi-automated tests of Chrome with
NVDA (NonVisual Desktop Access), a popular open-source screen reader for
visually impaired users on Windows. It works by launching Chrome in a
subprocess, then launching NVDA in a special environment that simulates
speech rather than actually speaking, and ignores all events coming from
processes other than a specific Chrome process ID. Each test automates
Chrome with a series of actions and asserts that NVDA gives the expected
feedback in response.

Instructions for running these tests:

1. Install Python 2.7, 32-bit: http://www.python.org/

   Note - the version of Python installed by Chrome's depot_tools will not
   work, it's 64-bit.

2. Download pywinauto here:
     https://code.google.com/p/pywinauto/downloads/list

   Unzip it, then install it by running this from a cmd shell in that directory:
     python setup.py install

   If you get an error, make sure you're using the 32-bit version of Python.

3. Install the latest NVDA "next" snapshot from:
   http://community.nvda-project.org/wiki/Snapshots

   In the installer, choose "Create Portable copy" rather than "Install...".
   From the Browse dialog, create an new folder called nvdaPortable inside
   this folder.

   Note: after NVDA 2014.3 stable is released, just use the stable version
   instead, from http://www.nvaccess.org/download/
   - if you do this, you need to run NVDA, then from the NVDA menu, choose
   Tools > Create Portable Copy.
   From the Browse dialog, create an new folder called nvdaPortable inside
   this folder.
   You should now have something like this:
     d:\src\nvda_chrome_tests\nvdaPortable\nvda.exe
   You can now exit NVDA.

4. Install Chrome Canary. The binary is typically installed in:
   c:\Users\USERNAME\AppData\Local\Google\Chrome SxS\Application\chrome.exe
   ...if not, edit nvda_chrome_tests.py to point to it.

5. Clone the nvda-proctest environment into this directory:
   git clone https://bitbucket.org/nvaccess/nvda-proctest.git

6. Run the tests:

   First make sure NVDA is not already running.

   Open a cmd console, change to the nvda_chrome_tests directory, and run:
   python nvda_chrome_tests.py

   If you get an error, open the Windows task manager and make sure NVDA
   isn't running, kill it if necessary.

