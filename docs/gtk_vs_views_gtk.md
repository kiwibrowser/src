# Gtk vs ViewsGtk

## Benefits of ViewsGtk

*   Better code sharing. For example, don't have to duplicate tab layout or
    bookmark bar layout code.
    *   Tab Strip
        *   Drawing
        *   All the animationy bits
        *   Subtle click selection behavior (curved corners)
        *   Drag behavior, including dropping of files onto the URL bar
        *   Closing behavior
    *   Bookmarks bar
        *   drag & drop behavior, including menus
        *   chevron?
*   Easier for folks to work on both platforms without knowing much about the
    underlying toolkits.
*   Don't have to implement ui features twice.

## Benefits of Gtk

*   Dialogs
    *   Native feel layout
    *   Font size changes (e.g., changing the system font size will apply to our
        dialogs)
    *   Better RTL (e.g., https://crbug.com/2822 https://crbug.com/5729
        https://crbug.com/6082 https://crbug.com/6103 https://crbug.com/6125
        https://crbug.com/8686 https://crbug.com/8649)
*   Being able to obey the user's system theme
*   Accessibility for buttons and dialogs (but not for tabstrip and bookmarks)
*   A better change at good remote X performance?
*   We still would currently need Pango / Cairo for text layout, so it will be
    more efficient to just draw that during the Gtk pipeline instead of with
    Skia.
*   Gtk widgets will automatically "feel and behave" like Linux. The behavior of
    our own Views system does not necessarily feel right on Linux.
*   People working on Windows features don't need to worry about breaking the
    Linux build.
