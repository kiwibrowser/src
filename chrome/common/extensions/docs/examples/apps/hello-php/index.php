<?php
/**
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * A "Hello world!" for the Chrome Web Store Licensing API, in PHP. This
 * program logs the user in with Google's Federated Login API (OpenID), fetches
 * their license state with OAuth, and prints one of these greetings as
 * appropriate:
 *
 *   1. This user has FREE_TRIAL access to this application ( appId: 1 )
 *   2. This user has FULL access to this application ( appId: 1 )
 *   3. This user has NO access to this application ( appId: 1 )
 *
 * This code makes use of a popup ui extension to the OpenID protocol. Instead
 * of the user being redirected to the Google login page, a popup window opens
 * to the login page, keeping the user on the main application page. See
 * popuplib.js
 *
 * Eric Bidelman <ericbidelman@chromium.org>
 */

session_start();

require_once 'lib/oauth/OAuth.php';
require_once 'lib/lightopenid/openid.php';

// Full URL of the current application is running under.
$scheme = (!isset($_SERVER['HTTPS']) || $_SERVER['HTTPS'] != 'on') ? 'http' :
                                                                     'https';
$selfUrl = "$scheme://{$_SERVER['HTTP_HOST']}{$_SERVER['PHP_SELF']}";


/**
 * Wrapper class to make calls to the Chrome Web Store License Server.
 */
class LicenseServerClient {

  const LICENSE_SERVER_HOST = 'https://www.googleapis.com';
  const CONSUMER_KEY = 'anonymous';
  const CONSUMER_SECRET = 'anonymous';
  const APP_ID = '1';  // Change to the correct id of your application.
  const TOKEN = '[REPLACE THIS WITH YOUR OAUTH TOKEN]';
  const TOKEN_SECRET = '[REPLACE THIS WITH YOUR OAUTH TOKEN SECRET]';
  public $consumer;
  public $token;
  public $signatureMethod;

  public function __construct() {
    $this->consumer = new OAuthConsumer(
        self::CONSUMER_KEY, self::CONSUMER_SECRET, NULL);
    $this->token = new OAuthToken(self::TOKEN, self::TOKEN_SECRET);
    $this->signatureMethod = new OAuthSignatureMethod_HMAC_SHA1();
  }

  /**
   * Makes an HTTP GET request to the specified URL.
   *
   * @param string $url Full URL of the resource to access
   * @param string $request OAuthRequest containing the signed request to make.
   * @param array $extraHeaders (optional) Array of headers.
   * @param bool $returnResponseHeaders True if resp headers should be returned.
   * @return string Response body from the server.
   */
  protected function send_signed_get($request, $extraHeaders=NULL,
                                     $returnRequestHeaders=false,
                                     $returnResponseHeaders=false) {
    $url = explode('?', $request->to_url());
    $curl = curl_init($url[0]);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($curl, CURLOPT_FAILONERROR, false);
    curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);

    // Return request headers in the response.
    curl_setopt($curl, CURLINFO_HEADER_OUT, $returnRequestHeaders);

    // Return response headers in the response?
    if ($returnResponseHeaders) {
      curl_setopt($curl, CURLOPT_HEADER, true);
    }

    $headers = array($request->to_header());
    if (is_array($extraHeaders)) {
      $headers = array_merge($headers, $extraHeaders);
    }
    curl_setopt($curl, CURLOPT_HTTPHEADER, $headers);

    // Execute the request.  If an error occurs fill the response body with it.
    $response = curl_exec($curl);
    if (!$response) {
      $response = curl_error($curl);
    }

    // Add server's response headers to our response body
    $response = curl_getinfo($curl, CURLINFO_HEADER_OUT) . $response;

    curl_close($curl);

    return $response;
  }

  public function checkLicense($userId) {
    $url = self::LICENSE_SERVER_HOST . '/chromewebstore/v1/licenses/' .
           self::APP_ID . '/' . urlencode($userId);

    $request = OAuthRequest::from_consumer_and_token(
        $this->consumer, $this->token, 'GET', $url, array());

    $request->sign_request($this->signatureMethod, $this->consumer,
                           $this->token);

    return $this->send_signed_get($request);
  }
}

try {
  $openid = new LightOpenID();
  $userId = $openid->identity;
  if (!isset($_GET['openid_mode'])) {
    // This section performs the OpenID dance with the normal redirect. Use it
    // if you want an alternative to the popup UI.
    if (isset($_GET['login'])) {
      $openid->identity = 'https://www.google.com/accounts/o8/id';
      $openid->required = array('namePerson/first', 'namePerson/last',
                                'contact/email');
      header('Location: ' . $openid->authUrl());
    }
  } else if ($_GET['openid_mode'] == 'cancel') {
    echo 'User has canceled authentication!';
  } else {
    $userId = $openid->validate() ? $openid->identity : '';
    $_SESSION['userId'] = $userId;
    $attributes = $openid->getAttributes();
    $_SESSION['attributes'] = $attributes;
  }
} catch(ErrorException $e) {
  echo $e->getMessage();
  exit;
}

