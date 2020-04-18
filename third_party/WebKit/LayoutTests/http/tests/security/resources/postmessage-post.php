<script>
top.postMessage(<?php
    echo json_encode($_POST);
?>, "*");
</script>
