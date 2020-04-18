Those tests are almost ready for WPT, with some minor differences for them to
work properly on fast/canvas:

- worker.html template files can be removed (wpt tests auto-generate those)
- deferTest should automatically be added to the template?
- resources/ directory is not needed.
- there are 2 expected files for invalid font parsing that, if not solved,
  need to be removed.
