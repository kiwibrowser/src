description(
"This test checks that a correct exception is raised when calculating the base value of a dot expression fails."
);

// Should throw TypeError with appropriate message.
shouldThrow('(document.appendChild()).foobar()', '"TypeError: Failed to execute \'appendChild\' on \'Node\': 1 argument required, but only 0 present."');