if (isset($_REQUEST['popup']) && !isset($_SESSION['redirect_to'])) {
  $_SESSION['redirect_to'] = $selfUrl;
  echo '<script type = "text/javascript">window.close();</script>';
  exit;
} else if (isset($_SESSION['redirect_to'])) {
  $redirect = $_SESSION['redirect_to'];
  unset($_SESSION['redirect_to']);
  header('Location: ' . $redirect);
} else if (isset($_REQUEST['queryLicenseServer'])) {
  $ls = new LicenseServerClient();
  echo $ls->checkLicense($_REQUEST['user_id']);
  exit;
} else if (isset($_GET['logout'])) {
  unset($_SESSION['attributes']);
  unset($_SESSION['userId']);
  header('Location: ' . $selfUrl);
}
?>

<!DOCTYPE html>
<html>
  <head>
  <meta charset="utf-8" />
  <link href="main.css" type="text/css" rel="stylesheet" />
  <script type="text/javascript" src="popuplib.js"></script>
  <script type="text/html" id="ls_tmpl">
    <div id="access-level">
      <% if (result.toLowerCase() == 'yes') { %>
        This user has <span class="<%= accessLevel.toLowerCase() %>"><%= accessLevel %></span> access to this application ( appId: <%= appId %> )
      <% } else { %>
        This user has <span class="<%= result.toLowerCase() %>"><%= result %></span> access to this application ( appId: <%= appId %> )
      <% } %>
    </div>
  </script>
  </head>
  <body>
    <nav>
      <?php if (!isset($_SESSION['userId'])): ?>
        <a href="javascript:" onclick="openPopup(450, 500, this);">Sign in</a>
      <?php else: ?>
        <span>Welcome <?php echo @$_SESSION['attributes']['namePerson/first'] ?> <?php echo @$_SESSION['attributes']['namePerson/last'] ?> ( <?php echo $_SESSION['attributes']['contact/email'] ?> )</span>
        <a href="?logout">Sign out</a>
      <?php endif; ?>
    </nav>
    <?php if (isset($_SESSION['attributes'])): ?>
      <div id="container">
        <form action="<?php echo "$selfUrl?queryLicenseServer" ?>" onsubmit="return queryLicenseServer(this);">
          <input type="hidden" id="user_id" name="user_id" value="<?php echo $_SESSION['userId'] ?>" />
          <input type="submit" value="Check user's access" />
        </form>
        <div id="license-server-response"></div>
      </div>
    <?php endif; ?>
    <script>
      // Simple JavaScript Templating
      // John Resig - http://ejohn.org/ - MIT Licensed
      (function(){
        var cache = {};

        this.tmpl = function tmpl(str, data){
          // Figure out if we're getting a template, or if we need to
          // load the template - and be sure to cache the result.
          var fn = !/\W/.test(str) ?
            cache[str] = cache[str] ||
              tmpl(document.getElementById(str).innerHTML) :

            // Generate a reusable function that will serve as a template
            // generator (and which will be cached).
            new Function("obj",
              "var p=[],print=function(){p.push.apply(p,arguments);};" +

              // Introduce the data as local variables using with(){}
              "with(obj){p.push('" +

              // Convert the template into pure JavaScript
              str
                .replace(/[\r\t\n]/g, " ")
                .split("<%").join("\t")
                .replace(/((^|%>)[^\t]*)'/g, "$1\r")
                .replace(/\t=(.*?)%>/g, "',$1,'")
                .split("\t").join("');")
                .split("%>").join("p.push('")
                .split("\r").join("\\'")
            + "');}return p.join('');");

          // Provide some basic currying to the user
          return data ? fn( data ) : fn;
        };
      })();

      function queryLicenseServer(form) {
        var userId = form.user_id.value;

        if (!userId) {
          alert('No OpenID specified!');
          return false;
        }

        var req = new XMLHttpRequest();
        req.onreadystatechange = function(e) {
          if (this.readyState == 4) {
            var resp = JSON.parse(this.responseText);
            var el = document.getElementById('license-server-response');
            if (resp.error) {
              el.innerHTML = ['<div class="error">Error ', resp.error.code,
                              ': ', resp.error.message, '</div>'].join('');
            } else {
              el.innerHTML = tmpl('ls_tmpl', resp);
            }
          }
        };
        var url =
            [form.action, '&user_id=', encodeURIComponent(userId)].join('');
        req.open('GET', url, true);
        req.send(null);

        return false;
      }

      function openPopup(w, h, link) {
        var extensions = {
          'openid.ns.ext1': 'http://openid.net/srv/ax/1.0',
          'openid.ext1.mode': 'fetch_request',
          'openid.ext1.type.email': 'http://axschema.org/contact/email',
          'openid.ext1.type.first': 'http://axschema.org/namePerson/first',
          'openid.ext1.type.last': 'http://axschema.org/namePerson/last',
          'openid.ext1.required': 'email,first,last',
          'openid.ui.icon': 'true'
        };

        var googleOpener = popupManager.createPopupOpener({
          opEndpoint: 'https://www.google.com/accounts/o8/ud',
          returnToUrl: '<?php echo "$selfUrl?popup=true" ?>',
          onCloseHandler: function() {
            window.location = '<?php echo $selfUrl ?>';
          },
          shouldEncodeUrls: false,
          extensions: extensions
        });
        link.parentNode.appendChild(
            document.createTextNode('Authenticating...'));
        link.parentNode.removeChild(link);
        googleOpener.popup(w, h);
      }
    </script>
  </body>
</html>
