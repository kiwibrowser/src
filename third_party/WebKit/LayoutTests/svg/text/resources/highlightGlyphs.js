// Highlight glyphs from multiple text elements using SVG text queries.

var colors = ["red", "orange", "yellow", "green", "blue", "indigo", "violet"];
function highlightGlyphs(textElements, highlightContainer) {
    // Highlight each glyph with a semi-transparent rectangle and
    // a number corresponding to the queried character index.
    for (var elemNum = 0; elemNum < textElements.length; ++elemNum) {
        var text = textElements[elemNum];
        var charCount = text.getNumberOfChars();
        for (var index = 0; index < charCount; ++index) {
            var color = colors[index % colors.length];
            highlightGlyph(text, index, color, highlightContainer);
        }
    }
}

function highlightGlyph(textElement, index, color, highlightContainer) {
    var extent = textElement.getExtentOfChar(index);

    // Highlight rect that we've selected using the extent information
    var rect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    rect.setAttribute("x", extent.x);
    rect.setAttribute("y", extent.y);
    rect.setAttribute("width", extent.width);
    rect.setAttribute("height", extent.height);
    rect.setAttribute("fill-opacity", "0.5");
    rect.setAttribute("fill", color);
    highlightContainer.appendChild(rect);

    // Output the start offset
    var text = document.createElementNS("http://www.w3.org/2000/svg", "text");
    text.setAttribute("x", extent.x + extent.width / 2);
    text.setAttribute("y", extent.y + extent.height + 5);
    text.setAttribute("text-anchor", "middle");
    text.setAttribute("font-size", 8);
    text.setAttribute("style", "-webkit-user-select: none; select: none;");
    text.appendChild(document.createTextNode(index));
    highlightContainer.appendChild(text);
}
