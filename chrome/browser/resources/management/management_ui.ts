// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/cr_toolbar/cr_toolbar.js';
import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/icons.html.js';
import './icons.html.js';
import './promotion_banner.js';
import '/strings.m.js';

import {ColorChangeUpdater, COLORS_CSS_SELECTOR} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';
import {I18nMixinLit} from 'chrome://resources/cr_elements/i18n_mixin_lit.js';
import {WebUiListenerMixinLit} from 'chrome://resources/cr_elements/web_ui_listener_mixin_lit.js';
import {assert} from 'chrome://resources/js/assert.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {sanitizeInnerHtml} from 'chrome://resources/js/parse_html_subset.js';
import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';

import type {Application, BrowserReportingResponse, Extension, ManagementBrowserProxy, ThreatProtectionInfo} from './management_browser_proxy.js';
import { ManagementBrowserProxyImpl, ReportingType} from './management_browser_proxy.js';
import {getCss} from './management_ui.css.js';
import {getHtml} from './management_ui.html.js';
// clang-format on

interface BrowserReportingData {
  messageIds: string[];
  icon?: string;
}

const ManagementUiElementBase =
    WebUiListenerMixinLit(I18nMixinLit(CrLitElement));

export class ManagementUiElement extends ManagementUiElementBase {
  static get is() {
    return 'management-ui';
  }

