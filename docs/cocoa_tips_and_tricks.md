# Cocoa Tips and Tricks

A collection of idioms that we use when writing the Cocoa views and controllers
for Chromium.

[TOC]

## NSWindowController Initialization

To make sure that |window| and |delegate| are wired up correctly in your xib,
it's useful to add this to your window controller:

```objective-c
- (void)awakeFromNib {
  DCHECK([self window]);
  DCHECK_EQ(self, [[self window] delegate]);
}
```

## NSWindowController Cleanup

"You want the window controller to release itself it |-windowDidClose:|, because
else it could die while its views are still around. if it (auto)releases itself
in the callback, the window and its views are already gone and they won't send
messages to the released controller."
- Nico Weber (thakis@)

See
[Window Closing Behavior, ADC Reference](http://developer.apple.com/mac/library/documentation/Cocoa/Conceptual/Documents/Concepts/WindowClosingBehav.html#//apple_ref/doc/uid/20000027)
for the full story.

What this means in practice is:

```objective-c
@interface MyWindowController : NSWindowController<NSWindowDelegate> {
  IBOutlet NSButton* closeButton_;
}
- (IBAction)closeButton:(id)sender;
@end

@implementation MyWindowController
- (id)init {
  if ((self = [super initWithWindowNibName:@"MyWindow" ofType:@"nib"])) {
  }
  return self;
}

- (void)awakeFromNib {
  // Check that we set the window and its delegate in the XIB.
  DCHECK([self window]);
  DCHECK_EQ(self, [[self window] delegate]);
}

// NSWindowDelegate notification.
- (void)windowWillClose:(NSNotification*)notif {
  [self autorelease];
}

// Action for a button that lets the user close the window.
- (IBAction)closeButton:(id)sender {
  // We clean ourselves up after the window has closed.
  [self close];
}
@end
```

## Unit Tests

There are four Chromium-specific GTest macros for writing ObjC++ test cases.
These macros are `EXPECT_NSEQ`, `EXPECT_NSNE`, and `ASSERT` variants by the same
names.  These test `-[id<NSObject> isEqual:]` and will print the object's
`-description` in GTest-style if the assertion fails. These macros are defined
in `//testing/gtest_mac.h`. Just include that file and you can start using them.

This allows you to write this:

```objective-c
EXPECT_NSEQ(@"foo", aString);
```

Instead of this:

```objective-c
EXPECT_TRUE([aString isEqualToString:@"foo"]);
```
