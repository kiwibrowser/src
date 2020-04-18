if (window.testRunner)
    testRunner.dumpAsText();

document.getElementById("result").textContent = ("ó" == "\u0421") ? "PASS" : "FAIL";
