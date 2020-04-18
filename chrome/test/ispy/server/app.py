# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import webapp2

sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir))
import debug_view_handler
import image_handler
import main_view_handler
import rebaseline_handler
import update_mask_handler


application = webapp2.WSGIApplication(
    [('/update_mask', update_mask_handler.UpdateMaskHandler),
     ('/rebaseline', rebaseline_handler.RebaselineHandler),
     ('/debug_view', debug_view_handler.DebugViewHandler),
     ('/image', image_handler.ImageHandler),
     ('/', main_view_handler.MainViewHandler)], debug=True)
