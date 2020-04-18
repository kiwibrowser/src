# Action Recorder Extension

> An extension that generates Python scripts which automate integration testing
> through Chrome. It was primarily designed for testing Autofill but is easily
> portable to other uses.

## Usage

1.  Install the extension into Chrome as an unpacked extension on
    chrome://extensions (don't forget to turn on "Developer mode" on this page).
2.  Navigate to the desired start page or use the extension's dropdown menu
    (next to the omnibox) to go to the next "top 100" site.
3.  Use the dropdown menu or right-click context menu to start action recording.
4.  Proceed to click on page elements to navigate through the desired sequence
    of pages.
5.  To validate the input field types simply right-click on the inputs and
    select the appropriate 'Input Field Type'. Before performing any other
    actions, right click on the page and select 'Validate Field Types'.
6.  Select 'Stop & Copy', at which point the test code will be in your
    clipboard.
7.  Paste the generated code into the autofill_top_100.py file
    (components/test/data/password_manager/form_classification_tests).
8.  Clean up as necessary.

You might also visit arbitrary sites. Just go to a site and start recording
there.
