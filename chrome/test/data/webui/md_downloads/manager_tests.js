// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('manager tests', function() {
  const DOWNLOAD_DATA_TEMPLATE = Object.freeze({
    by_ext_id: '',
    by_ext_name: '',
    danger_type: downloads.DangerType.NOT_DANGEROUS,
    date_string: '',
    file_externally_removed: false,
    file_path: '/some/file/path',
    file_name: 'download 1',
    file_url: 'file:///some/file/path',
    id: '',
    last_reason_text: '',
    otr: false,
    percent: 100,
    progress_status_text: '',
    resume: false,
    return: false,
    since_string: 'Today',
    started: Date.now() - 10000,
    state: downloads.States.COMPLETE,
    total: -1,
    url: 'http://permission.site',
  });

  /** @type {!downloads.Manager} */
  let manager;

  setup(function() {
    PolymerTest.clearBody();
    manager = document.createElement('downloads-manager');
    document.body.appendChild(manager);
    assertEquals(manager, downloads.Manager.get());
  });

  test('long URLs ellide', function() {
    downloads.Manager.insertItems(0, [{
                                    file_name: 'file name',
                                    state: downloads.States.COMPLETE,
                                    url: 'a'.repeat(1000),
                                  }]);
    Polymer.dom.flush();

    const item = manager.$$('downloads-item');
    assertLT(item.$$('#url').offsetWidth, item.offsetWidth);
    assertEquals(300, item.$$('#url').textContent.length);
  });

  test('inserting items at beginning render dates correctly', function() {
    const dateQuery = '* /deep/ h3[id=date]:not(:empty)';
    const countDates = () => manager.querySelectorAll(dateQuery).length;

    let download1 = Object.assign({}, DOWNLOAD_DATA_TEMPLATE);
    let download2 = Object.assign({}, DOWNLOAD_DATA_TEMPLATE);

    downloads.Manager.insertItems(0, [download1, download2]);
    Polymer.dom.flush();
    assertEquals(1, countDates());

    downloads.Manager.removeItem(0);
    Polymer.dom.flush();
    assertEquals(1, countDates());

    downloads.Manager.insertItems(0, [download1]);
    Polymer.dom.flush();
    assertEquals(1, countDates());
  });

  test('update', function() {
    let dangerousDownload = Object.assign({}, DOWNLOAD_DATA_TEMPLATE, {
      danger_type: downloads.DangerType.DANGEROUS_FILE,
      state: downloads.States.DANGEROUS,
    });
    downloads.Manager.insertItems(0, [dangerousDownload]);
    Polymer.dom.flush();
    assertTrue(!!manager.$$('downloads-item').$$('.dangerous'));

    let safeDownload = Object.assign({}, dangerousDownload, {
      danger_type: downloads.DangerType.NOT_DANGEROUS,
      state: downloads.States.COMPLETE,
    });
    downloads.Manager.updateItem(0, safeDownload);
    Polymer.dom.flush();
    assertFalse(!!manager.$$('downloads-item').$$('.dangerous'));
  });
});
