description("This test ensures that WebKit doesn't crash when the document.createTouchList API is called with non-Touch parameters");

shouldThrow('document.createTouchList(document).item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');
shouldThrow('document.createTouchList({"a":1}).item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');
shouldThrow('document.createTouchList(new Array(5)).item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');
shouldThrow('document.createTouchList("string").item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');
shouldThrow('document.createTouchList(null).item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');
shouldThrow('document.createTouchList(undefined).item(0)', '"TypeError: Failed to execute \'createTouchList\' on \'Document\': parameter 1 is not of type \'Touch\'."');

isSuccessfullyParsed();
