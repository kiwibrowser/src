import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))

import pkg_resources
pkg_resources.require('Paste')
