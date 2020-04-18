# How to Contribute to Vulkan Source Repositories

## The Repository

The source code for the Vulkan Loader component is sponsored
by Khronos and LunarG.

* [KhronosGroup/Vulkan-Loader](https://github.com/KhronosGroup/Vulkan-Loader)

### The Vulkan Ecosystem Needs Your Help

There are a couple of methods to identify areas of need:

* Examine the [issues list](https://github.com/KhronosGroup/Vulkan-Loader/issues)
  in this repository and look for issues that are of interest
* If you have your own work in mind, please open an issue to describe
  it and assign it to yourself.

Please feel free to contact any of the developers that are actively
contributing should you wish to coordinate further.

Repository Issue labels:

* _Bug_:          These issues refer to invalid or broken functionality and are
  the highest priority.
* _Enhancement_:  These issues refer to ideas for extending or improving the
  loader.

It is the maintainers goal for all issues to be assigned within one business day
of their submission.
If you choose to work on an issue that is assigned, simply coordinate with the
current assignee.

### How to Submit Fixes

* **Ensure that the bug was not already reported or fixed** by searching on
  GitHub under Issues and Pull Requests.
* Use the existing GitHub forking and pull request process.
  This will involve
  [forking the repository](https://help.github.com/articles/fork-a-repo/),
  creating a branch with your commits, and then
  [submitting a pull request](https://help.github.com/articles/using-pull-requests/).
* Please read and adhere to the style and process
  [guidelines](#coding-conventions-and-formatting) enumerated below.
* Please base your fixes on the master branch.
  SDK branches are generally not updated except for critical fixes needed to
  repair an SDK release.

#### Coding Conventions and Formatting

* Use the
 **[Google style guide](https://google.github.io/styleguide/cppguide.html)**
 for source code with the following exceptions:
  * The column limit is 132 (as opposed to the default value 80).
    The clang-format tool will handle this. See below.
  * The indent is 4 spaces instead of the default 2 spaces.
    Again, the clang-format tool will handle this.
  * If you can justify a reason for violating a rule in the guidelines,
    then you are free to do so. Be prepared to defend your
    decision during code review. This should be used responsibly.
    An example of a bad reason is "I don't like that rule."
    An example of a good reason is "This violates the style guide,
    but it improves type safety."

* Run **clang-format** on your changes to maintain consistent formatting
  * There are `.clang-format` files present in the repository to define
    clang-format settings which are found and used automatically by clang-format.
  * **clang-format** binaries are available from the LLVM orginization, here:
    [LLVM](https://clang.llvm.org/).
    Our CI system (Travis-CI) currently uses clang-format version 5.0.0 to
    check that the lines of code you have changed are formatted properly.
    It is recommended that you use the same version to format your code prior
    to submission.
  * A sample git workflow may look like:

>        # Make changes to the source.
>        $ git add -u .
>        $ git clang-format --style=file
>        # Check to see if clang-format made any changes and if they are OK.
>        $ git add -u .
>        $ git commit

* **Commit Messages**
  * Limit the subject line to 50 characters --
    this allows the information to display correctly in git/Github logs
  * Begin subject line with a one-word component description followed
    by a colon (e.g. loader, layers, tests, etc.)
  * Separate subject from body with a blank line
  * Wrap the body at 72 characters
  * Capitalize the subject line
  * Do not end the subject line with a period
  * Use the body to explain what and why vs. how
  * Use the imperative mode in the subject line.
    This just means to write it as a command (e.g. Fix the sprocket)

Strive for commits that implement a single or related set of functionality,
using as many commits as is necessary (more is better).

Please ensure that the repository compiles and passes tests without
error for each commit in your pull request.
Note that to be accepted into the repository, the pull request must
pass all tests on all supported platforms.
The automatic Github Travis and AppVeyor continuous integration features
will assist in enforcing this requirement.

#### Testing Your Changes

* Run the existing tests in the `tests` directory of the repository
  before and after each of your commits to check for any regressions.
  * Linux: `run_all_tests.sh`
  * Windows: `run_all_tests.ps1`

* Run tests that explicitly exercise your changes.
* Feel free to subject your code changes to other tests as well!

#### Coding Conventions for [CMake](http://cmake.org) files

* When editing configuration files for CMake, follow the style conventions of the surrounding code.
  * The column limit is 132.
  * The indent is 4 spaces.
  * CMake functions are lower-case.
  * Variable and keyword names are upper-case.
* The format is defined by
  [cmake-format](https://github.com/cheshirekow/cmake_format)
  using the `.cmake-format.py` file in the repository to define the settings.
  See the cmake-format page for information about its simple markup for comments.
* Disable reformatting of a block of comment lines by inserting
  a `# ~~~` comment line before and after that block.
* Disable any formatting of a block of lines by surrounding that block with
  `# cmake-format: off` and `# cmake-format: on` comment lines.
* To install: `sudo pip install cmake_format`
* To run: `cmake-format --in-place $FILENAME`
* **IMPORTANT (June 2018)** cmake-format v0.3.6 has a
  [bug]( https://github.com/cheshirekow/cmake_format/issues/50)
  that can corrupt the formatting of comment lines in CMake files.
  A workaround is to use the following command _before_ running cmake-format:
  `sed --in-place='' 's/^  *#/#/' $FILENAME`

### Contributor License Agreement (CLA)

You will be prompted with a one-time "click-through" CLA dialog as part of
submitting your pull request or other contribution to GitHub.

### License and Copyrights

All contributions made to the Vulkan-Loader repository are Khronos branded
and as such, any new files need to have the Khronos license
(Apache 2.0 style) and copyright included.
Please see an existing file in this repository for an example.

All contributions made to the LunarG repositories are to be made under
the Apache 2.0 license and any new files need to include this license
and any applicable copyrights.

You can include your individual copyright after any existing copyrights.
