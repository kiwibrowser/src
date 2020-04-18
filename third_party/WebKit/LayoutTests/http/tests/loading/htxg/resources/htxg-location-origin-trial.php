<?php
header('HTTP/1.0 200 OK');
// generate_token.py http://127.0.0.1:8000 SignedHTTPExchange --expire-timestamp=2000000000
header("Origin-Trial: AgeFm+W/+DvAEn/vDjtqgd5PQX73YxKJLGBwLp14SiMjKFNTEUK2Bx5R3gH23JOfP+IL2EGNj+x9uhzh2krVRgsAAABaeyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiU2lnbmVkSFRUUEV4Y2hhbmdlIiwgImV4cGlyeSI6IDIwMDAwMDAwMDB9");
header("Content-Type: application/signed-exchange;v=b0");
$name = 'htxg-location.htxg';
$fp = fopen($name, 'rb');
header("Content-Length: " . filesize($name));
fpassthru($fp);
?>
