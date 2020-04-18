package com.android.i18n.addressinput;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLEncoder;

public class AndroidAsyncEncodedRequestApi extends AndroidAsyncRequestApi {
  /**
   * A quick hack to transform a string into an RFC 3986 compliant URL.
   *
   * <p>TODO: Refactor the code to stop passing URLs around as strings, to eliminate the need for
   * this broken hack.
   */
  @Override
  protected URL stringToUrl(String url) throws MalformedURLException {
    int length = url.length();
    StringBuilder tmp = new StringBuilder(length);

    try {
      for (int i = 0; i < length; i++) {
        int j = i;
        char c = '\0';
        for (; j < length; j++) {
          c = url.charAt(j);
          if (c == ':' || c == '/') {
            break;
          }
        }
        if (j == length) {
          tmp.append(URLEncoder.encode(url.substring(i), "UTF-8"));
          break;
        } else if (j > i) {
          tmp.append(URLEncoder.encode(url.substring(i, j), "UTF-8"));
          tmp.append(c);
          i = j;
        } else {
          tmp.append(c);
        }
      }
    } catch (UnsupportedEncodingException e) {
      throw new AssertionError(e); // Impossible.
    }
    return new URL(tmp.toString());
  }
}
