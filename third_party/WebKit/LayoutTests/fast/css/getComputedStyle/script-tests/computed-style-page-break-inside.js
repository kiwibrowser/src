description("Test that page-break-inside property is not inherited");

var parent = document.createElement("div");
parent.setAttribute("style", "page-break-inside: avoid");

var child = document.createElement("div");
parent.appendChild(child);
document.body.appendChild(parent);

shouldBe("window.getComputedStyle(parent).pageBreakInside", '"avoid"');
shouldBe("window.getComputedStyle(child).pageBreakInside", '"auto"');

document.body.removeChild(parent);
