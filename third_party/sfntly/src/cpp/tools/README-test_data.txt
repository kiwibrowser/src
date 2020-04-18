To generate test data:
In the current directory (tools/):

1. XML test data, run:
   $ python generate_test_data_xml.py

2. C++ test data (.h file containing the list of fonts to be tested), run:
   $ python generate_file_list_cc.py
   (after having generated the xml data)

By default the XML files are put in the same folder as the font they were
created from and the CMap test data is put in $PWD.

To get the list of paramters for both scripts run them with the --help option.

When adding new fonts to data/fonts/ (possibly by cloning the Google Web Fonts
directory), run the cleanup script:
    $ python clean_fonts_repo.py
to delete all files that are not {.ttf, .ttc, .otf, OFL.txt} and all files that
are more than 1 level deep from a font's folder.
(e.g. for copse/ it will delete all subfolders of copse/ and should only leave
the font and the license file untouched - and possibly an empty src folder;
however since empty folders are not added to git, they will be discarded when
committing the new fonts).
