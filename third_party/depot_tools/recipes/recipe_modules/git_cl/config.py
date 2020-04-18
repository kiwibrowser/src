# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import types

from recipe_engine.config import config_item_context, ConfigGroup, BadConf
from recipe_engine.config import Single
from recipe_engine.config_types import Path

def BaseConfig(**_kwargs):
  return ConfigGroup(
    repo_location=Single(Path)
  )

config_ctx = config_item_context(BaseConfig)

@config_ctx()
def basic(c):
  pass


