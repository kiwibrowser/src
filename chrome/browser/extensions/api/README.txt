This file describes steps and files needed when adding a new API to Chrome.
Before you start coding your new API, though, make sure you follow the process
described at:
  http://www.chromium.org/developers/design-documents/extensions/proposed-changes/apis-under-development

Two approaches are available for writing your API specification. The original
approach relies on JSON specification files. The more recent and simpler system 
uses Web IDL files, but does not yet support all the features of the JSON files.
Discuss with a member of the extensions team (aa@chromium.org) before you decide
which approach is better suited to your API.

The following steps suppose you're writing an experimental API called "Foo".

--------------------------------------------------------------------------------
APPROACH 1: JSON FILES

1) Write your API specification.
Create "chrome/common/extensions/api/experimental_foo.json". For inspiration 
look at the "app" API. Include descriptions fields to generate the
documentation.

2) Add your API specification to api.gyp.
Add "experimental_foo.json" to the "schema_files" section in
"chrome/common/extensions/api/api.gyp".

3) Write the API function handlers.
Create foo_api.cc and foo_api.h under "chrome/browser/extensions/api/foo". You
should use the JSON Schema Compiler. Look at the "permissions_api.cc" for
details on how to do that.

--------------------------------------------------------------------------------
APPROACH 2: IDL FILES

1) Write your API specification.
Create "chrome/common/extensions/api/experimental_foo.idl". For inspiration look
at "alarms.idl". Include comments, they will be used to automatically generate
the documentation.

2) Add your API specification to api.gyp.
Add "experimental_foo.idl" to the "schema_files" section in
"chrome/common/extensions/api/api.gyp".

3) Write the API function handlers.
Create foo_api.cc and foo_api.h under "chrome/browser/extensions/api/foo". You
should use the JSON Schema Compiler. Look at the "alarms_api.cc" for details on
how to do that.

--------------------------------------------------------------------------------
STEPS COMMON TO BOTH APPROACHES

6) Write support classes for your API
If your API needs any support classes add them to
"chrome/browser/extensions/api/foo". Some old APIs added their support classes
directly to chrome/browser/extensions. Don't do that.

7) Update the project with your new files.
The files you created in (3) and (5) should be added to
"chrome/chrome_browser_extensions.gypi".

--------------------------------------------------------------------------------
GENERATING DOCUMENTATION

8) Add a stub template in ../docs/templates/public corresponding to your API.
See other templates for inspiration.

9) Run ../docs/templates/server2/preview.py to view the generated documentation.

--------------------------------------------------------------------------------
WRITING TESTS

12) Write a unit test for your API.
Create "chrome/browser/extensions/api/foo/foo_api_unittest.cc" and test each of
your API methods. See "alarms_api_unittest.cc" for details. Once done add your
.cc to "chrome/chrome_tests.gypi".
