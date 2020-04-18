# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict, deque, namedtuple
from HTMLParser import HTMLParser, HTMLParseError
from itertools import groupby
from operator import itemgetter
import posixpath
from urlparse import urlsplit

from file_system_util import CreateURLsFromPaths
from path_util import AssertIsDirectory


Page = namedtuple('Page', 'status, links, anchors, anchor_refs')


def _SplitAnchor(url):
  components = urlsplit(url)
  return components.path, components.fragment


def _Process(path, renderer):
  '''Render the page at |path| using a |renderer| and process the contents of
  that page. Returns a |Page| namedtuple with fields for the http status code
  of the page render, the href of all the links that occurred on the page, all
  of the anchors on the page (ids and names), and all links that contain an
  anchor component.

  If a non-html page is properly rendered, a |Page| with status code 200 and
  all other fields empty is returned.
  '''
  parser = _ContentParser()
  response = renderer(path)

  if response.status != 200:
    return Page(response.status, (), (), ())
  if not path.endswith('.html'):
    return Page(200, (), (), ())

  try:
    parser.feed(str(response.content))
  except HTMLParseError:
    return Page(200, (), (), ())

  links, anchors = parser.links, parser.anchors
  if '/' in path:
    base, _ = path.rsplit('/', 1)
  else:
    base = ''
  edges = []
  anchor_refs = []

  # Convert relative links to absolute links and categorize links as edges
  # or anchor_refs.
  for link in links:
    # Files like experimental_history.html are refered to with the URL
    # experimental.history.html.
    head, last = link.rsplit('/', 1) if '/' in link else ('', link)
    last, anchor = _SplitAnchor(last)

    if last.endswith('.html') and last.count('.') > 1:
      last = last.replace('.', '_', last.count('.') - 1)
      link = posixpath.join(head, last)
      if anchor:
        link = '%s#%s' % (link, anchor)

    if link.startswith('#'):
      anchor_refs.append(link)
    else:
      if link.startswith('/'):
        link = link[1:]
      else:
        link = posixpath.normpath('%s/%s' % (base, link))

      if '#' in link:
        anchor_refs.append(link)
      else:
        edges.append(link)

  return Page(200, edges, anchors, anchor_refs)


class _ContentParser(HTMLParser):
  '''Parse an html file pulling out all links and anchor_refs, where an
  anchor_ref is a link that contains an anchor.
  '''

  def __init__(self):
    HTMLParser.__init__(self)
    self.links = []
    self.anchors = set()

  def handle_starttag(self, tag, raw_attrs):
    attrs = dict(raw_attrs)

    if tag == 'a':
      # Handle special cases for href's that: start with a space, contain
      # just a '.' (period), contain python templating code, are an absolute
      # url, are a zip file, or execute javascript on the page.
      href = attrs.get('href', '').strip()
      if href and not href == '.' and not '{{' in href:
        if not urlsplit(href).scheme in ('http', 'https'):
          if not href.endswith('.zip') and not 'javascript:' in href:
            self.links.append(href)

    if attrs.get('id'):
      self.anchors.add(attrs['id'])
    if attrs.get('name'):
      self.anchors.add(attrs['name'])


