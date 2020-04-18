// Invokes "fully exit fullscreen" for document.
function fully_exit_fullscreen(document)
{
    // FIXME: window.open() invokes "fully exit fullscreen", but the HTML spec
    // doesn't say so, and none of the spec'd behavior is implemented:
    // https://www.w3.org/Bugs/Public/show_bug.cgi?id=26584
    document.defaultView.open("data:text/html,<script>window.close()</script>");
}
