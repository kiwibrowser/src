
function createPlaceholder()
{
    var link = document.createElement("link");
    link.setAttribute("href", "resources/placeholder.html");
    link.setAttribute("rel", "import");
    document.head.appendChild(link);
    return link;
}
