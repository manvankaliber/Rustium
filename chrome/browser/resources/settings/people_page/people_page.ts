// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-people-page' is the settings page containing sign-in settings.
 */
import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js';
import 'chrome://resources/cr_elements/cr_toast/cr_toast.js';
import 'chrome://resources/cr_elements/icons.html.js';
import 'chrome://resources/cr_elements/policy/cr_policy_indicator.js';
import '../controls/settings_toggle_button.js';
import './sync_account_control.js';
import '../icons.html.js';
import '../settings_page/settings_section.js';

import type {ProfileInfo} from '/shared/settings/people_page/profile_info_browser_proxy.js';
import {ProfileInfoBrowserProxyImpl} from '/shared/settings/people_page/profile_info_browser_proxy.js';
import type {StoredAccount, SyncBrowserProxy, SyncStatus} from '/shared/settings/people_page/sync_browser_proxy.js';
import {SignedInState, StatusAction, SyncBrowserProxyImpl} from '/shared/settings/people_page/sync_browser_proxy.js';
import type {CrToastElement} from 'chrome://resources/cr_elements/cr_toast/cr_toast.js';
import {WebUiListenerMixinLit} from 'chrome://resources/cr_elements/web_ui_listener_mixin_lit.js';
import {assert, assertNotReached} from 'chrome://resources/js/assert.js';
import {focusWithoutInk} from 'chrome://resources/js/focus_without_ink.js';
import {getImage} from 'chrome://resources/js/icon.js';
import {OpenWindowProxyImpl} from 'chrome://resources/js/open_window_proxy.js';
import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';
import type {PropertyValues} from 'chrome://resources/lit/v3_0/lit.rollup.js';

import {loadTimeData} from '../i18n_setup.js';
import {routes} from '../route.js';
import type {Route} from '../router.js';
import {Router} from '../router.js';
import {SettingsViewMixinLit} from '../settings_page/settings_view_mixin_lit.js';

import {getCss} from './people_page.css.js';
import {getHtml} from './people_page.html.js';

export interface SettingsPeoplePageElement {
  $: {
    importDataDialogTrigger: HTMLElement,
    toast: CrToastElement,
  };
}

const SettingsPeoplePageElementBase =
    SettingsViewMixinLit(WebUiListenerMixinLit(CrLitElement));