class LinkErrorDetector(object):
  '''Finds link errors on the doc server. This includes broken links, those with
  a target page that 404s or contain an anchor that doesn't exist, or pages that
  have no links to them.
  '''

  def __init__(self, file_system, renderer, public_path, root_pages):
    '''Creates a new broken link detector. |renderer| is a callable that takes
    a path and returns a full html page. |public_path| is the path to public
    template files. All URLs in |root_pages| are used as the starting nodes for
    the orphaned page search.
    '''
    AssertIsDirectory(public_path)
    self._file_system = file_system
    self._renderer = renderer
    self._public_path = public_path
    self._pages = defaultdict(lambda: Page(404, (), (), ()))
    self._root_pages = frozenset(root_pages)
    self._always_detached = frozenset((
        'apps/404.html',
        'extensions/404.html',
        'apps/private_apis.html',
        'extensions/private_apis.html'))
    self._redirection_whitelist = frozenset(('extensions/', 'apps/'))

    self._RenderAllPages()

  def _RenderAllPages(self):
    '''Traverses the public templates directory rendering each URL and
    processing the resultant html to pull out all links and anchors.
    '''
    top_level_directories = (
      ('docs/templates/public/', ''),
      ('docs/static/', 'static/'),
      ('docs/examples/', 'extensions/examples/'),
    )

    for dirpath, urlprefix in top_level_directories:
      files = CreateURLsFromPaths(self._file_system, dirpath, urlprefix)
      for url, path in files:
        self._pages[url] = _Process(url, self._renderer)

        if self._pages[url].status != 200:
          print(url, ', a url derived from the path', dirpath +
              ', resulted in a', self._pages[url].status)

  def _FollowRedirections(self, starting_url, limit=4):
    '''Follow redirection until a non-redirectable page is reached. Start at
    |starting_url| which must return a 301 or 302 status code.

    Return a tuple of: the status of rendering |staring_url|, the final url,
    and a list of the pages reached including |starting_url|. If no redirection
    occurred, returns (None, None, None).
    '''
    pages_reached = [starting_url]
    redirect_link = None
    target_page = self._renderer(starting_url)
    original_status = status = target_page.status
    count = 0

    while status in (301, 302):
      if count > limit:
        return None, None, None
      redirect_link = target_page.headers.get('Location')
      target_page = self._renderer(redirect_link)
      status = target_page.status
      pages_reached.append(redirect_link)
      count += 1

    if redirect_link is None:
      return None, None, None

    return original_status, redirect_link, pages_reached

  def _CategorizeBrokenLinks(self, url, page, pages):
    '''Find all broken links on a page and create appropriate notes describing
    why tehy are broken (broken anchor, target redirects, etc). |page| is the
    current page being checked and is the result of rendering |url|. |pages|
    is a callable that takes a path and returns a Page.
    '''
    broken_links = []

    for link in page.links + page.anchor_refs:
      components = urlsplit(link)
      fragment = components.fragment

      if components.path == '':
        if fragment == 'top' or fragment == '':
          continue
        if not fragment in page.anchors:
          broken_links.append((200, url, link, 'target anchor not found'))
      else:
        # Render the target page
        target_page = pages(components.path)

        if target_page.status != 200:
          if components.path in self._redirection_whitelist:
            continue

          status, relink, _ = self._FollowRedirections(components.path)
          if relink:
            broken_links.append((
                status,
                url,
                link,
                'redirects to %s' % relink))
          else:
            broken_links.append((
                target_page.status, url, link, 'target page not found'))

        elif fragment:
          if not fragment in target_page.anchors:
            broken_links.append((
                target_page.status, url, link, 'target anchor not found'))

    return broken_links

  def GetBrokenLinks(self):
    '''Find all broken links. A broken link is a link that leads to a page
    that does not exist (404s), redirects to another page (301 or 302), or
    has an anchor whose target does not exist.

    Returns a list of tuples of four elements: status, url, target_page,
    notes.
    '''
    broken_links = []

    for url in self._pages.keys():
      page = self._pages[url]
      if page.status != 200:
        continue
      broken_links.extend(self._CategorizeBrokenLinks(
          url, page, lambda x: self._pages[x]))

    return broken_links

  def GetOrphanedPages(self):
    '''Crawls the server find all pages that are connected to the pages at
    |seed_url|s. Return the links that are valid on the server but are not in
    part of the connected component containing the |root_pages|. These pages
    are orphans and cannot be reached simply by clicking through the server.
    '''
    pages_to_check = deque(self._root_pages.union(self._always_detached))
    found = set(self._root_pages) | self._always_detached

    while pages_to_check:
      item = pages_to_check.popleft()
      target_page = self._pages[item]

      if target_page.status != 200:
        redirected_page = self._FollowRedirections(item)[1]
        if not redirected_page is None:
          target_page = self._pages[redirected_page]

      for link in target_page.links:
        if link not in found:
          found.add(link)
          pages_to_check.append(link)

    all_urls = set(
        [url for url, page in self._pages.iteritems() if page.status == 200])

    return [url for url in all_urls - found if url.endswith('.html')]


def StringifyBrokenLinks(broken_links):
  '''Prints out broken links in a more readable format.
  '''
  def fixed_width(string, width):
    return "%s%s" % (string, (width - len(string)) * ' ')

  first_col_width = max(len(link[1]) for link in broken_links)
  second_col_width = max(len(link[2]) for link in broken_links)
  target = itemgetter(2)
  output = []

  def pretty_print(link, col_offset=0):
    return "%s -> %s %s" % (
        fixed_width(link[1], first_col_width - col_offset),
        fixed_width(link[2], second_col_width),
        link[3])

  for target, links in groupby(sorted(broken_links, key=target), target):
    links = list(links)
    # Compress messages
    if len(links) > 50 and not links[0][2].startswith('#'):
      message = "Found %d broken links (" % len(links)
      output.append("%s%s)" % (message, pretty_print(links[0], len(message))))
    else:
      for link in links:
        output.append(pretty_print(link))

  return '\n'.join(output)
