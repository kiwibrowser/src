# Chrome Installer for Mac -- the repository!

This Mac app installs Chrome on the machine it's run on. It's meant to be the
Mac app the user first downloads when they click 'Download Chrome' on a Mac
machine. After the user runs the app, Chrome would launch directly after
installation completes successfully.

## The 10,000 Foot View

The installer breaks down its task into the following steps:

  1.  __OmahaCommunication__: Ask Omaha for a URL of the most recent compatible
      copy of the Google Chrome disk image in Google's servers.
  2.  __Downloader__: Download the disk image, via URL from OmahaCommunication.
  3.  __Unpacker__: Mount the disk image and extract the Chrome app bundle.
  4.  __AuthorizedInstall__: With privilege escalation if able, move the Chrome
      app into the Applications folder, set permissions, and hand ownership of
      the Chrome app to `root`.
  5.  Launch Chrome & close.

Each of the above modules are designed to carry one action before returning via
delegate method. All of these main steps occur on a primary working thread
(non-UI), with the exception of `AuthorizedInstall`, which makes use of an
authorized-if-able subprocess. If the user does not provide permission to
escalate privileges, `AuthorizedInstall` still does its job, but opts for the
User's Applications folder instead of the system Applications folder.

The OmahaXML* classes and SystemInfo class are simply classes to help
OmahaCommunication do its work.

The app's UI is made of a single window that has a determinate progress bar
during download, which turns into an indeterminate one during installation. The
options to set Chrome as a default browser and opt in for user metrics are made
available to the user before they launch Chrome. Once Chrome is ready to launch,
a Launch button will now be pressable -- this button will trigger Chrome to open
and the installer app to close its remaining window. In the background, the app
will take care of any tear-down tasks before exiting naturally.

We initialize the AuthorizedInstall class early in the life of the installer so
the user can immediately choose to authorize the installer for root installation.
The script consumes the authorization token (which expires in five minutes by
default) immediately, then waits until the installer has progressed to step 4
(above).

## The Class Breakdown

| Class                      | Role                                               |
|----------------------------|----------------------------------------------------|
| AppDelegate                | Controls the flow of the program                   |
| AuthorizedInstall          | Attempts authorization to add Chrome to the Applications folder and adjust permissions as root |
| Downloader                 | Downloads GoogleChrome.dmg from Omaha servers      |
| InstallerWindowController  | Controls the user interface                        |
| OmahaCommunication         | Talks with Omaha Servers to get URL of disk image  |
| OmahaXMLParser             | Extracts URLs from Omaha's XML response            |
| OmahaXMLRequest            | Creates an XML request to send to Omaha            |
| SystemInfo                 | Provides system information to help craft the XML request for Omaha |
| Unpacker                   | Mounts the disk image and controls the temporary directory that abstracts the installer's file-manipulating activity from the user |

## The Future

Here lies a list of hopes and dreams:

* Implement resumable downloads.
* Add in adequate testing using a local test server.
* Include basic error recovery attempts -- say, if during a download a URL
  does not provide a valid disk image, the installer can try re-downloading
  the disk image from another URL.
* Manage potential conflicts, in the case that Google Chrome already exists in
  the Applications folder when the installer is run.
* Trash the installer application after it has completed running.

## Diagram Appendix

### Task Flow

```

 Exposed Errors                Main Logic

                 +--------------------------------------+
                 |                                      |
                 |  Request authentication from users   |
                 |                                      |
                 +------------------+-------------------+
                                    |
                 +------------------v-------------------+
                 |                                      |
        +--------+ Ask Omaha for appropriate Chrome app |
        v        |                                      |
                 +------------------+-------------------+
 Network Error                      |
                 +------------------v-------------------+
        ^        |                                      |
        +--------+    Parse the response from Omaha     |
                 |                                      |
                 +------------------+-------------------+
                                    |
                 +------------------v-------------------+
                 |                                      |
Download Error <-+    Download the Chrome disk image    |
                 |                                      |
                 +------------------+-------------------+
                                    |
                 +------------------v-------------------+
                 |                                      |
        +--------+         Mount the disk image         |
        v        |                                      |
                 +------------------+-------------------+
 Install Error                      |
                 +------------------v-------------------+
        ^        |                                      |
        +--------+    Install & Configure Chrome app    |
                 |                                      |
                 +------------------+-------------------+
                                    |
                 +------------------v-------------------+
                 |                                      |
                 |          Unmount disk image          +-> If unmount fails, system
                 |                                      |   restart can resolve this.
                 +------------------+-------------------+
                                    |
                 +------------------v-------------------+
                 |                                      |
  Launch Error <-+            Launch Chrome             |
                 |                                      |
                 +--------------------------------------+

```

### Class Heirarchy

```

                                 Users

                                ^     +
                                |     |
                                |     |
+-------------------------------+-----v--------------------------------+
|                                                                      |
|                       InstallerWindowController                      |
|                                                                      |
+-------+------^------------+---^--------+----^---------+---------^----+
        |      |            |   |        |    |         |         |
        |      |            |   |        |    |         |         |
        |      |            |   |        |    |         |         |
+-------v------+------------v---+--------v----+---------v---------+----+
|                                                                      |
|                              AppDelegate                             |
|                                                                      |
+-------+------^------------+---^--------+----^---------+---------^----+
        |      |            |   |        |    |         |         |
        |      |            |   |        |    |         |         |
        |      |            |   |        |    |         |         |
+-------v------+-----+ +----v---+---+ +--v----+--+ +----v---------+----+
|                    | |            | |          | |                   |
| OmahaCommunication | | Downloader | | Unpacker | | AuthorizedInstall |
|                    | |            | |          | |                   |
+-------^------^-----+ +------------+ +----------+ +---------^---------+
        |      |                                             |
        |      +------------+                                |
        |                   |                                |
+-------+---------+ +-------+--------+              +--------+--------+
|                 | |                |              |                 |
| OmahaXMLRequest | | OmahaXMLParser |              | copy_to_disk.sh |
|                 | |                |              |                 |
+-------^---------+ +----------------+              +-----------------+
        |
        |
        |
  +-----+------+
  |            |
  | SystemInfo |
  |            |
  +------------+

```