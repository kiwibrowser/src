(function() {

function checkColumnRowValues(gridItem, columnValue, rowValue)
{
    this.gridItem = gridItem;
    var gridItemId = gridItem.id ? gridItem.id : "gridItem";

    var gridColumnStartEndValues = columnValue.split("/")
    var gridColumnStartValue = gridColumnStartEndValues[0].trim();
    var gridColumnEndValue = gridColumnStartEndValues[1].trim();

    var gridRowStartEndValues = rowValue.split("/")
    var gridRowStartValue = gridRowStartEndValues[0].trim();
    var gridRowEndValue = gridRowStartEndValues[1].trim();

    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-column')", columnValue);
    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-column-start')", gridColumnStartValue);
    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-column-end')", gridColumnEndValue);
    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-row')", rowValue);
    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-row-start')", gridRowStartValue);
    shouldBeEqualToString("getComputedStyle(" + gridItemId + ", '').getPropertyValue('grid-row-end')", gridRowEndValue);
}

window.testColumnRowCSSParsing = function(id, columnValue, rowValue)
{
    var gridItem = document.getElementById(id);
    checkColumnRowValues(gridItem, columnValue, rowValue);
}

window.testColumnRowJSParsing = function(columnValue, rowValue, expectedColumnValue, expectedRowValue)
{
    var gridItem = document.createElement("div");
    var gridElement = document.getElementsByClassName("grid")[0];
    gridElement.appendChild(gridItem);
    gridItem.style.gridColumn = columnValue;
    gridItem.style.gridRow = rowValue;

    checkColumnRowValues(gridItem, expectedColumnValue ? expectedColumnValue : columnValue, expectedRowValue ? expectedRowValue : rowValue);

    gridElement.removeChild(gridItem);
}

window.testColumnStartRowStartJSParsing = function(columnStartValue, rowStartValue, expectedColumnStartValue, expectedRowStartValue)
{
    var gridItem = document.createElement("div");
    var gridElement = document.getElementsByClassName("grid")[0];
    gridElement.appendChild(gridItem);
    gridItem.style.gridColumnStart = columnStartValue;
    gridItem.style.gridRowStart = rowStartValue;

    if (expectedColumnStartValue === undefined)
        expectedColumnStartValue = columnStartValue;
    if (expectedRowStartValue === undefined)
        expectedRowStartValue = rowStartValue;

    checkColumnRowValues(gridItem, expectedColumnStartValue + " / auto", expectedRowStartValue + " / auto");

    gridElement.removeChild(gridItem);
}

window.testColumnEndRowEndJSParsing = function(columnEndValue, rowEndValue, expectedColumnEndValue, expectedRowEndValue)
{
    var gridItem = document.createElement("div");
    var gridElement = document.getElementsByClassName("grid")[0];
    gridElement.appendChild(gridItem);
    gridItem.style.gridColumnEnd = columnEndValue;
    gridItem.style.gridRowEnd = rowEndValue;

    if (expectedColumnEndValue === undefined)
        expectedColumnEndValue = columnEndValue;
    if (expectedRowEndValue === undefined)
        expectedRowEndValue = rowEndValue;

    checkColumnRowValues(gridItem, "auto / " + expectedColumnEndValue, "auto / " + expectedRowEndValue);

    gridElement.removeChild(gridItem);
}

window.testColumnRowInvalidJSParsing = function(columnValue, rowValue)
{
    var gridItem = document.createElement("div");
    document.body.appendChild(gridItem);
    gridItem.style.gridColumn = columnValue;
    gridItem.style.gridRow = rowValue;

    checkColumnRowValues(gridItem, "auto / auto", "auto / auto");

    document.body.removeChild(gridItem);
}

var placeholderParentColumnStartValueForInherit = "6";
var placeholderParentColumnEndValueForInherit = "span 2";
var placeholderParentColumnValueForInherit = placeholderParentColumnStartValueForInherit + " / " + placeholderParentColumnEndValueForInherit;
var placeholderParentRowStartValueForInherit = "span 1";
var placeholderParentRowEndValueForInherit = "7";
var placeholderParentRowValueForInherit = placeholderParentRowStartValueForInherit + " / " + placeholderParentRowEndValueForInherit;

var placeholderColumnStartValueForInitial = "1";
var placeholderColumnEndValueForInitial = "span 2";
var placeholderColumnValueForInitial = placeholderColumnStartValueForInitial + " / " + placeholderColumnEndValueForInitial;
var placeholderRowStartValueForInitial = "span 3";
var placeholderRowEndValueForInitial = "5";
var placeholderRowValueForInitial = placeholderRowStartValueForInitial + " / " + placeholderRowEndValueForInitial;

function setupInheritTest()
{
    var parentElement = document.createElement("div");
    document.body.appendChild(parentElement);
    parentElement.style.gridColumn = placeholderParentColumnValueForInherit;
    parentElement.style.gridRow = placeholderParentRowValueForInherit;

    var gridItem = document.createElement("div");
    parentElement.appendChild(gridItem);
    return parentElement;
}

function setupInitialTest()
{
    var gridItem = document.createElement("div");
    document.body.appendChild(gridItem);
    gridItem.style.gridColumn = placeholderColumnValueForInitial;
    gridItem.style.gridRow = placeholderRowValueForInitial;

    checkColumnRowValues(gridItem, placeholderColumnValueForInitial, placeholderRowValueForInitial);
    return gridItem;
}

window.testColumnRowInheritJSParsing = function(columnValue, rowValue)
{
    var parentElement = setupInheritTest();
    var gridItem = parentElement.firstChild;
    gridItem.style.gridColumn = columnValue;
    gridItem.style.gridRow = rowValue;

    checkColumnRowValues(gridItem, columnValue !== "inherit" ? columnValue : placeholderParentColumnValueForInherit, rowValue !== "inherit" ? rowValue : placeholderParentRowValueForInherit);

    document.body.removeChild(parentElement);
}

window.testColumnStartRowStartInheritJSParsing = function(columnStartValue, rowStartValue)
{
    var parentElement = setupInheritTest();
    var gridItem = parentElement.firstChild;
    gridItem.style.gridColumnStart = columnStartValue;
    gridItem.style.gridRowStart = rowStartValue;

    // Initial value is 'auto' but we shouldn't touch the opposite grid line.
    var columnValueForInherit = (columnStartValue !== "inherit" ? columnStartValue : placeholderParentColumnStartValueForInherit) + " / auto";
    var rowValueForInherit = (rowStartValue !== "inherit" ? rowStartValue : placeholderParentRowStartValueForInherit) + " / auto";
    checkColumnRowValues(parentElement.firstChild, columnValueForInherit, rowValueForInherit);

    document.body.removeChild(parentElement);
}

window.testColumnEndRowEndInheritJSParsing = function(columnEndValue, rowEndValue)
{
    var parentElement = setupInheritTest();
    var gridItem = parentElement.firstChild;
    gridItem.style.gridColumnEnd = columnEndValue;
    gridItem.style.gridRowEnd = rowEndValue;

    // Initial value is 'auto' but we shouldn't touch the opposite grid line.
    var columnValueForInherit = "auto / " + (columnEndValue !== "inherit" ? columnEndValue : placeholderParentColumnEndValueForInherit);
    var rowValueForInherit = "auto / " + (rowEndValue !== "inherit" ? rowEndValue : placeholderParentRowEndValueForInherit);
    checkColumnRowValues(parentElement.firstChild, columnValueForInherit, rowValueForInherit);

    document.body.removeChild(parentElement);
}

window.testColumnRowInitialJSParsing = function()
{
    var gridItem = setupInitialTest();

    gridItem.style.gridColumn = "initial";
    checkColumnRowValues(gridItem, "auto / auto", placeholderRowValueForInitial);

    gridItem.style.gridRow = "initial";
    checkColumnRowValues(gridItem, "auto / auto", "auto / auto");

    document.body.removeChild(gridItem);
}

window.testColumnStartRowStartInitialJSParsing = function()
{
    var gridItem = setupInitialTest();

    gridItem.style.gridColumnStart = "initial";
    checkColumnRowValues(gridItem, "auto / " + placeholderColumnEndValueForInitial, placeholderRowValueForInitial);

    gridItem.style.gridRowStart = "initial";
    checkColumnRowValues(gridItem,  "auto / " + placeholderColumnEndValueForInitial, "auto / " + placeholderRowEndValueForInitial);

    document.body.removeChild(gridItem);
}

window.testEndAfterInitialJSParsing = function()
{
    var gridItem = setupInitialTest();

    gridItem.style.gridColumnEnd = "initial";
    checkColumnRowValues(gridItem, placeholderColumnStartValueForInitial + " / auto", placeholderRowValueForInitial);

    gridItem.style.gridRowEnd = "initial";
    checkColumnRowValues(gridItem, placeholderColumnStartValueForInitial + " / auto", placeholderRowStartValueForInitial + " / auto");

    document.body.removeChild(gridItem);
}

})();
