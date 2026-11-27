// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {html} from '//resources/lit/v3_0/lit.rollup.js';
import {ChromeSigninUserChoice} from '/shared/settings/people_page/sync_browser_proxy.js';

import {RestartType} from '../relaunch_mixin_lit.js';


import type {SettingsPersonalizationOptionsElement} from './personalization_options.js';

export function getHtml(this: SettingsPersonalizationOptionsElement) {
  return html`<!--_html_template_start_-->
<div id="chromeSigninUserChoiceSetting" class="hr cr-row" role="group"
    ?hidden="${!this.chromeSigninUserChoiceInfo_?.shouldShowSettings}"
    aria-label="$i18n{chromeSigninChoiceTitle}">
  <div id="labelWrapper" class="flex">
    <div>
      $i18n{chromeSigninChoiceTitle}
    </div>
    <div id="chromeSigninChoiceDescription" class="secondary">
      ${this.i18n(
          'chromeSigninChoiceDescription',
          this.chromeSigninUserChoiceInfo_?.signedInEmail || '')}
    </div>
  </div>
  <select id="chromeSigninUserChoiceSelection" class="md-select"
      @change="${this.onChromeSigninChoiceChange_}">
    <option selected disabled hidden
        value="${ChromeSigninUserChoice.NO_CHOICE}">
      $i18n{chromeSigninChoiceSelectOptionPlaceholder}
    </option>
    <option value="${ChromeSigninUserChoice.SIGNIN}">
      $i18n{chromeSigninChoiceSignin}
    </option>
    <option value="${ChromeSigninUserChoice.DO_NOT_SIGNIN}">
      $i18n{chromeSigninChoiceDoNotSignin}
    </option>
    <option value="${ChromeSigninUserChoice.ALWAYS_ASK}">
      $i18n{chromeSigninChoiceAlwaysAsk}
    </option>
  </select>
</div>
<settings-toggle-button id="signinAllowedToggle" class="hr"
    ?hidden="${!this.signinAvailable_}"
    ?disabled="${this.syncFirstSetupInProgress_}"
    pref-key="signin.allowed_on_next_startup" label="$i18n{signinAllowedTitle}"
    sub-label="$i18n{signinAllowedDescription}"
    @settings-boolean-control-change="${
        this.onSigninAllowedSettingsBooleanControlChange_}"
    no-set-pref>
</settings-toggle-button>
<!-- not chromeos -->
<if expr="_google_chrome">
<!-- chromeos -->
${!this.shouldUseMetricsConsentRestructure_ ? html`
  <div class="cr-row hr toggle-button-wrapper">
    <div class="flex cr-padded-text">
      <div id="metricsReportingLabel">
        $i18n{enablePersonalizationLogging}
      </div>
      <div class="cr-secondary-text">
        $i18n{enablePersonalizationLoggingDesc}
      </div>
    </div>
    ${this.showRestart_ ? html`
      <cr-button @click="${this.onRestartClick_}" id="restart">
        $i18n{restart}
      </cr-button>
    ` : ''}
    ${this.metricsReporting_?.managed ? html`
      <cr-policy-indicator indicator-type="userPolicy"></cr-policy-indicator>
    ` : ''}
    <cr-toggle id="metricsReportingControl"
        .checked="${!!this.metricsReporting_?.enabled}"
        ?disabled="${!!this.metricsReporting_?.managed}"
        @change="${this.onMetricsReportingChange_}"
        aria-labelledby="metricsReportingLabel">
    </cr-toggle>
  </div>
` : ''}
<!-- not chromeos -->
</if><!-- _google_chrome -->
<settings-toggle-button id="urlCollectionToggle" class="hr"
    pref-key="url_keyed_anonymized_data_collection.enabled"
    label="$i18n{urlKeyedAnonymizedDataCollection}"
    sub-label="$i18n{urlKeyedAnonymizedDataCollectionDesc}">
</settings-toggle-button>
<if expr="_google_chrome">
<settings-toggle-button id="spellCheckControl" class="hr"
    pref-key="spellcheck.use_spelling_service"
    @settings-boolean-control-change="${
        this.onUseSpellingServiceSettingsBooleanControlChange_}"
    label="$i18n{spellingPref}" sub-label="$i18n{spellingDescription}"
    ?hidden="${!this.showSpellCheckControlToggle_()}">
</settings-toggle-button>
<!-- chromeos -->
</if><!-- _google_chrome -->
${this.showSearchSuggestToggle_() ? html`
  <settings-toggle-button id="searchSuggestToggle" class="hr"
      pref-key="search.suggest_enabled" label="$i18n{searchSuggestPref}"
      sub-label="$i18n{searchSuggestPrefDesc}">
  </settings-toggle-button>
` : ''}

<!-- This toggle is only shown for users with the appropriate enterprise
        policy enabled. The state is not changeable by the user, so when shown,
        it is alays selected but disabled. -->
${this.showSearchAggregatorSuggest_ ? html`
  <settings-toggle-button id="searchAggregatorSuggestToggle"
      pref-key="synthetic.settings.search_aggregator_suggest" disabled
      class="hr" label="$i18n{searchAggregatorSuggestPref}"
      sub-label="$i18n{searchAggregatorSuggestPrefDesc}">
  </settings-toggle-button>
` : ''}

${this.showSignoutDialog_ ? html`
  <settings-signout-dialog .syncStatus="${this.syncStatus}"
      @close="${this.onSignoutDialogClose_}">
  </settings-signout-dialog>
` : ''}

<cr-toast id="toast">
  <div>$i18n{restartToApplyChanges}</div>
  <cr-button @click="${this.onRestartClick_}">$i18n{restart}</cr-button>
</cr-toast>

${this.shouldShowRelaunchDialog ? html`
  <relaunch-confirmation-dialog .restartType="${RestartType.RESTART}"
      @close="${this.onRelaunchDialogClose}">
  </relaunch-confirmation-dialog>
` : ''}
<cr-toast id="chromeSigninUserChoiceToast" duration="5000">
  <div>$i18n{chromeSigninChoiceToast}</div>
</cr-toast>
<!--_html_template_end_-->`;
}
