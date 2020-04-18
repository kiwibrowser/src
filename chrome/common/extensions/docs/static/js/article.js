// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Scroll handling.
//
// Switches the sidebar between floating on the left and position:fixed
// depending on whether it's scrolled into view, and manages the scroll-to-top
// button: click logic, and when to show it.
(function() {

var sidebar = document.querySelector('.inline-toc');
var articleBody = document.querySelector('[itemprop="articleBody"]');

// Bomb out unless we're on an article page and have a TOC.
// Ideally, template shouldn't load this JS file on a non-article page
if (!(sidebar && articleBody)) {
  return;
}

var isLargerThanMobileQuery =
    window.matchMedia('screen and (min-width: 581px)');

var toc = sidebar.querySelector('#toc');
var tocOffsetTop = sidebar.offsetParent.offsetTop + toc.offsetTop;

function updateTocOffsetTop() {
  // Note: Attempting to read offsetTop with sticky on causes toc overlap
  toc.classList.remove('sticky');
  tocOffsetTop = sidebar.offsetParent.offsetTop + toc.offsetTop;
}

function addPermalink(el) {
  el.classList.add('has-permalink');
  var id = el.id || el.textContent.toLowerCase().replace(' ', '-');
  el.insertAdjacentHTML('beforeend',
      '<a class="permalink" title="Permalink" href="#' + id + '">#</a>');
}

// Add permalinks to heading elements.
function addPermalinkHeadings(container) {
  if (container) {
    ['h2','h3','h4'].forEach(function(h, i) {
      [].forEach.call(container.querySelectorAll(h), addPermalink);
    });
  }
}

function toggleStickySidenav(){
  toc.classList.toggle('sticky', window.scrollY >= tocOffsetTop);
}

function onScroll(e) {
  toggleStickySidenav();
}

function onMediaQuery(e) {
  if (e.matches) {
    // On tablet & desktop, show permalinks, manage TOC position.
    document.body.classList.remove('no-permalink');
    document.addEventListener('scroll', onScroll);
    updateTocOffsetTop();
    toggleStickySidenav();
  } else {
    // On mobile, hide permalinks. TOC is hidden, doesn't need to scroll.
    document.body.classList.add('no-permalink');
    document.removeEventListener('scroll', onScroll);
  }
}

// Toggle collapsible sections (mobile).
articleBody.addEventListener('click', function(e) {
  if (e.target.localName == 'h2' && !isLargerThanMobileQuery.matches) {
    e.target.parentElement.classList.toggle('active');
  }
});

toc.addEventListener('click', function(e) {
  // React only if clicking on a toplevel menu anchor item
  // that is not currently open
  if (e.target.classList.contains('hastoc') &&
      !e.target.parentElement.classList.contains('active')) {
    e.stopPropagation();

    // close any previously open subnavs
    [].forEach.call(toc.querySelectorAll('.active'), function(li) {
      li.classList.remove('active');
    });

    // then open the clicked one
    e.target.parentElement.classList.add('active');
  }
});

// Add +/- expander to headings with subheading children.
[].forEach.call(toc.querySelectorAll('.toplevel'), function(heading) {
  if (heading.querySelector('.toc')) {
    heading.firstChild.classList.add('hastoc');
  }
});

isLargerThanMobileQuery.addListener(onMediaQuery);
onMediaQuery(isLargerThanMobileQuery);

addPermalinkHeadings(articleBody);

}());
