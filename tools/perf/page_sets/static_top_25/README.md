This directory contains static html snapshots of top trafficked web
sites. The URL list is:

https://www.google.com/search?q=barack+obama
https://mail.google.com/mail/ (logged in)
https://www.google.com/calendar/ (logged in)
https://www.google.com/search?q=cats&tbm=isch (logged in)
https://docs.google.com/document/d/1X-IKNjtEnx-WW5JIKRLsyhz5sbsat3mfTpAPUSX3_s4/view (logged in)
https://plus.google.com/110031535020051778989/posts (logged in)
http://www.youtube.com (logged in)
http://googlewebmastercentral.blogspot.com/
http://en.blog.wordpress.com/2012/09/04/freshly-pressed-editors-picks-for-august-2012/
https://www.facebook.com/barackobama
http://www.linkedin.com/in/linustorvalds
http://en.wikipedia.org/wiki/Wikipedia
https://twitter.com/katyperry
http://pinterest.com
http://espn.go.com
http://www.weather.com/weather/right-now/Mountain+View+CA+94043
http://games.yahoo.com
http://news.yahoo.com
http://www.cnn.com
http://www.amazon.com
http://www.ebay.com
http://booking.com
http://answers.yahoo.com
http://sports.yahoo.com/
http://techcrunch.com

Where "logged in" is noted, we use the "googletest" credentials.

Typical command to snapshot a static page:

% ./third_party/catapult/telemetry/bin/snap_page --browser=system \
    --url='https://www.cnn.com' --snapshot-path=cnn.html

For a logged in page where one must log in before snapshotting, use
the "--interactive" flag:

% ./third_party/catapult/telemetry/bin/snap_page --browser=system \
    --url='https://www.youtube.com' --snapshot-path=youtube.html --interactive
