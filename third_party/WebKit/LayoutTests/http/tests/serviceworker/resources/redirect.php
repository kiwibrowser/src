<?php
if ($_SERVER['REQUEST_METHOD'] !== 'OPTIONS') {
  $url = $_GET['Redirect'];
  if ($url != "noLocation") {
    header("Location: $url");
  }
  if (isset($_GET['Status'])) {
    header("HTTP/1.1 " . $_GET["Status"]);
  } else {
    header("HTTP/1.1 302");
  }
}
if (isset($_GET['ACAOrigin'])) {
  $origins = explode(',', $_GET['ACAOrigin']);
  for ($i = 0; $i < sizeof($origins); ++$i)
    header("Access-Control-Allow-Origin: " . $origins[$i], false);
}
if (isset($_GET['ACAHeaders']))
    header("Access-Control-Allow-Headers: {$_GET['ACAHeaders']}");
if (isset($_GET['ACAMethods']))
    header("Access-Control-Allow-Methods: {$_GET['ACAMethods']}");
if (isset($_GET['ACACredentials']))
    header("Access-Control-Allow-Credentials: {$_GET['ACACredentials']}");
if (isset($_GET['ACEHeaders']))
    header("Access-Control-Expose-Headers: {$_GET['ACEHeaders']}");
if (isset($_GET['NoRedirectTest'])) {
    echo "report({jsonpResult:'noredirect'});";
}
?>
