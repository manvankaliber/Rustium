// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {html} from 'chrome://resources/lit/v3_0/lit.rollup.js';

import {AiSettingsSection} from '../route.js';

import type {SettingsAiPageIndexElement} from './ai_page_index.js';

export function getHtml(this: SettingsAiPageIndexElement) {
  return html`<!--_html_template_start_-->
<cr-view-manager id="viewManager" class="cr-centered-card-container"
    ?show-all="${this.shouldShowAll}">
  <settings-ai-info-card slot="view" id="aiInfoCard"></settings-ai-info-card>

  ${this.enableAiModeSearchSetting_ ? html`
    <settings-ai-mode-search-page slot="view" id="aiModeSearch">
    </settings-ai-mode-search-page>
  ` : ''}

  ${''}

  ${''}


  ${this.showAiPageAiFeatureSection_ ? html`
    <settings-ai-page slot="view" id="parent"></settings-ai-page>
  ` : ''}

  ${this.showComposeControl_ ? html`
    <settings-offer-writing-help-page slot="view" id="compose"
        data-parent-view-id="parent"
        route-path="${this.routes_.OFFER_WRITING_HELP.path}">
    </settings-offer-writing-help-page>
  ` : ''}

  ${this.showAiSuggestionsControl_ ? html`
    <settings-ai-suggestions-page slot="view" id="aiSuggestions"
        data-parent-view-id="parent"
        route-path="${this.routes_.AI_SUGGESTIONS.path}">
    </settings-ai-suggestions-page>
  ` : ''}

  ${''}

  ${this.showDictationControl_ ? html`
    <settings-dictation-page slot="view" id="dictation"
        data-parent-view-id="parent"
        route-path="${this.routes_.DICTATION.path}">
    </settings-dictation-page>
  ` : ''}

<if expr="_google_chrome">
  ${this.showOnDeviceAiAdvancedSettings_ ? html`
    <settings-on-device-ai-page slot="view" id="onDeviceAi"
        data-parent-view-id="parent"
        route-path="${this.routes_.ON_DEVICE_AI.path}">
    </settings-on-device-ai-page>
  ` : ''}
</if>
</cr-view-manager>
<!--_html_template_end_-->`;
}
