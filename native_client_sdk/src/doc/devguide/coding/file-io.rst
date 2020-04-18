.. _devguide-coding-fileio:

.. include:: /migration/deprecation.inc

########
File I/O
########

.. contents::
  :local:
  :backlinks: none
  :depth: 2

Introduction
============

This section describes how to use the `FileIO API
</native-client/pepper_stable/cpp/classpp_1_1_file_i_o>`_ to read and write
files using a local secure data store.

You might use the File IO API with the URL Loading APIs to create an overall
data download and caching solution for your NaCl applications. For example:

#. Use the File IO APIs to check the local disk to see if a file exists that
   your program needs.
#. If the file exists locally, load it into memory using the File IO API. If
   the file doesn't exist locally, use the URL Loading API to retrieve the
   file from the server.
#. Use the File IO API to write the file to disk.
#. Load the file into memory using the File IO API when needed by your
   application.

The example discussed in this section is included in the SDK in the directory
``examples/api/file_io``.

Reference information
=====================

For reference information related to FileIO, see the following documentation:

* `file_io.h </native-client/pepper_stable/cpp/file__io_8h>`_ - API to create a
  FileIO object
* `file_ref.h </native-client/pepper_stable/cpp/file__ref_8h>`_ - API to create
  a file reference or "weak pointer" to a file in a file system
* `file_system.h </native-client/pepper_stable/cpp/file__system_8h>`_ - API to
  create a file system associated with a file

Local file I/O
==============

Chrome provides an obfuscated, restricted area on disk to which a web app can
safely `read and write files
<https://developers.google.com/chrome/whitepapers/storage#persistent>`_. The
Pepper FileIO, FileRef, and FileSystem APIs (collectively called the File IO
APIs) allow you to access this sandboxed local disk so you can read and write
files and manage caching yourself. The data is persistent between launches of
Chrome, and is not removed unless your application deletes it or the user
manually deletes it. There is no limit to the amount of local data you can
use, other than the actual space available on the local drive.

.. _quota_management:
.. _enabling_file_access:

Enabling local file I/O
-----------------------

The easiest way to enable the writing of persistent local data is to include
the `unlimitedStorage permission
</extensions/declare_permissions#unlimitedStorage>`_ in your Chrome Web Store
manifest file. With this permission you can use the Pepper FileIO API without
the need to request disk space at run time. When the user installs the app
Chrome displays a message announcing that the app writes to the local disk.

If you do not use the ``unlimitedStorage`` permission you must include
JavaScript code that calls the `HTML5 Quota Management API
<http://updates.html5rocks.com/2011/11/Quota-Management-API-Fast-Facts>`_ to
explicitly request local disk space before using the FileIO API. In this case
Chrome will prompt the user to accept a requestQuota call every time one is
made.

Testing local file I/O
----------------------

You should be aware that using the ``unlimitedStorage`` manifest permission
constrains the way you can test your app. Three of the four techniques
described in :doc:`Running Native Client Applications <../devcycle/running>`
read the Chrome Web Store manifest file and enable the ``unlimitedStorage``
permission when it appears, but the first technique (local server) does not.
If you want to test the file IO portion of your app with a simple local server,
you need to include JavaScript code that calls the HTML5 Quota Management API.
When you deliver your application you can replace this code with the
``unlimitedStorage`` manifest permission.

The ``file_io`` example
=======================

The Native Client SDK includes an example, ``file_io``, that demonstrates how
to read and write a local disk file. Since you will probably run the example
from a local server without a Chrome Web Store manifest file, the example's
index file uses JavaScript to perform the Quota Management setup as described
above. The example has these primary files:

* ``index.html`` - The HTML code that launches the Native Client module and
  displays the user interface.
* ``example.js`` - JavaScript code that requests quota (as described above). It
  also listens for user interaction with the user interface, and forwards the
  requests to the Native Client module.
* ``file_io.cc`` - The code that sets up and provides an entry point to the
  Native Client module.

The remainder of this section covers the code in the ``file_io.cc`` file for
reading and writing files.

File I/O overview
-----------------

Like many Pepper APIs, the File IO API includes a set of methods that execute
asynchronously and that invoke callback functions in your Native Client module.
Unlike most other examples, the ``file_io`` example also demonstrates how to
make Pepper calls synchronously on a worker thread.

It is illegal to make blocking calls to Pepper on the module's main thread.
This restriction is lifted when running on a worker thread---this is called
"calling Pepper off the main thread". This often simplifies the logic of your
code; multiple asynchronous Pepper functions can be called from one function on
your worker thread, so you can use the stack and standard control flow
structures normally.

