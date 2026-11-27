// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface LifetimeBrowserProxy {
  // Triggers a browser restart.
  restart(): void;

  // Triggers a browser relaunch.
  relaunch(): void;

  // Indicates whether a relaunch confirmation dialog needs to be shown or not.
  shouldShowRelaunchConfirmationDialog(alwaysShowDialog: boolean):
      Promise<boolean>;

  // Returns the description of the relaunch confirmation dialog.
  // A null value can be returned if the condition to show the relaunch dialog
  // is no longer true.
  getRelaunchConfirmationDialogDescription(isVersionUpdate: boolean):
      Promise<string|null>;

}

export class LifetimeBrowserProxyImpl implements LifetimeBrowserProxy {
  restart() {
    chrome.send('restart');
  }

  relaunch() {
    chrome.send('relaunch');
  }

  shouldShowRelaunchConfirmationDialog(alwaysShowDialog: boolean) {
    return sendWithPromise<boolean>(
        'shouldShowRelaunchConfirmationDialog', alwaysShowDialog);
  }

  getRelaunchConfirmationDialogDescription(isVersionUpdate: boolean) {
    return sendWithPromise<string|null>(
        'getRelaunchConfirmationDialogDescription', isVersionUpdate);
  }


  static getInstance(): LifetimeBrowserProxy {
    return instance || (instance = new LifetimeBrowserProxyImpl());
  }

  static setInstance(obj: LifetimeBrowserProxy) {
    instance = obj;
  }
}

let instance: LifetimeBrowserProxy|null = null;
