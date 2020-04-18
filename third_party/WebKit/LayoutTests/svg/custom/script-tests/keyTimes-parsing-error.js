description("Tests parsing of keyTimes attribute.");

var animate = document.createElementNS("http://www.w3.org/2000/svg", "animate");
animate.setAttribute("keyTimes", ";;");
animate.setAttribute("keyTimes", "0;.25;.5;1;;");

var successfullyParsed = true;