export class SettingsPeoplePageElement extends SettingsPeoplePageElementBase {
  static get is() {
    return 'settings-people-page';
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
       * This flag is used to conditionally show a set of new sign-in UIs to the
       * profiles that have been migrated to be consistent with the web
       * sign-ins.
       * TODO(tangltom): In the future when all profiles are completely
       * migrated, this should be removed, and UIs hidden behind it should
       * become default.
       */
      signinAllowed_: {type: Boolean},

      /**
       * This property stores whether the profile is a Dasherless profiles,
       * which is associated with a non-Dasher account. Some UIs related to
       * sign in and sync service will be different because they are not
       * available for these profiles.
       */
      isDasherlessProfile_: {type: Boolean},

      /**
       * Stored accounts to the system, supplied by SyncBrowserProxy.
       */
      storedAccounts: {type: Array},

      /**
       * The current sync status, supplied by SyncBrowserProxy.
       */
      syncStatus: {type: Object},

      /**
       * Authentication token provided by settings-lock-screen.
       */
      authToken_: {type: String},

      /**
       * The currently selected profile icon URL. May be a data URL.
       */
      profileIconUrl_: {type: String},

      /**
       * Whether the profile row is clickable. The behavior depends on the
       * platform.
       */
      isProfileActionable_: {type: Boolean},

      /**
       * The current profile name.
       */
      profileName_: {type: String},

      primaryAccountName_: {type: String},
      primaryAccountEmail_: {type: String},
      primaryAccountIconUrl_: {type: String},

      replaceSyncPromosWithSignInPromos_: {type: Boolean},

      shouldShowGoogleAccount_: {type: Boolean},
      showImportDataDialog_: {type: Boolean},
      showSignoutDialog_: {type: Boolean},
    };
  }

  protected accessor signinAllowed_: boolean =
      loadTimeData.getBoolean('signinAllowed');
  protected accessor isDasherlessProfile_: boolean =
      loadTimeData.getBoolean('isDasherlessProfile');
  accessor syncStatus: SyncStatus|null = null;
  private accessor authToken_: string = '';
  protected accessor profileIconUrl_: string = '';
  protected accessor isProfileActionable_: boolean = true;
  protected accessor profileName_: string = '';
  private accessor replaceSyncPromosWithSignInPromos_: boolean =
      loadTimeData.getBoolean('replaceSyncPromosWithSignInPromos');
  protected accessor primaryAccountName_: string = '';
  private accessor primaryAccountEmail_: string = '';
  protected accessor primaryAccountIconUrl_: string = '';

  accessor storedAccounts: StoredAccount[]|null = null;
  protected accessor shouldShowGoogleAccount_: boolean = false;
  protected accessor showImportDataDialog_: boolean = false;
  protected accessor showSignoutDialog_: boolean = false;

  private syncBrowserProxy_: SyncBrowserProxy =
      SyncBrowserProxyImpl.getInstance();

  override connectedCallback() {
    super.connectedCallback();

    let useProfileNameAndIcon = true;
    if (useProfileNameAndIcon) {
      ProfileInfoBrowserProxyImpl.getInstance().getProfileInfo().then(
          this.handleProfileInfo_.bind(this));
      this.addWebUiListener(
          'profile-info-changed', this.handleProfileInfo_.bind(this));
    }

    this.syncBrowserProxy_.getSyncStatus().then(
        this.handleSyncStatus_.bind(this));
    this.addWebUiListener(
        'sync-status-changed', this.handleSyncStatus_.bind(this));

    this.syncBrowserProxy_.getStoredAccounts().then(
        this.handleStoredAccounts_.bind(this));
    this.addWebUiListener(
        'stored-accounts-updated', this.handleStoredAccounts_.bind(this));

    this.addWebUiListener('sync-settings-saved', () => {
      this.$.toast.show();
    });
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties);

    if (changedProperties.has('storedAccounts') ||
        changedProperties.has('syncStatus')) {
      this.shouldShowGoogleAccount_ = this.computeShouldShowGoogleAccount_();
    }
  }

  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute);

    this.showImportDataDialog_ =
        Router.getInstance().getCurrentRoute() === routes.IMPORT_DATA;

    if (Router.getInstance().getCurrentRoute() === routes.SIGN_OUT) {
      // If the sync status has not been fetched yet, optimistically display
      // the sign-out dialog. There is another check when the sync status is
      // fetched. The dialog will be closed when the user is not signed in.
      if (this.syncStatus && !this.isSyncing_()) {
        Router.getInstance().navigateToPreviousRoute();
      } else {
        this.showSignoutDialog_ = true;
      }
    }
  }

  /**
   * Handler for when the profile's icon and name is updated.
   */
  private handleProfileInfo_(info: ProfileInfo) {
    this.profileName_ = info.name;
    /**
     * Extract first frame from image by creating a single frame PNG using
     * url as input if base64 encoded and potentially animated.
     */

    this.profileIconUrl_ = info.iconUrl;
  }


  /**
   * Handler for when the sync state is pushed from the browser.
   */
  private handleSyncStatus_(syncStatus: SyncStatus) {
    this.syncStatus = syncStatus;
  }

  private computeShouldShowGoogleAccount_(): boolean {
    if (!this.storedAccounts || !this.syncStatus) {
      return false;
    }

    if (this.syncStatus.hasError &&
        this.syncStatus.statusAction !== StatusAction.UPGRADE_CLIENT &&
        this.syncStatus.statusAction !==
            StatusAction.SHOW_BOOKMARKS_LIMIT_HELP_ARTICLE) {
      return false;
    }

    return this.isSyncing_();
  }

  protected onProfileClick_() {
    Router.getInstance().navigateTo(routes.MANAGE_PROFILE);
  }

  protected onDisconnectDialogClose_() {
    this.showSignoutDialog_ = false;

    if (Router.getInstance().getCurrentRoute() === routes.SIGN_OUT) {
      Router.getInstance().navigateToPreviousRoute();
    }
  }

  protected onSyncClick_() {
    // Users can go to sync subpage regardless of sync status.
    Router.getInstance().navigateTo(routes.SYNC);
  }

  protected onAccountClick_() {
    Router.getInstance().navigateTo(routes.ACCOUNT);
  }

  protected onGoogleServicesClick_() {
    Router.getInstance().navigateTo(routes.GOOGLE_SERVICES);
  }

  protected shouldLinkToAccountSettingsPage_(): boolean {
    return this.replaceSyncPromosWithSignInPromos_ && !!this.syncStatus &&
        this.syncStatus.signedInState === SignedInState.SIGNED_IN;
  }

  protected onImportDataClick_() {
    Router.getInstance().navigateTo(routes.IMPORT_DATA);
  }

  protected onImportDataDialogClose_() {
    Router.getInstance().navigateToPreviousRoute();
    focusWithoutInk(this.$.importDataDialogTrigger);
  }

  protected shouldLinkToProfileRow_(): boolean {
    return !this.shouldShowSyncAccountControl_() &&
        !this.shouldLinkToAccountSettingsPage_();
  }

  protected shouldShowSyncAccountControl_(): boolean {
    if (!this.syncStatus) {
      return false;
    }
    return !!this.syncStatus.syncSystemEnabled && this.signinAllowed_ &&
        !this.shouldLinkToAccountSettingsPage_();
  }

  private handleStoredAccounts_(accounts: StoredAccount[]) {
    this.storedAccounts = accounts;

    // The user might not have any GAIA accounts (e.g. signed out). In this case
    // the link row to the account settings page does not exist, so there's
    // nothing to do.
    if (accounts.length === 0) {
      return;
    }
    this.primaryAccountName_ = accounts[0].fullName!;
    this.primaryAccountEmail_ = accounts[0].email;
    this.primaryAccountIconUrl_ = accounts[0].avatarImage!;
  }

  /**
   * Open URL for managing your Google Account.
   */
  protected onGoogleAccountClick_() {
    OpenWindowProxyImpl.getInstance().openUrl(
        loadTimeData.getString('googleAccountUrl'));
    chrome.metricsPrivate.recordUserAction('ManageGoogleAccount_Clicked');
  }

  /**
   * @return A CSS image-set for multiple scale factors.
   */
  protected getIconImageSet_(iconUrl?: string): string {
    if (!iconUrl) {
      return '';
    }
    return getImage(iconUrl);
  }

  protected isSyncing_() {
    return !!this.syncStatus &&
        this.syncStatus.signedInState === SignedInState.SYNCING;
  }

  protected getSyncAndNonPersonalizedServicesSubtext_(): string {
    return '';
  }

  protected shouldHideSyncSetupLinkRow_() {
    return this.replaceSyncPromosWithSignInPromos_ &&
        (!this.syncStatus ||
         this.syncStatus.signedInState !== SignedInState.SYNCING);
  }

  protected getAccountRowSubtitle_(): string {
    if (this.syncStatus && this.syncStatus.statusText) {
      if (this.syncStatus.statusAction === StatusAction.ENTER_PASSPHRASE) {
        return loadTimeData.substituteString(
            this.syncStatus.statusText, this.primaryAccountEmail_);
      }

      if (this.syncStatus.statusAction ===
          StatusAction.SHOW_BOOKMARKS_LIMIT_HELP_ARTICLE) {
        return this.syncStatus.statusText;
      }
    }

    return this.primaryAccountEmail_;
  }

  // SettingsViewMixin implementation.
  override getFocusConfig() {
    const map = new Map();
    if (routes.SYNC) {
      map.set(routes.SYNC.path, '#sync-setup');
    }
    if (routes.ACCOUNT) {
      map.set(routes.ACCOUNT.path, '#account-subpage-row');
    }
    if (routes.GOOGLE_SERVICES) {
      map.set(routes.GOOGLE_SERVICES.path, '#google-services');
    }
    if (routes.MANAGE_PROFILE) {
      map.set(
          routes.MANAGE_PROFILE.path,
          loadTimeData.getBoolean('signinAllowed') ?
              '#edit-profile' :
              '#profile-row .subpage-arrow');
    }
    return map;
  }

  // SettingsViewMixin implementation.
  override getAssociatedControlFor(childViewId: string): HTMLElement {
    const ids = [
      'sync',
      'syncControls',
      'account',
      'googleServices',
      'manageProfile',
    ];
    assert(ids.includes(childViewId));

    let triggerId: string|null = null;
    switch (childViewId) {
      case 'sync':
      case 'syncControls':
        triggerId = 'sync-setup';
        break;
      case 'account':
        assert(loadTimeData.getBoolean('replaceSyncPromosWithSignInPromos'));
        triggerId = 'account-subpage-row';
        break;
      case 'googleServices':
        assert(loadTimeData.getBoolean('replaceSyncPromosWithSignInPromos'));
        triggerId = 'google-services';
        break;
      case 'manageProfile':
        triggerId = this.signinAllowed_ ? 'edit-profile' : 'profile-row';
        break;
      default:
        assertNotReached();
    }

    assert(triggerId);

    const control = this.shadowRoot.querySelector<HTMLElement>(`#${triggerId}`);
    assert(
        control,
        `Failed to find associated control for child '${childViewId}'`);
    return control;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-people-page': SettingsPeoplePageElement;
  }
}

customElements.define(SettingsPeoplePageElement.is, SettingsPeoplePageElement);
