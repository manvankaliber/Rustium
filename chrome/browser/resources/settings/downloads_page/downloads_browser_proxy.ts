// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


export interface DownloadsBrowserProxy {
  initializeDownloads(): void;


  selectDownloadLocation(): void;

  resetAutoOpenFileTypes(): void;

}

export class DownloadsBrowserProxyImpl implements DownloadsBrowserProxy {
  initializeDownloads() {
    chrome.send('initializeDownloads');
  }

  selectDownloadLocation() {
    chrome.send('selectDownloadLocation');
  }

  resetAutoOpenFileTypes() {
    chrome.send('resetAutoOpenFileTypes');
  }


  static getInstance(): DownloadsBrowserProxy {
    return instance || (instance = new DownloadsBrowserProxyImpl());
  }

  static setInstance(obj: DownloadsBrowserProxy) {
    instance = obj;
  }
}

let instance: DownloadsBrowserProxy|null = null;