The high-level flow for the ``file_io`` example is described below.  Note that
methods in the namespace ``pp`` are part of the Pepper C++ API.

Creating and writing a file
---------------------------

Following are the high-level steps involved in creating and writing to a
file:

#. ``pp::FileIO::Open`` is called with the ``PP_FILEOPEN_FLAG_CREATE`` flag to
   create a file.  Because the callback function is ``pp::BlockUntilComplete``,
   this thread is blocked until ``Open`` succeeds or fails.
#. ``pp::FileIO::Write`` is called to write the contents. Again, the thread is
   blocked until the call to ``Write`` completes. If there is more data to
   write, ``Write`` is called again.
#. When there is no more data to write, call ``pp::FileIO::Flush``.

Opening and reading a file
--------------------------

Following are the high-level steps involved in opening and reading a file:

#. ``pp::FileIO::Open`` is called to open the file. Because the callback
   function is ``pp::BlockUntilComplete``, this thread is blocked until Open
   succeeds or fails.
#. ``pp::FileIO::Query`` is called to query information about the file, such as
   its file size. The thread is blocked until ``Query`` completes.
#. ``pp::FileIO::Read`` is called to read the contents. The thread is blocked
   until ``Read`` completes. If there is more data to read, ``Read`` is called
   again.

Deleting a file
---------------

Deleting a file is straightforward: call ``pp::FileRef::Delete``. The thread is
blocked until ``Delete`` completes.

Making a directory
------------------

Making a directory is also straightforward: call ``pp::File::MakeDirectory``.
The thread is blocked until ``MakeDirectory`` completes.

Listing the contents of a directory
-----------------------------------

Following are the high-level steps involved in listing a directory:

#. ``pp::FileRef::ReadDirectoryEntries`` is called, and given a directory entry
   to list. A callback is given as well; many of the other functions use
   ``pp::BlockUntilComplete``, but ``ReadDirectoryEntries`` returns results in
   its callback, so it must be specified.
#. When the call to ``ReadDirectoryEntries`` completes, it calls
   ``ListCallback`` which packages up the results into a string message, and
   sends it to JavaScript.

``file_io`` deep dive
=====================

The ``file_io`` example displays a user interface with a couple of fields and
several buttons. Following is a screenshot of the ``file_io`` example:

.. image:: /images/fileioexample.png

Each radio button is a file operation you can perform, with some reasonable
default values for filenames. Try typing a message in the large input box and
clicking ``Save``, then switching to the ``Load File`` operation, and
clicking ``Load``.

Let's take a look at what is going on under the hood.

Opening a file system and preparing for file I/O
------------------------------------------------

``pp::Instance::Init`` is called when an instance of a module is created. In
this example, ``Init`` starts a new thread (via the ``pp::SimpleThread``
class), and tells it to open the filesystem:

.. naclcode::

  virtual bool Init(uint32_t /*argc*/,
                    const char * /*argn*/ [],
                    const char * /*argv*/ []) {
    file_thread_.Start();
    // Open the file system on the file_thread_. Since this is the first
    // operation we perform there, and because we do everything on the
    // file_thread_ synchronously, this ensures that the FileSystem is open
    // before any FileIO operations execute.
    file_thread_.message_loop().PostWork(
        callback_factory_.NewCallback(&FileIoInstance::OpenFileSystem));
    return true;
  }

When the file thread starts running, it will call ``OpenFileSystem``. This
calls ``pp::FileSystem::Open`` and blocks the file thread until the function
returns.

.. Note::
  :class: note

  Note that the call to ``pp::FileSystem::Open`` uses
  ``pp::BlockUntilComplete`` as its callback. This is only possible because we
  are running off the main thread; if you try to make a blocking call from the
  main thread, the function will return the error
  ``PP_ERROR_BLOCKS_MAIN_THREAD``.

.. naclcode::

  void OpenFileSystem(int32_t /*result*/) {
    int32_t rv = file_system_.Open(1024 * 1024, pp::BlockUntilComplete());
    if (rv == PP_OK) {
      file_system_ready_ = true;
      // Notify the user interface that we're ready
      PostMessage("READY|");
    } else {
      ShowErrorMessage("Failed to open file system", rv);
    }
  }

Handling messages from JavaScript
---------------------------------

When you click the ``Save`` button, JavaScript posts a message to the NaCl
module with the file operation to perform sent as a string (See :doc:`Messaging
System <message-system>` for more details on message passing). The string is
parsed by ``HandleMessage``, and new work is added to the file thread:

