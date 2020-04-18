description(
'Tests to make sure we do not gc the constants contained by functions defined inside eval code.  To pass we need to not crash.'
);

evalStringTest = "'test'";
evalString = "function f() { shouldBe(\"'test'\", evalStringTest) }; f()";
function doTest() {
    eval(evalString);
}
doTest();
gc();

// Scribble all over the registerfile and c stacks
a={};
a*=({}*{}+{}*{})*({}*{}+{}*{})+({}*{}+{}*{});
[[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]]];
gc();
a={};
a*=({}*{}+{}*{})*({}*{}+{}*{})+({}*{}+{}*{});
[[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]]];
gc();
a={};
a*=({}*{}+{}*{})*({}*{}+{}*{})+({}*{}+{}*{});
[[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]],[[1,2,3],[1,2,3],[1,2,3]]];

doTest();
