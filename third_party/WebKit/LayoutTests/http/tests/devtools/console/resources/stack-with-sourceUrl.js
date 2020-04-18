var c = "Some test"

function namedFunction()
{
    console.log(new Error(c).stack);
}

namedFunction();

//# sourceURL=foob.js