.. naclcode::

  virtual void HandleMessage(const pp::Var& var_message) {
    if (!var_message.is_string())
      return;

    // Parse message into: instruction file_name_length file_name [file_text]
    std::string message = var_message.AsString();
    std::string instruction;
    std::string file_name;
    std::stringstream reader(message);
    int file_name_length;

    reader >> instruction >> file_name_length;
    file_name.resize(file_name_length);
    reader.ignore(1);  // Eat the delimiter
    reader.read(&file_name[0], file_name_length);

    ...

    // Dispatch the instruction
    if (instruction == kLoadPrefix) {
      file_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&FileIoInstance::Load, file_name));
    } else if (instruction == kSavePrefix) {
      ...
    }
  }

Saving a file
-------------

``FileIoInstance::Save`` is called when the ``Save`` button is pressed. First,
it checks to see that the FileSystem has been successfully opened:

.. naclcode::

  if (!file_system_ready_) {
    ShowErrorMessage("File system is not open", PP_ERROR_FAILED);
    return;
  }

It then creates a ``pp::FileRef`` resource with the name of the file. A
``FileRef`` resource is a weak reference to a file in the FileSystem; that is,
a file can still be deleted even if there are outstanding ``FileRef``
resources.

.. naclcode::

  pp::FileRef ref(file_system_, file_name.c_str());

Next, a ``pp::FileIO`` resource is created and opened. The call to
``pp::FileIO::Open`` passes ``PP_FILEOPEFLAG_WRITE`` to open the file for
writing, ``PP_FILEOPENFLAG_CREATE`` to create a new file if it doesn't already
exist and ``PP_FILEOPENFLAG_TRUNCATE`` to clear the file of any previous
content:

.. naclcode::

  pp::FileIO file(this);

  int32_t open_result =
      file.Open(ref,
                PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_CREATE |
                    PP_FILEOPENFLAG_TRUNCATE,
                pp::BlockUntilComplete());
  if (open_result != PP_OK) {
    ShowErrorMessage("File open for write failed", open_result);
    return;
  }

Now that the file is opened, it is written to in chunks. In an asynchronous
model, this would require writing a separate function, storing the current
state on the free store and a chain of callbacks. Because this function is
called off the main thread, ``pp::FileIO::Write`` can be called synchronously
and a conventional do/while loop can be used:

.. naclcode::

  int64_t offset = 0;
  int32_t bytes_written = 0;
  do {
    bytes_written = file.Write(offset,
                               file_contents.data() + offset,
                               file_contents.length(),
                               pp::BlockUntilComplete());
    if (bytes_written > 0) {
      offset += bytes_written;
    } else {
      ShowErrorMessage("File write failed", bytes_written);
      return;
    }
  } while (bytes_written < static_cast<int64_t>(file_contents.length()));

Finally, the file is flushed to push all changes to disk:

.. naclcode::

  int32_t flush_result = file.Flush(pp::BlockUntilComplete());
  if (flush_result != PP_OK) {
    ShowErrorMessage("File fail to flush", flush_result);
    return;
  }

Loading a file
--------------

``FileIoInstance::Load`` is called when the ``Load`` button is pressed. Like
the ``Save`` function, ``Load`` first checks to see if the FileSystem has been
successfully opened, and creates a new ``FileRef``:

.. naclcode::

  if (!file_system_ready_) {
    ShowErrorMessage("File system is not open", PP_ERROR_FAILED);
    return;
  }
  pp::FileRef ref(file_system_, file_name.c_str());

Next, ``Load`` creates and opens a new ``FileIO`` resource, passing
``PP_FILEOPENFLAG_READ`` to open the file for reading. The result is compared
to ``PP_ERROR_FILENOTFOUND`` to give a better error message when the file
doesn't exist:

.. naclcode::

  int32_t open_result =
      file.Open(ref, PP_FILEOPENFLAG_READ, pp::BlockUntilComplete());
  if (open_result == PP_ERROR_FILENOTFOUND) {
    ShowErrorMessage("File not found", open_result);
    return;
  } else if (open_result != PP_OK) {
    ShowErrorMessage("File open for read failed", open_result);
    return;
  }

Then ``Load`` calls ``pp::FileIO::Query`` to get metadata about the file, such
as its size. This is used to allocate a ``std::vector`` buffer that holds the
data from the file in memory:

.. naclcode::

  int32_t query_result = file.Query(&info, pp::BlockUntilComplete());
  if (query_result != PP_OK) {
    ShowErrorMessage("File query failed", query_result);
    return;
  }

  ...

  std::vector<char> data(info.size);

Similar to ``Save``, a conventional while loop is used to read the file into
the newly allocated buffer:

