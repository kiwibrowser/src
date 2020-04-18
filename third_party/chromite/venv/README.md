chromite virtualenv README
==========================

chromite is currently transitioning to a virtualenv setup.

There are currently two approaches to virtualenv in chromite:
`virtualenv_wrapper.py` and full virtualenv.

`virtualenv_wrapper.py` uses a wrapper in `scripts`, similar to the
original `wrapper.py`.  It uses `requirements.txt` to install third
party dependencies, but still relies on `wrapper.py`'s logic for
finding the Python script to run from the wrapper symlink and also
relies on the logic for importing chromite.  This uses
`requirements.txt` and creates the virtualenv `.venv`.

Full virtualenv eschews `wrapper.py` entirely, installing chromite
itself into the virtualenv and running scripts by their import path.
This uses `full_requirements.txt` and creates the virtualenv
`.full_venv`.
