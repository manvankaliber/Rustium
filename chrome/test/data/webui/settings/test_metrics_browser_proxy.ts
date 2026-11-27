// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {YourSavedInfoDataCategory, YourSavedInfoDataChip, YourSavedInfoRelatedService} from 'chrome://settings/settings.js';
import type {AiPageComposeInteractions, AiPageInteractions, AiPageSuggestionsInteractions, AutofillAiWalletNoticeFunnelEvents, AutofillSettingsReferrer, DeleteBrowsingDataAction, MetricsBrowserProxy, PrivacyElementInteractions, PrivacyGuideInteractions, PrivacyGuideSettingsStates, PrivacyGuideStepsEligibleAndReached, SafeBrowsingInteractions, SafetyCheckUnusedSitePermissionsModuleInteractions, SafetyHubCardState, SafetyHubEntryPoint, SafetyHubModuleType, SafetyHubSurfaces, SettingsSearchExitReasonDesktop} from 'chrome://settings/settings.js';
import {TestBrowserProxy} from 'chrome://webui-test/test_browser_proxy.js';

export class TestMetricsBrowserProxy extends TestBrowserProxy implements
    MetricsBrowserProxy {
  constructor() {
    super([
      'recordAction',
      'recordBooleanHistogram',
      'recordSettingsPageHistogram',
      'recordPrivacyGuideFlowLengthHistogram',
      'recordSafeBrowsingInteractionHistogram',
      'recordPrivacyGuideNextNavigationHistogram',
      'recordPrivacyGuideEntryExitHistogram',
      'recordPrivacyGuideSettingsStatesHistogram',
      'recordPrivacyGuideStepsEligibleAndReachedHistogram',
      'recordDeleteBrowsingDataAction',
      'recordClearBrowsingDataCancelled',
      'recordSafetyHubAbusiveNotificationPermissionRevocationInteractionsHistogram',
      'recordSafetyHubCardStateClicked',
      'recordSafetyHubDashboardAnyWarning',
      'recordSafetyHubEntryPointClicked',
      'recordSafetyHubEntryPointShown',
      'recordSafetyHubImpression',
      'recordSafetyHubInteraction',
      'recordSafetyHubModuleWarningImpression',
      'recordSafetyHubUnusedSitePermissionsModuleInteractionsHistogram',
      'recordSafetyHubUnusedSitePermissionsModuleListCountHistogram',
      // <if expr="_google_chrome and is_win">
      'recordFeatureNotificationsChange',
      // </if>
      'recordAiPageInteractions',
      'recordAiPageComposeInteractions',
      'recordAiPageSuggestionsInteractions',
      'recordSettingsSearchExitReason',
      'recordSettingsSearchResultDwellTime',
      'recordSettingsNavCategoryClicked',
      'recordSettingsSearchQueryEntered',
      'recordAutofillSettingsReferrer',
      'recordYourSavedInfoCategoryClick',
      'recordYourSavedInfoDataChipClick',
      'recordYourSavedInfoRelatedServiceClick',
      'recordWalletNoticeSettingsFunnelEvent',
    ]);
  }

  recordSettingsSearchExitReason(reason: SettingsSearchExitReasonDesktop) {
    this.methodCalled('recordSettingsSearchExitReason', reason);
  }

  recordSettingsSearchResultDwellTime(time: number) {
    this.methodCalled('recordSettingsSearchResultDwellTime', time);
  }

  recordSettingsNavCategoryClicked() {
    this.methodCalled('recordSettingsNavCategoryClicked');
  }

  recordSettingsSearchQueryEntered() {
    this.methodCalled('recordSettingsSearchQueryEntered');
  }

  recordAction(action: string) {
    this.methodCalled('recordAction', action);
  }

  recordBooleanHistogram(histogramName: string, visible: boolean) {
    this.methodCalled('recordBooleanHistogram', [histogramName, visible]);
  }

  recordAutofillSettingsReferrer(
      histogramName: string, referrer: AutofillSettingsReferrer) {
    this.methodCalled(
        'recordAutofillSettingsReferrer', [histogramName, referrer]);
  }

  recordSettingsPageHistogram(interaction: PrivacyElementInteractions) {
    this.methodCalled('recordSettingsPageHistogram', interaction);
  }

  recordSafeBrowsingInteractionHistogram(
      interaction: SafeBrowsingInteractions) {
    this.methodCalled('recordSafeBrowsingInteractionHistogram', interaction);
  }

  recordPrivacyGuideNextNavigationHistogram(
      interaction: PrivacyGuideInteractions) {
    this.methodCalled('recordPrivacyGuideNextNavigationHistogram', interaction);
  }

  recordPrivacyGuideEntryExitHistogram(interaction: PrivacyGuideInteractions) {
    this.methodCalled('recordPrivacyGuideEntryExitHistogram', interaction);
  }

  recordPrivacyGuideSettingsStatesHistogram(state: PrivacyGuideSettingsStates) {
    this.methodCalled('recordPrivacyGuideSettingsStatesHistogram', state);
  }

  recordPrivacyGuideFlowLengthHistogram(steps: number) {
    this.methodCalled('recordPrivacyGuideFlowLengthHistogram', steps);
  }

  recordPrivacyGuideStepsEligibleAndReachedHistogram(
      status: PrivacyGuideStepsEligibleAndReached) {
    this.methodCalled(
        'recordPrivacyGuideStepsEligibleAndReachedHistogram', status);
  }

  recordDeleteBrowsingDataAction(action: DeleteBrowsingDataAction) {
    this.methodCalled('recordDeleteBrowsingDataAction', action);
  }

  recordClearBrowsingDataCancelled() {
    this.methodCalled('recordClearBrowsingDataCancelled');
  }

  recordSafetyHubAbusiveNotificationPermissionRevocationInteractionsHistogram(
      interaction: SafetyCheckUnusedSitePermissionsModuleInteractions) {
    this.methodCalled(
        'recordSafetyHubAbusiveNotificationPermissionRevocationInteractionsHistogram',
        interaction);
  }

  recordSafetyHubCardStateClicked(
      histogramName: string, state: SafetyHubCardState) {
    this.methodCalled(
        'recordSafetyHubCardStateClicked', [histogramName, state]);
  }

  recordSafetyHubDashboardAnyWarning(visible: boolean) {
    this.methodCalled('recordSafetyHubDashboardAnyWarning', visible);
  }

  recordSafetyHubEntryPointClicked(page: SafetyHubEntryPoint) {
    this.methodCalled('recordSafetyHubEntryPointClicked', page);
  }

  recordSafetyHubEntryPointShown(page: SafetyHubEntryPoint) {
    this.methodCalled('recordSafetyHubEntryPointShown', page);
  }

  recordSafetyHubImpression(surface: SafetyHubSurfaces) {
    this.methodCalled('recordSafetyHubImpression', surface);
  }

  recordSafetyHubInteraction(surface: SafetyHubSurfaces) {
    this.methodCalled('recordSafetyHubInteraction', surface);
  }

  recordSafetyHubModuleWarningImpression(module: SafetyHubModuleType) {
    this.methodCalled('recordSafetyHubModuleWarningImpression', module);
  }

  recordSafetyHubUnusedSitePermissionsModuleInteractionsHistogram(
      interaction: SafetyCheckUnusedSitePermissionsModuleInteractions) {
    this.methodCalled(
        'recordSafetyHubUnusedSitePermissionsModuleInteractionsHistogram',
        interaction);
  }

  recordSafetyHubUnusedSitePermissionsModuleListCountHistogram(
      suggestions: number) {
    this.methodCalled(
        'recordSafetyHubUnusedSitePermissionsModuleListCountHistogram',
        suggestions);
  }

  // <if expr="_google_chrome and is_win">
  recordFeatureNotificationsChange(enabled: boolean) {
    this.methodCalled('recordFeatureNotificationsChange', enabled);
  }
  // </if>

  recordAiPageInteractions(interaction: AiPageInteractions) {
    this.methodCalled('recordAiPageInteractions', interaction);
  }

  recordAiPageComposeInteractions(interaction: AiPageComposeInteractions) {
    this.methodCalled('recordAiPageComposeInteractions', interaction);
  }

  recordAiPageSuggestionsInteractions(
      interaction: AiPageSuggestionsInteractions) {
    this.methodCalled('recordAiPageSuggestionsInteractions', interaction);
  }

  recordYourSavedInfoCategoryClick(category: YourSavedInfoDataCategory) {
    this.methodCalled('recordYourSavedInfoCategoryClick', [category]);
    if (category !== YourSavedInfoDataCategory.COUNT) {
      this.recordAction(`Settings.YourSavedInfo.CategoryClick.${
          YourSavedInfoDataCategory[category]}`);
    }
  }

  recordYourSavedInfoDataChipClick(chip: YourSavedInfoDataChip) {
    this.methodCalled('recordYourSavedInfoDataChipClick', [chip]);
    if (chip !== YourSavedInfoDataChip.COUNT) {
      this.recordAction(
          `Settings.YourSavedInfo.ChipClick.${YourSavedInfoDataChip[chip]}`);
    }
  }

  recordYourSavedInfoRelatedServiceClick(service: YourSavedInfoRelatedService) {
    this.methodCalled('recordYourSavedInfoRelatedServiceClick', [service]);
    if (service !== YourSavedInfoRelatedService.COUNT) {
      this.recordAction(`Settings.YourSavedInfo.RelatedServiceClick.${
          YourSavedInfoRelatedService[service]}`);
    }
  }

  recordWalletNoticeSettingsFunnelEvent(
      event: AutofillAiWalletNoticeFunnelEvents) {
    this.methodCalled('recordWalletNoticeSettingsFunnelEvent', event);
  }
}
