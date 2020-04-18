<script>
if (window.testRunner)
    testRunner.dumpAsText();

var observer = new MutationObserver(function(mutations) {
    mutations.forEach(function(mutation) {
        if (mutation.type === 'childList')
            console.log("PASS: childList mutation event fired from HTML parser change.");
    });
});

observer.observe(document.documentElement, {childList: true});
</script>
<body>
Test result is dumped on console.log.

foo
<?php sleep(1); ?>
bar
</body>
