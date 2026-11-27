// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import type {LifetimeBrowserProxy} from 'chrome://settings/settings.js';
import {TestBrowserProxy} from 'chrome://webui-test/test_browser_proxy.js';

/**
 * A test version of LifetimeBrowserProxy.
 */
export class TestLifetimeBrowserProxy extends TestBrowserProxy implements
    LifetimeBrowserProxy {
  private shouldShowRelaunchDialog_: boolean = false;
  private relaunchConfirmationDialogDescription_: string|null = null;

  constructor() {
    super([
      'restart', 'relaunch',

      'shouldShowRelaunchDialog', 'getRelaunchConfirmationDialogDescription',

    ]);
  }

  restart() {
    this.methodCalled('restart');
  }

  relaunch() {
    this.methodCalled('relaunch');
  }

  shouldShowRelaunchConfirmationDialog(alwaysShowDialog: boolean) {
    this.methodCalled('shouldShowRelaunchDialog', alwaysShowDialog);
    return Promise.resolve(this.shouldShowRelaunchDialog_);
  }

  setShouldShowRelaunchConfirmationDialog(value: boolean) {
    this.shouldShowRelaunchDialog_ = value;
  }

  setRelaunchConfirmationDialogDescription(value: string) {
    this.relaunchConfirmationDialogDescription_ = value;
  }

  getRelaunchConfirmationDialogDescription(isVersionUpdate: boolean) {
    this.methodCalled(
        'getRelaunchConfirmationDialogDescription', isVersionUpdate);
    return Promise.resolve(this.relaunchConfirmationDialogDescription_);
  }

}
