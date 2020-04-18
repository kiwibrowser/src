# Views Platform Styling

## Overview

Views controls may have different appearances on different platforms, so that
Views UIs can fit better into the platform's native styling. This document
describes how to build Views UIs that will look good on all platforms with a
minimum of manual intervention.

UIs looking good happens at two levels: first, the individual controls must look
and act appropriately for their platform, and second, the overall layout of the
controls in a dialog or UI surface must match what users of the platform would
expect. There are differences at both of these layers between desktop platforms,
and mobile platforms have still more differences.

## Controls

Individual controls have different looks and behaviors on different platforms.
If you're adding a new control or a subclass of an existing control, there are
some best practices you should follow in designing it so that it works well
everywhere:

### Use PlatformStyle for stylistic elements

PlatformStyle exposes factory functions that produce different subclasses of
Border, Background, and so on that are appropriate to the current platform. If
your class needs a special kind of border or another stylistic element, creating
it through a factory function in PlatformStyle will make per-platform styling
for it easier, and will make which parts of the appearance are platform-specific
more apparent. For example, if you were adding a Foo control that had a special
FooBackground background, you might add a function to PlatformStyle:

    unique_ptr<FooBackground> CreateFooBackground();

and a default implementation in PlatformStyle. This way, in future a
platform-specific implementation can go in PlatformStyleBar and change the
background of that control on platform Bar without changing the implementation
of the Foo control at all.

### Use PlatformStyle to add simple behavior switches

When adding platform-specific behavior for an existing control, if possible, it
is useful to implement the switch using a const boolean exported from
PlatformStyle, instead of ifdefs inside the control's implementation. For
example, instead of:

    #if defined(OS_BAR)
    void Foo::DoThing() { ... }
    #else
    void Foo::DoThing() { ... }
    #endif

It's better to do this:

    Foo::Foo() : does_thing_that_way_(PlatformStyle::kFooDoesThingThatWay)

    void Foo::DoThing() {
        if (does_thing_that_way_)
            ...
        else
            ...
    }

This pattern makes it possible to unit-test all the different platform behaviors
on one platform.

### Use subclassing to add complex behavior switches

If a lot of the behavior of Foo needs to change per-platform, creating
platform-specific subclasses of Foo and a factory method on Foo that creates the
appropriate subclass for the platform is easier to read and understand than
having ifdefs or lots of control flow inside Foo to implement per-platform
behavior.

Note that it's best only to do this when no other alternative presents itself,
because having multiple subclasses to do different behaviors per-platform makes
subclassing a control require one subclass per platform as well. It's better to
abstract the per-platform behavior into a separate model class, with a factory
that produces the right model for the current platform.

## UI Layout / Controls

TODO(ellyjones): This section needs a bit more thought.

Some platforms have conventions about the ordering of buttons in dialogs, or the
presence or absence of certain common controls. For example, on Mac, dialogs are
expected to have their "default" button at the bottom right, and expected not to
have a "close" button in their top corner if they have a "Cancel"/"Dismiss"
button in the dialog body. If you can design a layout that follows all
platforms' conventions simultaneously, that is the lowest-effort route to
follow, but if not, there are static booleans in PlatformStyle that hold the
appropriate values for these decisions on the current platform, like:

    static const bool PlatformStyle::kDialogsShouldHaveCloseButton;

You can then condition your dialog creation code like this:

    if (PlatformStyle::kDialogsShouldHaveCloseButton)
        views::Button* close_button = ...;

TODO(ellyjones): Actually add these variables to PlatformStyle
