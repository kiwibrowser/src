# Browser View Resizer

To fix bug [458](http://code.google.com/p/chromium/issues/detail?id=458), which
identifies that it is hard to hit the thin window frame corner to resize the
window. It would be better to have a resize hit area (called widget from now on)
in the corner, as we currently have for edit boxes for example.

[TOC]

## Background

This is specific to the Windows OS. On the Mac, Cocoa automatically adds a
resize widget (Not sure about Linux, we should double check). On Windows, those
resize widgets are at the extreme right of a status bar. For example, if you
remove the status bar from a Windows Explorer window, you lose the resize
widget. But since Chrome never ever has a status bar and simply take over the
bottom of the window for specific tasks (like the download shelf for example),
we need to find a creative way of giving access to a resize widget.

The bottom corners where we would like to add the resize widget are currently
controlled by the browser view, which can have either the tab contents view or
other dynamic views (like the download shelf view) displayed in this area.

## Requirements

Since there is no status bar to simply fix a resize widget to, we must
dynamically create a widget that can be laid either on the tab contents view or
on other views that might temporarily take over the bottom part of the browser
view.

When no dynamic view is taking over the bottom of the browser view, the resize
widget can sit in the bottom right corner of the tab contents view, over the tab
contents view.

![Resize Corner](http://lh6.ggpht.com/_2OD0ww7UZAs/SUAaNi6TWYI/AAAAAAAAGmI/89jCYQ1Cxsw/ResizeCorner-2.png)

The resize widget must have the same width and height as
the scroll bars so that it can fit in the corner currently left empty when both
scroll bars are visible. If only one scroll bar is visible (either the
horizontal or the vertical one), that scroll bar must still leave room for the
resize widget to fit there (as it currently leave room for the empty corner when
both scroll bars are visible), yet, only when the resize widget is laid on top
of the tab contents view, not when a dynamic shelf is added at the bottom of the
browser view.

![Resize Corner](http://lh6.ggpht.com/_2OD0ww7UZAs/SUAaNjqr_iI/AAAAAAAAGmA/56hzjdnkVRI/ResizeCorner-1.png)
![Resize Corner](http://lh3.ggpht.com/_2OD0ww7UZAs/SUAaN_wDEUI/AAAAAAAAGmQ/7B4CTZTXOmk/ResizeCorner-3.png)
![Resize Corner](http://lh6.ggpht.com/_2OD0ww7UZAs/SUAaN7yme9I/AAAAAAAAGmY/EaniiAbwi-Q/ResizeCorner-4.png)

If another view (e.g.,  again, the download shelf) is added at the bottom of the
browser view, below the tab contents view, and covers the bottom corners, then
the resize widget must be laid on top of this other child view. Of course, all
child views that can potentially be added at the bottom of the browser view,
must be designed in a way that leaves enough room in the bottom corners for the
resize widget.

![Resize Corner](http://lh3.ggpht.com/_2OD0ww7UZAs/SUAaN17TIrI/AAAAAAAAGmg/6bljNQ_vZkI/ResizeCorner-5.png)
![Resize Corner](http://lh4.ggpht.com/_2OD0ww7UZAs/SUAaWINHA6I/AAAAAAAAGmo/-VG5FGC8Xds/ResizeCorner-6.png)
![Resize Corner](http://lh6.ggpht.com/_2OD0ww7UZAs/SUAaWDUpo0I/AAAAAAAAGmw/8USPzoMpgu0/ResizeCorner-7.png)

Since the bottom corners might have different colors, based on the state and
content of the browser view, the resize widget must have a transparent
background.

The resize widget is not animated itself. It might move with the animation of
the view it is laid on top of (e.g., when the download shelf is being animated
in), but we won't attempt to animate the resize widget itself (or fix it in the
bottom right corner of the browser view while the other views get animated it).

## Design

Unfortunately, we must deal with the two different cases (with or without a
dynamic bottom view) in two different and distinct ways.

### Over a Dynamic View

For the cases where there is a dynamic view at the bottom of the browser view, a
new view class (named `BrowserResizerView`) inheriting from
[views::View](https://src.chromium.org/svn/trunk/src/chrome/views/view.h) is used
to display the resize widget. It is set as a child of the dynamic view laid at
the bottom of the browser view. The Browser view takes care of properly setting
the bounds of the resize widget view, based on the language direction.

Also, it is easier and more efficient to let the browser view handle the mouse
interactions to resize the browser. We can let Windows take care of properly
resizing the view by returning the HTBOTTOMLEFT or HTBOTTOMRIGHT flags from the
NCClientHitTest windows message handler when they occur over the resize widget.
The browser view also takes care of changing the mouse cursor to the appropriate
resizing arrows when the mouse hovers over the resize widget area.

### Without a Dynamic View

To make sure that the scroll bars (handled by `WebKit`) are not drawn on top of
the resizer widget (or vice versa), we need to properly implement the callback
specifying the rectangle covered by the resizer. This callback is implemented
on the `RenderWidget` class that can delegate to a derive class via a new
virtual method which returns an empty rect on the base class. Via a series of
delegate interface calls, we eventually get back to the browser view which can
return the size and position of the resize widget, but only if it is laid out on
top of the tabs view, it returns an empty rect when there is a dynamic view.

To handle the drawing of the resize widget over the render widget, we need to
add code to the Windows specific version of the render widget host view which
receives the bitmap rendered by WebKit so it can layer the transparent bitmap
used for the resize widget. That same render widget host view must also handle
the mouse interaction and use the same trick as the browser view to let Windows
take care of resizing the whole frame. It must also take care of changing the
mouse cursor to the appropriate resizing arrows when the mouse hovers over the
resize widget area.

## Implementation

You can find the changes made to make this work in patch
[16488](https://codereview.chromium.org/16488).

## Alternatives Considered

We could have tried to reuse the code that currently takes care of resizing the
edit boxes within WebKit, but this code is wired to the overflow style of HTML
element and would have been hard to rewire in an elegant way to be used in a
higher level object like the browser view. Unless we missed something.

We might also decide to go with the easier solution of only showing the resize
corner within the tab contents view. In that case, it would still be recommended
that the resize widget would not appear when dynamic views are taking over the
bottom portion of the browser view, since it would look weird to have a resize
corner widget that is not in the real... corner... of the browser view ;-)

We may decide that we don't want to see the resize widget bitmap hide some
pixels from the tab contents (or dynamic view) yet we would still have the
resizing functionality via the mouse interaction and also get visual feedback
with the mouse cursor changes while we hover over the resize widget area.

We may do more research to find a way to solve this problem in a single place as
opposed to the current dual solution, but none was found so far.
