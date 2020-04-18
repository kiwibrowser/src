'use strict';

// Rationale for this particular test character sequence, which is
// used in filenames and also in file contents:
//
// - ABC~ ensures the string starts with something we can read to
//   ensure it is from the correct source.
// - ‾¥ are inside JIS and help diagnose problems due to filesystem
//   encoding or locale
// - ≈ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - ¤ is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale
// - ･ is inside JIS and single-byte in some variants and helps
//   diagnose problems due to filesystem encoding or locale
// - ・ is inside JIS and double-byte and helps diagnose problems
//   due to filesystem encoding or locale
// - • is inside Windows-1252 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures these aren't
//   accidentally turned into e.g. control codes
// - ∙ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - · is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale
// - ☼ is inside IBM437 shadowing C0 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures these aren't
//   accidentally turned into e.g. control codes
// - ★ is inside JIS on a non-Kanji page and makes correct output easier to spot
// - 星 is inside JIS on a Kanji page and makes correct output easier to spot
// - 🌟 is outside the BMP
// - 星 repeated here ensures the correct codec state is used after a non-BMP
//   character
// - ★ repeated here also makes correct output easier to spot
// - ☼ is inside IBM437 shadowing C0 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures these aren't
//   accidentally turned into e.g. control codes
// - · is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale
// - ∙ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - • is inside Windows-1252 and again helps diagnose problems
//   due to filesystem encoding or locale
// - ・ is inside a double-byte range of JIS and helps diagnose
//   problems due to filesystem encoding or locale
// - ･ is inside a single-byte range of JIS in some variants and helps
//   diagnose problems due to filesystem encoding or locale
// - ¤ is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale
// - ≈ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - ¥‾ are inside a single-byte range of JIS and help
//   diagnose problems due to filesystem encoding or locale
// - ~XYZ ensures earlier errors don't lead to misencoding of
//   simple ASCII
//
// Overall the near-symmetry makes common I18N mistakes like
// off-by-1-after-non-BMP easier to spot. All the characters
// are also allowed in Windows Unicode filenames.
const kTestChars = 'ABC~‾¥≈¤･・•∙·☼★星🌟星★☼·∙•・･¤≈¥‾~XYZ';

// fileInputTest - verifies <input type=file> single file selection.
//
// Uses eventSender.beginDragWithFiles and related methods to select
// using drag-n-drop because that is currently the only file selection
// mechanism available to Blink layout tests, likely leading to the
// observed renderer crashes on POSIX-like systems using non-UTF-8
// locales.
//
// Fields in the parameter object:
//
// - fileNameSource: purely explanatory and gives a clue about which
//   character encoding is the source for the non-7-bit-ASCII parts of
//   the fileBaseName, or Unicode if no smaller-than-Unicode source
//   contains all the characters. Used in the test name.
// - fileBaseName: the not-necessarily-just-7-bit-ASCII file basename
//   for the test file. Used in the test name.
//
// NOTE: This does not correctly account for varying representation of
// combining characters across platforms and filesystems due to
// Unicode normalization or similar platform-specific normalization
// rules. For that reason none of the tests exercise such characters
// or character sequences.
const fileInputTest = ({
  fileNameSource,
  fileBaseName,
}) => {
  promise_test(async testCase => {

    if (document.readyState !== 'complete') {
      await new Promise(resolve => addEventListener('load', resolve));
    }
    assert_own_property(
        window,
        'eventSender',
        'This test relies on eventSender.beginDragWithFiles');

    const fileInput = Object.assign(document.createElement('input'), {
      type: 'file',
    });

    // This element must be at the top of the viewport so it can be dragged to.
    document.body.prepend(fileInput);
    testCase.add_cleanup(() => {
      document.body.removeChild(fileInput);
    });

    eventSender.beginDragWithFiles([`resources/${fileBaseName}`]);
    const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
    const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
    eventSender.mouseMoveTo(centerX, centerY);
    eventSender.mouseUp();
    // eventSender is synchronous so we do not wait for onchange
    assert_equals(
        fileInput.files[0].name,
        fileBaseName,
        `Dropped file should be ${fileBaseName}`);
    // Removes c:\fakepath\ or other pseudofolder and returns just the
    // final component of filePath; allows both / and \ as segment
    // delimiters.
    const baseNameOfFilePath = filePath => filePath.split(/[\/\\]/).pop();
    // For historical reasons .value will be prefixed with
    // c:\fakepath\, but the basename should match the dropped file
    // name exposed through the newer .files[0].name API. This check
    // verifies that assumption.
    assert_equals(
        fileInput.files[0].name,
        baseNameOfFilePath(fileInput.value),
        `The basename of the field's value should match its files[0].name`);
    // Only by fetching the file can we be sure the file actually
    // exists and the host filesystem is actually used. Before this
    // point the dragged-in filename from eventSender is not actually
    // checked against the host filesystem.
    const fileObjectUrl = URL.createObjectURL(fileInput.files[0]);
    testCase.add_cleanup(() => {
      URL.revokeObjectURL(fileObjectUrl);
    });
    const fileContents = await (await fetch(fileObjectUrl)).text();
    assert_equals(
        fileContents,
        kTestChars,
        `The file should contain ${kTestChars}`);
  }, `Select ${fileBaseName} (${fileNameSource}) in a file input`);
};