  static override get styles() {
    return getCss();
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      /**
       * List of messages related to application reporting.
       */
      applications_: {type: Array},

      /**
       * Title of subsection for application reporting.
       */
      applicationReportingSubtitle_: {type: String},

      /**
       * List of messages related to browser reporting.
       */
      browserReportingInfo_: {type: Array},

      /**
       * List of messages related to profile reporting.
       */
      profileReportingInfo_: {type: Array},

      /**
       * List of messages related to extension reporting.
       */
      extensions_: {type: Array},

      /**
       * Title of subsection for extension reporting.
       */
      extensionReportingSubtitle_: {type: String},

      /**
       * List of messages related to managed websites reporting.
       */
      managedWebsites_: {type: Array},

      managedWebsitesSubtitle_: {type: String},

      /**
       * Whether the promotion banner should be shown.
       */
      shouldShowPromotion_: {type: Boolean},


      subtitle_: {type: String},

      managementNoticeHtml_: {type: String},

      managed_: {type: Boolean},
      threatProtectionInfo_: {type: Object},
    };
  }

  protected accessor applications_: Application[]|null = null;
  protected accessor browserReportingInfo_: BrowserReportingData[]|null = null;
  protected accessor profileReportingInfo_: BrowserReportingData[]|null = null;
  protected accessor extensions_: Extension[]|null = null;
  protected accessor managedWebsites_: string[]|null = null;
  protected accessor managedWebsitesSubtitle_: string = '';
  protected accessor shouldShowPromotion_: boolean = false;


  protected accessor subtitle_: string = '';

  protected accessor managementNoticeHtml_: TrustedHTML =
      window.trustedTypes!.emptyHTML;

  protected accessor managed_: boolean = false;
  protected accessor applicationReportingSubtitle_: string = '';
  protected accessor extensionReportingSubtitle_: string = '';
  protected accessor threatProtectionInfo_: ThreatProtectionInfo|null = null;
  private browserProxy_: ManagementBrowserProxy =
      ManagementBrowserProxyImpl.getInstance();

  /** @override */
  override connectedCallback() {
    super.connectedCallback();

    document.documentElement.classList.remove('loading');
    this.updateManagedFields_();
    this.initReportingInfo_();
    this.getThreatProtectionInfo_();

    const enableWebuiRefresh2026 =
        loadTimeData.getString('webuiRefresh2026') !== '';
    if (enableWebuiRefresh2026) {
      this.addThemedColors_();
      ColorChangeUpdater.forDocument().start();
    }

    this.addWebUiListener(
        'browser-reporting-info-updated',
        (reportingInfo: BrowserReportingResponse[]) =>
            this.onBrowserReportingInfoReceived_(reportingInfo));

    this.addWebUiListener(
        'profile-reporting-info-updated',
        (reportingInfo: BrowserReportingResponse[]) =>
            this.onProfileReportingInfoReceived_(reportingInfo));

    this.addWebUiListener('managed_data_changed', () => {
      this.updateManagedFields_();
    });

    this.addWebUiListener(
        'threat-protection-info-updated',
        (info: ThreatProtectionInfo) => this.threatProtectionInfo_ = info);

    this.getExtensions_();
    this.getManagedWebsites_();
    this.getApplications_();
    // Assign the promise result directly to the property
    this.browserProxy_.shouldShowPromotion().then(shouldShowPromotion => {
      this.shouldShowPromotion_ = shouldShowPromotion;
    });
  }

  private initReportingInfo_() {
    this.browserProxy_.initBrowserReportingInfo().then(
        reportingInfo => this.onBrowserReportingInfoReceived_(reportingInfo));
    this.browserProxy_.initProfileReportingInfo().then(
        reportingInfo => this.onProfileReportingInfoReceived_(reportingInfo));
  }

  private onBrowserReportingInfoReceived_(
      reportingInfo: BrowserReportingResponse[]) {
    const reportingInfoMap = reportingInfo.reduce((info, response) => {
      info[response.reportingType] = info[response.reportingType] || {
        icon: this.getIconForReportingType_(response.reportingType),
        messageIds: [],
      };
      info[response.reportingType]!.messageIds.push(response.messageId);
      return info;
    }, {} as {[k: string]: {icon: string, messageIds: string[]}});

    const reportingTypeOrder: {[k: string]: number} = {
      [ReportingType.URL]: 1,
      [ReportingType.SECURITY]: 2,
      [ReportingType.EXTENSIONS]: 3,
      [ReportingType.USER]: 4,
      [ReportingType.USER_ACTIVITY]: 5,
      [ReportingType.DEVICE]: 6,
      [ReportingType.LEGACY_TECH]: 7,
    };

    this.browserReportingInfo_ =
        Object.keys(reportingInfoMap)
            .sort((a, b) => reportingTypeOrder[a]! - reportingTypeOrder[b]!)
            .map(reportingType => reportingInfoMap[reportingType]) as
        BrowserReportingData[];
  }


  private onProfileReportingInfoReceived_(
      reportingInfo: BrowserReportingResponse[]) {
    this.profileReportingInfo_ =
        reportingInfo.map((info) => ({
                            messageIds: [info.messageId],
                          }));
  }
  private getExtensions_() {
    this.browserProxy_.getExtensions().then(extensions => {
      this.extensions_ = extensions;
    });
  }

  private getManagedWebsites_() {
    this.browserProxy_.getManagedWebsites().then(managedWebsites => {
      this.managedWebsites_ = managedWebsites;
    });
  }

  private getApplications_() {
    this.browserProxy_.getApplications().then(applications => {
      this.applications_ = applications;
    });
  }

  private getThreatProtectionInfo_() {
    this.browserProxy_.getThreatProtectionInfo().then(info => {
      this.threatProtectionInfo_ = info;
    });
  }

  /**
   * @return Whether there is threat protection info to show.
   */
  protected showThreatProtectionInfo_(): boolean {
    return !!this.threatProtectionInfo_ &&
        (this.threatProtectionInfo_.info.length > 0 ||
         !!this.threatProtectionInfo_.showSecureGatewayDisclosure);
  }


  /**
   * @return Whether there are browser reporting info to show.
   */
  protected showBrowserReportingInfo_(): boolean {
    return !!this.browserReportingInfo_ &&
        this.browserReportingInfo_.length > 0;
  }

  /**
   * @return Whether there are profile reporting info to show with new format.
   */
  protected showProfileReportingInfo_(): boolean {
    return !!this.profileReportingInfo_ &&
        this.profileReportingInfo_.length > 0;
  }


  /**
   * @return Whether there are extension reporting info to show.
   */
  protected showExtensionReportingInfo_(): boolean {
    return !!this.extensions_ && this.extensions_.length > 0;
  }

  /**
   * @return Whether there are application reporting info to show.
   */
  protected showApplicationReportingInfo_(): boolean {
    return !!this.applications_ && this.applications_.length > 0;
  }

  /**
   * @return Whether there is managed websites info to show.
   */
  protected showManagedWebsitesInfo_(): boolean {
    return !!this.managedWebsites_ && this.managedWebsites_.length > 0;
  }

  /**
   * @return The associated icon.
   */
  private getIconForReportingType_(reportingType: ReportingType): string {
    switch (reportingType) {
      case ReportingType.SECURITY:
        return 'cr:security';
      case ReportingType.DEVICE:
        return 'cr:computer';
      case ReportingType.EXTENSIONS:
        return 'cr:chrome-extension-filled';
      case ReportingType.USER:
        return loadTimeData.getBoolean('webuiRoundedIconsEnabled') ?
            'management:account-circle-filled' :
            'management:account-circle-old';
      case ReportingType.USER_ACTIVITY:
        return loadTimeData.getBoolean('webuiRoundedIconsEnabled') ?
            'management:public' :
            'management:public-old';
      case ReportingType.LEGACY_TECH:
        return loadTimeData.getBoolean('webuiRoundedIconsEnabled') ?
            'management:language' :
            'management:legacy-tech-old';
      case ReportingType.URL:
        return loadTimeData.getBoolean('webuiRoundedIconsEnabled') ?
            'management:link' :
            'management:link-old';
      default:
        return 'cr:security';
    }
  }

  /**
   * Handles the 'search-changed' event fired from the toolbar.
   * Redirects to the settings page initialized the the current
   * search query.
   */
  protected onSearchChanged_(e: CustomEvent<string>) {
    const query = e.detail;
    window.location.href =
        `chrome://settings?search=${encodeURIComponent(query)}`;
  }

  protected onBackClick_() {
    if (history.length > 1) {
      history.back();
    } else {
      window.location.href = 'chrome://settings/help';
    }
  }

  protected onPromotionDismissed_() {
    this.shouldShowPromotion_ = false;
  }

  private updateManagedFields_() {
    this.browserProxy_.getContextualManagedData().then(data => {
      this.managed_ = data.managed;
      this.extensionReportingSubtitle_ = data.extensionReportingSubtitle;
      this.managedWebsitesSubtitle_ = data.managedWebsitesSubtitle;
      this.applicationReportingSubtitle_ = data.applicationReportingSubtitle;
      this.subtitle_ = data.pageSubtitle;
      this.managementNoticeHtml_ = sanitizeInnerHtml(
          data.browserManagementNotice, {attrs: ['aria-label']});
    });
  }

  private addThemedColors_() {
    assert(document.body.querySelector(COLORS_CSS_SELECTOR) === null);
    const link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = 'chrome://theme/colors.css?sets=ui,chrome';
    document.body.appendChild(link);
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'management-ui': ManagementUiElement;
  }
}

customElements.define(ManagementUiElement.is, ManagementUiElement);