.. naclcode::

  int64_t offset = 0;
  int32_t bytes_read = 0;
  int32_t bytes_to_read = info.size;
  while (bytes_to_read > 0) {
    bytes_read = file.Read(offset,
                           &data[offset],
                           data.size() - offset,
                           pp::BlockUntilComplete());
    if (bytes_read > 0) {
      offset += bytes_read;
      bytes_to_read -= bytes_read;
    } else if (bytes_read < 0) {
      // If bytes_read < PP_OK then it indicates the error code.
      ShowErrorMessage("File read failed", bytes_read);
      return;
    }
  }

Finally, the contents of the file are sent back to JavaScript, to be displayed
on the page. This example uses "``DISP|``" as a prefix command for display
information:

.. naclcode::

  std::string string_data(data.begin(), data.end());
  PostMessage("DISP|" + string_data);
  ShowStatusMessage("Load success");

Deleting a file
---------------

``FileIoInstance::Delete`` is called when the ``Delete`` button is pressed.
First, it checks whether the FileSystem has been opened, and creates a new
``FileRef``:

.. naclcode::

  if (!file_system_ready_) {
    ShowErrorMessage("File system is not open", PP_ERROR_FAILED);
    return;
  }
  pp::FileRef ref(file_system_, file_name.c_str());

Unlike ``Save`` and ``Load``, ``Delete`` is called on the ``FileRef`` resource,
not a ``FileIO`` resource. Note that the result is checked for
``PP_ERROR_FILENOTFOUND`` to give a better error message when trying to delete
a non-existent file:

.. naclcode::

  int32_t result = ref.Delete(pp::BlockUntilComplete());
  if (result == PP_ERROR_FILENOTFOUND) {
    ShowStatusMessage("File/Directory not found");
    return;
  } else if (result != PP_OK) {
    ShowErrorMessage("Deletion failed", result);
    return;
  }

Listing files in a directory
----------------------------

``FileIoInstance::List`` is called when the ``List Directory`` button is
pressed. Like all other operations, it checks whether the FileSystem has been
opened and creates a new ``FileRef``:

.. naclcode::

  if (!file_system_ready_) {
    ShowErrorMessage("File system is not open", PP_ERROR_FAILED);
    return;
  }

  pp::FileRef ref(file_system_, dir_name.c_str());

Unlike the other operations, it does not make a blocking call to
``pp::FileRef::ReadDirectoryEntries``. Since ``ReadDirectoryEntries`` returns
the resulting directory entries in its callback, a new callback object is
created pointing to ``FileIoInstance::ListCallback``.

The ``pp::CompletionCallbackFactory`` template class is used to instantiate a
new callback. Notice that the ``FileRef`` resource is passed as a parameter;
this will add a reference count to the callback object, to keep the ``FileRef``
resource from being destroyed when the function finishes.

.. naclcode::

  // Pass ref along to keep it alive.
  ref.ReadDirectoryEntries(callback_factory_.NewCallbackWithOutput(
      &FileIoInstance::ListCallback, ref));

``FileIoInstance::ListCallback`` then gets the results passed as a
``std::vector`` of ``pp::DirectoryEntry`` objects, and sends them to
JavaScript:

.. naclcode::

  void ListCallback(int32_t result,
                    const std::vector<pp::DirectoryEntry>& entries,
                    pp::FileRef /*unused_ref*/) {
    if (result != PP_OK) {
      ShowErrorMessage("List failed", result);
      return;
    }

    std::stringstream ss;
    ss << "LIST";
    for (size_t i = 0; i < entries.size(); ++i) {
      pp::Var name = entries[i].file_ref().GetName();
      if (name.is_string()) {
        ss << "|" << name.AsString();
      }
    }
    PostMessage(ss.str());
    ShowStatusMessage("List success");
  }

Making a new directory
----------------------

``FileIoInstance::MakeDir`` is called when the ``Make Directory`` button is
pressed. Like all other operations, it checks whether the FileSystem has been
opened and creates a new ``FileRef``:

.. naclcode::

  if (!file_system_ready_) {
    ShowErrorMessage("File system is not open", PP_ERROR_FAILED);
    return;
  }
  pp::FileRef ref(file_system_, dir_name.c_str());

Then the ``pp::FileRef::MakeDirectory`` function is called.

.. naclcode::

  int32_t result = ref.MakeDirectory(
      PP_MAKEDIRECTORYFLAG_NONE, pp::BlockUntilComplete());
  if (result != PP_OK) {
    ShowErrorMessage("Make directory failed", result);
    return;
  }
  ShowStatusMessage("Make directory success");
