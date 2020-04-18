# Linux Debugging GTK

## Making warnings fatal

See
[Running GLib Applications](http://developer.gnome.org/glib/stable/glib-running.html)
for notes on how to make GTK warnings fatal.

## Using GTK Debug packages

    sudo apt-get install libgtk2.0-0-dbg

Make sure that you're building a binary that matches your architecture (e.g.
64-bit on a 64-bit machine), and there you go.

### Source

You'll likely want to get the source for gtk too so that you can step through
it. You can tell gdb that you've downloaded the source to your system's GTK by
doing:

```shell
$ cd /my/dir
$ apt-get source libgtk2.0-0
$ gdb ...
(gdb) set substitute-path /build/buildd /my/dir
```

NOTE: I tried debugging pango in a similar manner, but for some reason gdb
didn't pick up the symbols from the symbols from the `-dbg` package. I ended up
building from source and setting my `LD_LIBRARY_PATH`.

See [linux_building_debug_gtk.md](linux_building_debug_gtk.md) for more on how
to build your own debug version of GTK.

## Parasite

http://chipx86.github.com/gtkparasite/ is great. Go check out the site for more
about it.

Install it with

    sudo apt-get install gtkparasite

And then run Chrome with

    GTK_MODULES=gtkparasite ./out/Debug/chrome

### ghardy

If you're within the Google network on ghardy, which is too old to include
gtkparasite, you can do:

    scp bunny.sfo:/usr/lib/gtk-2.0/modules/libgtkparasite.so /tmp
    sudo cp /tmp/libgtkparasite.so /usr/lib/gtk-2.0/modules/libgtkparasite.so

## GDK_DEBUG

Use `GDK_DEBUG=nograbs` to run GTK+ without grabs. This is useful for gdb
sessions.
