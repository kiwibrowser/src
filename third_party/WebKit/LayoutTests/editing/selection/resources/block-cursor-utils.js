var blockCursorStartPosition;
var blockCursor;
var textNode;

function verifyBlockCursorLeftPositionAndWidth(elementId, expected)
{
    blockCursorStartPosition = new Array();

    var element = document.getElementById(elementId);
    textNode = element.firstChild;
    debug("Verifying block cursor position and width for each position in '" + textNode.nodeValue + "' in a " + element.style.direction + " block");

    for (var i = 0; i < textNode.length; i++) {
        evalAndLog("getSelection().collapse(textNode, "+i+")");

        blockCursor = internals.selectionBounds();
        blockCursorStartPosition.push(blockCursor.left);

        if (i > 0 && i < textNode.length) {
            if (expected[i-1] == ">")
                shouldBeTrue("blockCursorStartPosition["+(i-1)+"] > blockCursorStartPosition["+i+"]");
            else
                shouldBeTrue("blockCursorStartPosition["+(i-1)+"] < blockCursorStartPosition["+i+"]");
        }

        shouldBeTrue("getSelection().isCollapsed");
        shouldBeTrue("blockCursor.width > 1");
        shouldBe("internals.absoluteCaretBounds().width", "1");
    }

    evalAndLog("getSelection().collapse(textNode, "+i+")");
    blockCursor = internals.selectionBounds();
    shouldBeZero("blockCursor.width");
    shouldBe("internals.absoluteCaretBounds().width", "1");
    debug("");
}
