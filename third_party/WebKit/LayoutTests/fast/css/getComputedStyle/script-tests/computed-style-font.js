description("This test exercises the 'font' shorthand property in CSS computed styles.");

var testDiv = document.createElement('div');
document.body.appendChild(testDiv);

function computedFont(fontString) {
    testDiv.style.font = 'bold 600px serif';
    testDiv.style.font = fontString;
    return window.getComputedStyle(testDiv).getPropertyValue('font');
}

shouldBe("computedFont('10px sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('10px sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('10px SANS-SERIF')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('12px sans-serif')", "'normal normal 400 normal 12px / normal sans-serif'");
shouldBe("computedFont('12px  sans-serif')", "'normal normal 400 normal 12px / normal sans-serif'");
shouldBe("computedFont('10px sans-serif, sans-serif')", "'normal normal 400 normal 10px / normal sans-serif, sans-serif'");
shouldBe("computedFont('10px sans-serif, serif')", "'normal normal 400 normal 10px / normal sans-serif, serif'");
shouldBe("computedFont('12px ahem')", "'normal normal 400 normal 12px / normal ahem'");
shouldBe("computedFont('12px unlikely-font-name')", "'normal normal 400 normal 12px / normal unlikely-font-name'");
shouldBe("computedFont('100 10px sans-serif')", "'normal normal 100 normal 10px / normal sans-serif'");
shouldBe("computedFont('200 10px sans-serif')", "'normal normal 200 normal 10px / normal sans-serif'");
shouldBe("computedFont('300 10px sans-serif')", "'normal normal 300 normal 10px / normal sans-serif'");
shouldBe("computedFont('400 10px sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('normal 10px sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('500 10px sans-serif')", "'normal normal 500 normal 10px / normal sans-serif'");
shouldBe("computedFont('600 10px sans-serif')", "'normal normal 600 normal 10px / normal sans-serif'");
shouldBe("computedFont('700 10px sans-serif')", "'normal normal bold normal 10px / normal sans-serif'");
shouldBe("computedFont('bold 10px sans-serif')", "'normal normal bold normal 10px / normal sans-serif'");
shouldBe("computedFont('800 10px sans-serif')", "'normal normal 800 normal 10px / normal sans-serif'");
shouldBe("computedFont('900 10px sans-serif')", "'normal normal 900 normal 10px / normal sans-serif'");
shouldBe("computedFont('italic 10px sans-serif')", "'italic normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('small-caps 10px sans-serif')", "'normal small-caps 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('italic small-caps 10px sans-serif')", "'italic small-caps 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('italic small-caps bold 10px sans-serif')", "'italic small-caps 700 normal 10px / normal sans-serif'");
shouldBe("computedFont('10px/100% sans-serif')", "'normal normal 400 normal 10px / 10px sans-serif'");
shouldBe("computedFont('10px/100px sans-serif')", "'normal normal 400 normal 10px / 100px sans-serif'");
shouldBe("computedFont('10px/normal sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
shouldBe("computedFont('10px/normal sans-serif')", "'normal normal 400 normal 10px / normal sans-serif'");
