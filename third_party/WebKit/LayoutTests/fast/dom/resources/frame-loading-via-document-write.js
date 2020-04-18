function clickAnchor()
{
    var event = document.createEvent('MouseEvent');
    event.initEvent( 'click', true, true );
    document.getElementById('anchorLink').dispatchEvent(event);
}

function didImageLoad()
{
    var result = document.getElementById("result");

    var myImg = document.getElementById("myImg");
    if (myImg.naturalHeight == 0 && myImg.naturalWidth == 0) {
        result.innerHTML = "Image NOT loaded.";
    } else {
        result.innerHTML = "Image loaded.";
    }
}
