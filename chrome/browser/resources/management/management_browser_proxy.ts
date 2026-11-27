// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export interface Application {
  name: string;
  icon?: string;
  permissions: string[];
}

export interface Extension {
  name: string;
  icon?: string;
  permissions: string[];
}

export enum ReportingType {
  SECURITY = 'security',
  DEVICE = 'device',
  USER = 'user',
  USER_ACTIVITY = 'user-activity',
  EXTENSIONS = 'extensions',
  LEGACY_TECH = 'legacy-tech',
  URL = 'url',
}

export interface BrowserReportingResponse {
  messageId: string;
  reportingType: ReportingType;
}

interface ManagedDataResponse {
  applicationReportingSubtitle: string;
  browserManagementNotice: string;
  extensionReportingSubtitle: string;
  managedWebsitesSubtitle: string;
  pageSubtitle: string;
  managed: boolean;
  overview: string;
  customerLogo: string;
  threatProtectionDescription: string;
  showUpdateRequiredEol: boolean;
  eolMessage: string;
  eolAdminMessage: string;
  showMonitoredNetworkPrivacyDisclosure: boolean;
  showWindowsNoticeForDeskSync: boolean;
  showCookiesNoticeForDeskSync: boolean;
}

interface ThreatProtectionPermission {
  title: string;
  permission: string;
}

export interface ThreatProtectionInfo {
  info: ThreatProtectionPermission[];
  description: string;
  showSecureGatewayDisclosure?: boolean;
}


/** @interface */
export interface ManagementBrowserProxy {
  getExtensions(): Promise<Extension[]>;

  getManagedWebsites(): Promise<string[]>;

  getApplications(): Promise<Application[]>;


  getContextualManagedData(): Promise<ManagedDataResponse>;

  getThreatProtectionInfo(): Promise<ThreatProtectionInfo>;

  /**
   * @return The list of browser reporting info messages.
   */
  initBrowserReportingInfo(): Promise<BrowserReportingResponse[]>;

  /**
   * @return The list of profile reporting info messages.
   */
  initProfileReportingInfo(): Promise<BrowserReportingResponse[]>;

  /**
   * @return Whether the promotion banner should be shown.
   */
  shouldShowPromotion(): Promise<boolean>;

  setBannerDismissed(): Promise<void>;

  recordBannerRedirected(): Promise<void>;
}

export class ManagementBrowserProxyImpl implements ManagementBrowserProxy {
  getExtensions() {
    return sendWithPromise<Extension[]>('getExtensions');
  }

  getManagedWebsites() {
    return sendWithPromise<string[]>('getManagedWebsites');
  }

  getApplications() {
    return sendWithPromise<Application[]>('getApplications');
  }


  getContextualManagedData() {
    return sendWithPromise<ManagedDataResponse>('getContextualManagedData');
  }

  getThreatProtectionInfo() {
    return sendWithPromise<ThreatProtectionInfo>('getThreatProtectionInfo');
  }

  initBrowserReportingInfo() {
    return sendWithPromise<BrowserReportingResponse[]>(
        'initBrowserReportingInfo');
  }

  initProfileReportingInfo() {
    return sendWithPromise<BrowserReportingResponse[]>(
        'initProfileReportingInfo');
  }

  shouldShowPromotion() {
    return sendWithPromise<boolean>('shouldShowPromotion');
  }

  setBannerDismissed() {
    return sendWithPromise<void>('setBannerDismissed');
  }

  recordBannerRedirected() {
    return sendWithPromise<void>('recordBannerRedirected');
  }

  static getInstance(): ManagementBrowserProxy {
    return instance || (instance = new ManagementBrowserProxyImpl());
  }
}

let instance: ManagementBrowserProxy|null = null;

declare global {
  interface Window {
    ManagementBrowserProxyImpl: typeof ManagementBrowserProxyImpl;
  }
}

// Export |ManagementBrowserProxyImpl| on |window| so that it can be accessed by
// management_ui_browsertest.cc
window.ManagementBrowserProxyImpl = ManagementBrowserProxyImpl;
