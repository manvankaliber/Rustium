// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://bookmarks-side-panel.top-chrome/power_bookmarks_labels.js';

import type {PowerBookmarksLabelsElement} from 'chrome://bookmarks-side-panel.top-chrome/power_bookmarks_labels.js';
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {microtasksFinished} from 'chrome://webui-test/test_util.js';

suite('SidePanelPowerBookmarksLabelsTest', () => {
  let element: PowerBookmarksLabelsElement;

  setup(() => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    element = document.createElement('power-bookmarks-labels');
    document.body.appendChild(element);
  });

  test('NoLabels', async () => {
    await microtasksFinished();
    assertEquals(0, element.labels.length);
    assertEquals(0, element.shadowRoot.querySelectorAll('cr-chip').length);
  });

  test('TogglesLabelAndRespectsDisabledState', async () => {
    element.labels = [{label: 'Example', icon: 'bookmarks:check', active: false}];
    await microtasksFinished();
    const chip = element.shadowRoot.querySelector('cr-chip')!;
    assertFalse(chip.disabled);
    chip.click();
    await microtasksFinished();
    assertTrue(element.labels[0]!.active);
    element.disabled = true;
    await microtasksFinished();
    assertTrue(chip.disabled);
  });
});
