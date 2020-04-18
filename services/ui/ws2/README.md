This directory contains the code for building a Window Service
implementation on top of an existing Aura hierarchy.

Each client is managed by an instance of WindowServiceClient. In this
directory, a client generally means a unique connection to the WindowService.
More specifically a client is an implementation of mojom::WindowTreeClient.
WindowServiceClient implements the mojom::WindowTree implementation that is
passed to the client. WindowServiceClient creates a ClientRoot for the window
the client is embedded in, as well as a ClientRoot for all top-level
window requests.

Clients establish a connection to the WindowService by configuring Aura with a
mode of MUS. See aura::Env::Mode for details.

The WindowService provides a way for one client to embed another client in a
specific window (application composition). Embedding establishes a connection
to a new client and provides the embedded client with a window to use. See the
mojom for more details.

For example, on Chrome OS, Ash uses the WindowService to enable separate
processes, such as the tap_visualizer, to connect to the WindowService. The
tap_visualizer is a client of the WindowService. The tap_visualizer uses the
WindowService to create and manage windows, receive events, and ultimately
draw to the screen (using Viz). This is mostly seamless to the tap_visualizer.
The tap_visualizer configures Views to use Mus, which results in Views and Aura,
using the WindowService.
