description('Tests offsetWidth using min-width and max-width');

function testWidth(style)
{
    var div = document.createElement('div');
    div.setAttribute('style', 'width:100px;' + style);
    document.body.appendChild(div);

    var result = div.offsetWidth;
    document.body.removeChild(div);
    return result;
}

shouldBe('testWidth("max-width:150px")', '100');
shouldBe('testWidth("max-width:50px")', '50');
shouldBe('testWidth("min-width:150px")', '150');
shouldBe('testWidth("min-width:50px")', '100');
