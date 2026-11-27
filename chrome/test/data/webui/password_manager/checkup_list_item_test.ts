// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://password-manager/password_manager.js';

import {OpenWindowProxyImpl, PasswordManagerImpl} from 'chrome://password-manager/password_manager.js';
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {TestOpenWindowProxy} from 'chrome://webui-test/test_open_window_proxy.js';
import {microtasksFinished} from 'chrome://webui-test/test_util.js';

import {TestPasswordManagerProxy} from './test_password_manager_proxy.js';
import {createCredentialGroup, makeInsecureCredential} from './test_util.js';

suite('CheckupListItemTest', function() {
  test('uses the manual change link for previously supported automatic changes', async function() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    const passwordManager = new TestPasswordManagerProxy();
    PasswordManagerImpl.setInstance(passwordManager);
    const openWindow = new TestOpenWindowProxy();
    OpenWindowProxyImpl.setInstance(openWindow);
    const item = makeInsecureCredential({
      id: 42, url: 'test.com', username: 'viking',
      isAutomaticPasswordChangeSupported: true,
    });
    const element = document.createElement('checkup-list-item');
    element.item = item;
    element.group = createCredentialGroup({name: 'test.com', credentials: [item]});
    document.body.appendChild(element);
    await microtasksFinished();

    const manual = element.shadowRoot.querySelector<HTMLElement>('#changePasswordButton');
    assertTrue(!!manual);
    assertFalse(!!element.shadowRoot.querySelector('#autoChangePasswordButton'));
    manual.click();
    assertEquals(item.changePasswordUrl, await openWindow.whenCalled('openUrl'));
  });
});
