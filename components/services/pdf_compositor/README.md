The pdf_compositor service should composite multiple raw pictures from different
frames into a complete one, then converts it into a pdf file within an isolated
sandboxed process. Currently, it has no compositing functionality, just convert
a set of raw pictures into a pdf file within the sandboxed process.
