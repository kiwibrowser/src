[TOC]

# Touch-to-Search

Touch-to-Search is a feature that allows a Chrome user to quickly run Google
searches by touching the text they wish to search for on a web page they are 
viewing. The search results are displayed in an overlay that can be "promoted"
into a new tab if the user desires.

### Nomenclature

The original name for Touch-to-Search was "Contextual Search", and usage of that
name persists in many contexts, including all of the class names in this code.

### Availability

Touch-to-Search was available in a limited Finch trial on iOS phone devices in 
M41-44, and will be available to all users (phone and tablet) in M49. It is 
also available via `chrome://flags`.

### Bug component

[`Cr-UI-Browser-Mobile-TouchToSearch`](https://code.google.com/p/chromium/issues/list?can=2&q=Cr%3DUI-Browser-Mobile-TouchToSearch)

## Behavior {#behavior}

Generally speaking, a touch-to-search interaction goes through some or all of 
the following phases, and the transitions between each phase are asynchronous.

1. **Touch**: The user taps on a word on the web page. This yields both the 
tapped word and the surrounding text for use as context. 
2. **Dispatch**: The word and context are dispatched as a query to Google Search
servers. The Touch-to-Search UI is displayed with the initial tapped text 
and surrounding context displayed.
3. **Resolution**: The resolved search term is returned along with the URL of 
the search results page. The UI is updated with the resolved search term, and 
the search results page starts to load.
5. **Page Display**: The search tab page load is complete.
6. **Promotion**: The contextual search panel is promoted to a new tab. This is
initiated by user action (such as tapping on the search term, or tapping a link 
in the search results tab).

There is a slightly different flow of states for when the user long-presses on
a word on the web page, initiating an iOS text selection.

Additionally, during or between many of these phases, the user may be 
interacting with the touch-to-search panel, changing its position.

### Opt-Out flow

### Privacy controls

## Controller architecture

## Touch detection and handling

Touch detection is handled both by native code and injected javascript. Broadly
speaking, detection happens as follows:

1. The Touch-to-Search controller adds a `UITapGestureRecognizer` to the current
tab's web view. 
2. When this tap recognizer fires, the injected JavaScript method 
`handleTapAtPoint()` is called. This method determines if the tap was inside 
a plain text word. 
3. If it was, a 'context' object is returned that includes the URL of the page, 
the word that was tapped, the text surrounding the tapped word, and the position
of the tapped word inside the text. 
4. Once the Touch-to-Search controller receives the callback from the javascript
call, it displays the Touch-to-Search UI and composes and sends the search query
for resolution. 

This takes the Touch-to-Search system from the **Touch** to **Dispatch** phases 
as described in [Behavior](#behavior), above.

(There are numerous complicating factors inside this sequence; they are detailed
in sections below.) 

### DOM Mutation Lock.

In order to highlight text on the web page, Touch-to-Search inserts (via the 
JavaScript component) HTML and CSS. So that this DOM modification doesn't 
interfere with other DOM modifications performed by Chrome (for example, 
find-in-page highlighting), tap processing is performed after obtaining a DOM 
mutation lock on the page.

### Tap delay.

Some pages may dynamically adjust the page in response to a tap (for example,
a non-native drop-down menu may be implemented this way). In order to give a 
page time to update in response to a tap, tap processing is delayed by
(currently) 800ms.

### Tap rejection on mutated DOM elements.

Tapping on sections of a page that have recently been changed doesn't trigger
touch-to-search, again on the assumption that a recently-updated part of the
page is actually a control updating in response to the tap. To detect this, each
DOM modification on the page is observed via a JavaScript MutationObserver. 
For each mutation, the time and the mutated DOM element are recorded. Taps that
are in elements contained by or containing elements modified since the original
native tap event are rejected. In conjunction with the tap processing delay
(above), this is implemented by checking for DOM elements modified as long ago
as the tap processing was delayed.

The list of modified DOM elements is cleared of records older than the 800ms tap
delay every time a mutation occurs, but not more frequently than the tap delay.

## Views

The Touch-to-Search UI consists of the following views, which appear above the
active tab:

* The **panel** (`ContextualSearchPanelView`), which contains the other views
and moves up and down in response to user gestures.
* The **header** (`ContextualSearchHeaderView`), which displays the tapped
word and its context, and other UI elements.
* The **search tab**, a specially-configured Tab which is displayed inside the
panel, containing the results of the tap-to-search query. 

### Panel positions

The panel can be in several different positions relative to the current tab.
The panel's horizontal position never varies; it is always the width of the tab.
Vertically, the panel can be in any of four position. These are named
differently on iOS and Android; Android names are in parenthesis. (At some point
iOS will standardize on the Android names). 

Position | Description
--------------------------| -------
`DISMISSED` (`CLOSED`)    | Panel is offscreen, just below the current tab,
`PEEKING` (`PEEKED`)      | Panel is showing only the header at the bottom of the tab.
`PREVIEWING` (`EXPANDED`) | Panel covers 2/3 of the tab, showing most of the search results.
`COVERING` (`MAXIMIZED`)  | Panel covers the entire tab, so the header overlaps the toolbar.

### Header

The header is the main piece of UI for touch-to-search. It shows the currently
active search term, indicates the progress of the search term resolution, shows
the "Google G" for brand identification, and shows a control that hints as to
how to expand/dismiss the panel. 

## Animations

### Durations

Animations use standard material animation durations:

Animation | Duration
---------------------------------| -------:
Panel movement (non-dismissing)  | *0.35s*
Panel dismissal                  | *0.25s*
Text and other transitions       | *0.25s*


### Panel position changes

#### `PEEKING` to `PREVIEWING`

As the panel moves from `PEEKING` to `PREVIEWING` (whether dragged or animated),
the following animations occur:

1. The header control rotates clockwise from pointing upwards to pointing 
downwards.
2. The current tab's web page content is shaded with (approximately) 66% opaque
black.

#### `PREVIEWING` to `COVERING`

### Search term resolution and loading

#### Touch {#anim-search-touch}

When touch-to-search is initiated (assuming the panel is `DISMISSED`):

1. The panel moves to `PEEKING` (*0.35s*). The header is in the "initial" state.
2. The tapped word is highlighted. This is not animated.
3. After the panel motion completes, the header text and control fade in 
(*0.25s*).
4. After the fade-in is complete, the G-logo irises in (*0.25s*).

(Total animation time: *0.85s*)

These animations aren't preempted even if resolution occurs after they complete.

##### Implementation notes.

The G-logo iris effect is implemented by adding a circular alpha channel layer
as a mask to the G-logo image view; this mask layer is initially scaled to be
very small, and has its transform animated to scale up and reveal the logo.

#### Dispatch

There is currently no animation during dispatch.

#### Resolution

When resolution occurs:

1. If possible, the highlighted text on the web page is expanded to cover the
resolved search term.
2. After any initial (see [Touch](#anim-search-touch), above) animations
complete, the context text views fade, the query text cross-fades to show the
resolved text, and (*not implemented*) the resolved text moves to be 
leading-aligned and to the leading side of the header (*0.25s*).
3. (*Not implemented*) *1.0s* after the text transitions complete, the resolved 
text moves upwards and the search results caption text fades in underneath. 

### Promotion

## See also

