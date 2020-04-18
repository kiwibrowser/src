import os
import urllib
import time
import re
from cPickle import load, dump
from webob import Request, Response, html_escape
from webob import exc

class Commenter(object):

    def __init__(self, app, storage_dir):
        self.app = app
        self.storage_dir = storage_dir
        if not os.path.exists(storage_dir):
            os.makedirs(storage_dir)

    def __call__(self, environ, start_response):
        req = Request(environ)
        if req.path_info_peek() == '.comments':
            return self.process_comment(req)(environ, start_response)
        # This is the base path of *this* middleware:
        base_url = req.application_url
        resp = req.get_response(self.app)
        if resp.content_type != 'text/html' or resp.status_code != 200:
            # Not an HTML response, we don't want to
            # do anything to it
            return resp(environ, start_response)
        # Make sure the content isn't gzipped:
        resp.decode_content()
        comments = self.get_data(req.url)
        body = resp.body
        body = self.add_to_end(body, self.format_comments(comments))
        body = self.add_to_end(body, self.submit_form(base_url, req))
        resp.body = body
        return resp(environ, start_response)

    def get_data(self, url):
        # Double-quoting makes the filename safe
        filename = self.url_filename(url)
        if not os.path.exists(filename):
            return []
        else:
            f = open(filename, 'rb')
            data = load(f)
            f.close()
            return data

    def save_data(self, url, data):
        filename = self.url_filename(url)
        f = open(filename, 'wb')
        dump(data, f)
        f.close()

    def url_filename(self, url):
        return os.path.join(self.storage_dir, urllib.quote(url, ''))

    _end_body_re = re.compile(r'</body.*?>', re.I|re.S)

    def add_to_end(self, html, extra_html):
        """
        Adds extra_html to the end of the html page (before </body>)
        """
        match = self._end_body_re.search(html)
        if not match:
            return html + extra_html
        else:
            return html[:match.start()] + extra_html + html[match.start():]

    def format_comments(self, comments):
        if not comments:
            return ''
        text = []
        text.append('<hr>')
        text.append('<h2><a name="comment-area"></a>Comments (%s):</h2>' % len(comments))
        for comment in comments:
            text.append('<h3><a href="%s">%s</a> at %s:</h3>' % (
                html_escape(comment['homepage']), html_escape(comment['name']),
                time.strftime('%c', comment['time'])))
            # Susceptible to XSS attacks!:
            text.append(comment['comments'])
        return ''.join(text)

    def submit_form(self, base_path, req):
        return '''<h2>Leave a comment:</h2>
        <form action="%s/.comments" method="POST">
         <input type="hidden" name="url" value="%s">
         <table width="100%%">
          <tr><td>Name:</td>
              <td><input type="text" name="name" style="width: 100%%"></td></tr>
          <tr><td>URL:</td>
              <td><input type="text" name="homepage" style="width: 100%%"></td></tr>
         </table>
         Comments:<br>
         <textarea name="comments" rows=10 style="width: 100%%"></textarea><br>
         <input type="submit" value="Submit comment">
        </form>
        ''' % (base_path, html_escape(req.url))

    def process_comment(self, req):
        try:
            url = req.params['url']
            name = req.params['name']
            homepage = req.params['homepage']
            comments = req.params['comments']
        except KeyError, e:
            resp = exc.HTTPBadRequest('Missing parameter: %s' % e)
            return resp
        data = self.get_data(url)
        data.append(dict(
            name=name,
            homepage=homepage,
            comments=comments,
            time=time.gmtime()))
        self.save_data(url, data)
        resp = exc.HTTPSeeOther(location=url+'#comment-area')
        return resp

if __name__ == '__main__':
    import optparse
    parser = optparse.OptionParser(
        usage='%prog --port=PORT BASE_DIRECTORY'
        )
    parser.add_option(
        '-p', '--port',
        default='8080',
        dest='port',
        type='int',
        help='Port to serve on (default 8080)')
    parser.add_option(
        '--comment-data',
        default='./comments',
        dest='comment_data',
        help='Place to put comment data into (default ./comments/)')
    options, args = parser.parse_args()
    if not args:
        parser.error('You must give a BASE_DIRECTORY')
    base_dir = args[0]
    from paste.urlparser import StaticURLParser
    app = StaticURLParser(base_dir)
    app = Commenter(app, options.comment_data)
    from wsgiref.simple_server import make_server
    httpd = make_server('localhost', options.port, app)
    print 'Serving on http://localhost:%s' % options.port
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print '^C'
