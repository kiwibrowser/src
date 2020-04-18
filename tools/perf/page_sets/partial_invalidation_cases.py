# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry import story

# Even though we don't specialize page behavior directly, we have to define our
# own page class so that |story.serving_dir| fetched from the StorySet will
# reflect the path to our containing directory, under which we serve local files
# for recording. See also initialization of |base_dir| in telemetry's page.Page
# constructor, which is in turn referenced by |file_path_url|.
class PartialInvalidationCasesPage(page_module.Page):

  def __init__(self, url, page_set):
    super(PartialInvalidationCasesPage, self).__init__(
        url=url, page_set=page_set, name=url.split('/')[-1])


class PartialInvalidationCasesPageSet(story.StorySet):

  """ Page set consisting of pages specialized for partial invalidation,
    for example, pages with many elements. """

  def __init__(self):
    super(PartialInvalidationCasesPageSet, self).__init__(
        cloud_storage_bucket=story.PARTNER_BUCKET)

    other_urls = [
        # Why: Reduced test case similar to the single page html5 spec wherein
        # we saw a performance regression demonstrable via a small partial
        # invalidation.
        'file://partial_invalidation_cases/800_relpos_divs.html',
    ]

    for url in other_urls:
      self.AddStory(PartialInvalidationCasesPage(url, self))
