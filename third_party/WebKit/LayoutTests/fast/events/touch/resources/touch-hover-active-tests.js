function getHoverActiveState(e)
{
    var states = {
        "rgb(0, 0, 255)": "default",
        "rgb(0, 255, 0)": "active",
        "rgb(255, 0, 0)": "hovered",
        "rgb(255, 255, 0)": "hoveredAndActive"
    };

    var color = window.getComputedStyle(e).backgroundColor;
    var result = states[color];
    if (!result)
        result = "unknown: " + color;
    return result;
}

function elementCenter(e)
{
    return {
        x: e.offsetLeft + e.offsetWidth / 2,
        y: e.offsetTop + e.offsetHeight / 2
    }
}

function shouldBeDefault(e) { shouldBeEqualToString(e, "default"); }
function shouldBeOnlyActive(e) { shouldBeEqualToString(e, "active"); }
function shouldBeOnlyHovered(e) { shouldBeEqualToString(e, "hovered"); }
function shouldBeHoveredAndActive(e) { shouldBeEqualToString(e, "hoveredAndActive"); }
