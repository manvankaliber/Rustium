// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {html} from '//resources/lit/v3_0/lit.rollup.js';

import type {ManagementUiElement} from './management_ui.js';

export function getHtml(this: ManagementUiElement) {
  return html`<!--_html_template_start_-->
<cr-toolbar page-name="$i18n{toolbarTitle}" role="banner" autofocus
    @search-changed="${this.onSearchChanged_}" clear-label="$i18n{clearSearch}"
    search-prompt="$i18n{searchPrompt}">
</cr-toolbar>
<main id="mainContent" class="cr-scrollable">
  <div class="cr-scrollable-top-shadow"></div>
  <div class="cr-centered-card-container">
    <div class="card">
      <section ?hidden="${!this.managed_}" class="page-subtitle">
        <cr-icon-button class="icon-arrow-back" id="closeButton"
            @click="${this.onBackClick_}" aria-label="$i18n{backButton}">
        </cr-icon-button>
        <h2 class="cr-title-text">${this.subtitle_}</h2>
      </section>
      ${this.shouldShowPromotion_ ? html`
        <promotion-banner @promotion-dismissed="${this.onPromotionDismissed_}">
        </promotion-banner>
      ` : ''}

      <section class="overview-section">
        <div .innerHTML="${this.managementNoticeHtml_}"></div>
      </section>


      ${this.showThreatProtectionInfo_() ? html`
        <section>
          <h3 class="cr-title-text">$i18n{threatProtectionTitle}</h3>
          <div class="subtitle">${this.threatProtectionInfo_!.description}</div>
          ${this.threatProtectionInfo_!.showSecureGatewayDisclosure ? html`
            <div class="subtitle">
              <div class="secure-gateway-title">
                $i18n{secureGatewayTitle}
              </div>
              <div class="secure-gateway-description">
                $i18n{secureGatewayDisclosure}
              </div>
            </div>
          ` : ''}
          <!-- Wide Screen View: Semantic Table -->
          <table class="content-indented wide-screen-only"
              ?hidden="${!this.threatProtectionInfo_!.info.length}">
            <tr>
              <th class="protection-name">$i18n{connectorEvent}</th>
              <th class="protection-permissions">
                $i18n{connectorVisibleData}
              </th>
            </tr>
            ${this.threatProtectionInfo_!.info.map(item => html`
              <tr>
                <td class="protection-name">${this.i18n(item.title)}</td>
                <td class="protection-permissions">
                  ${this.i18n(item.permission)}
                </td>
              </tr>
            `)}
          </table>

          <!-- Small Screen View: Semantic Stacked List -->
          <ul class="content-indented small-screen-only" role="list"
              ?hidden="${!this.threatProtectionInfo_!.info.length}">
            ${this.threatProtectionInfo_!.info.map(item => html`
              <li class="connector-item" role="listitem">
                <div class="connector-item-title">
                  <strong>$i18n{connectorEvent}:</strong>
                  ${this.i18n(item.title)}
                </div>
                <div class="connector-item-value">
                  <strong>$i18n{connectorVisibleData}:</strong>
                  ${this.i18n(item.permission)}
                </div>
              </li>
            `)}
          </ul>
        </section>
      ` : ''}

      ${this.showBrowserReportingInfo_() ? html`
        <section>
          <h3 class="cr-title-text">$i18n{browserReporting}</h3>
          <div class="subtitle">$i18n{browserReportingExplanation}</div>
          <div>
            ${this.browserReportingInfo_!.map(item => html`
              <div class="report">
                <cr-icon icon="${item.icon}"></cr-icon>
                <ul class="browser">
                  ${item.messageIds.map(messageId => html`
                    <li .innerHTML="${this.i18nAdvanced(messageId)}"></li>
                  `)}
                </ul>
              </div>
            `)}
          </div>
        </section>
      ` : ''}

      ${this.showProfileReportingInfo_() ? html`
        <section>
          <h3 class="cr-title-text">$i18n{browserReporting}</h3>
          <div class="subtitle">$i18n{profileReportingExplanation}</div>
          <div>
            <div class="report">
              <ul class="profile">
                ${this.profileReportingInfo_!.map(item => html`
                  <li .innerHTML="${this.i18nAdvanced(item.messageIds[0]!)}">
                  </li>
                `)}
              </ul>
            </div>
          </div>
        </section>
      ` : ''}
      ${this.showExtensionReportingInfo_() ? html`
        <section class="extension-reporting">
          <h3 class="cr-title-text">$i18n{extensionReporting}</h3>
          <div class="subtitle">${this.extensionReportingSubtitle_}</div>

          <!-- Wide Screen View: Semantic Table -->
          <table class="content-indented wide-screen-only">
            <tr>
              <th class="extension-name">$i18n{extensionName}</th>
              <th class="extension-permissions">
                $i18n{extensionPermissions}
              </th>
            </tr>
            ${this.extensions_!.map(item => html`
              <tr>
                <td class="extension-name">
                  <div .title="${item.name}" role="presentation">
                    <img .src="${item.icon || ''}" alt="" aria-hidden="true">
                    <span>${item.name}</span>
                  </div>
                </td>
                <td class="extension-permissions">
                  <ul>
                    ${item.permissions.map(permission => html`
                      <li>${permission}</li>
                    `)}
                  </ul>
                </td>
              </tr>
            `)}
          </table>

          <!-- Small Screen View: Semantic Stacked List -->
          <ul class="content-indented small-screen-only" role="list">
            ${this.extensions_!.map(item => html`
              <li class="connector-item" role="listitem">
                <div class="connector-item-title">
                  <strong>$i18n{extensionName}:</strong>
                  <div .title="${item.name}" role="presentation"
                      class="extension-title-container">
                    <img .src="${item.icon || ''}" alt="" aria-hidden="true">
                    <span>${item.name}</span>
                  </div>
                </div>
                <div class="connector-item-value">
                  <strong>$i18n{extensionPermissions}:</strong>
                  <ul>
                    ${item.permissions.map(permission => html`
                      <li>${permission}</li>
                    `)}
                  </ul>
                </div>
              </li>
            `)}
          </ul>
        </section>
      ` : ''}

      ${this.showManagedWebsitesInfo_() ? html`
        <section class="managed-webistes">
          <h3 class="cr-title-text">$i18n{managedWebsites}</h3>
          <div class="subtitle">${this.managedWebsitesSubtitle_}</div>
          <div class="content-indented">
            ${this.managedWebsites_!.map(item => html`
              <div class="report">${item}</div>
            `)}
          </div>
        </section>
      ` : ''}

      ${this.showApplicationReportingInfo_() ? html`
        <section class="application-reporting">
          <h3 class="cr-title-text">$i18n{applicationReporting}</h3>
          <div class="subtitle">${this.applicationReportingSubtitle_}</div>

          <!-- Wide Screen View: Semantic Table -->
          <table class="content-indented wide-screen-only">
            <tr>
              <th class="application-name">$i18n{applicationName}</th>
              <th class="extension-permissions">
                $i18n{applicationPermissions}
              </th>
            </tr>
            ${this.applications_!.map(item => html`
              <tr>
                <td class="application-name">
                  <div .title="${item.name}" role="presentation">
                    <img .src="${item.icon || ''}" alt="" aria-hidden="true">
                    <span>${item.name}</span>
                  </div>
                </td>
                <td class="application-permissions">
                  <ul>
                    ${item.permissions.map(permission => html`
                      <li>${permission}</li>
                    `)}
                  </ul>
                </td>
              </tr>
            `)}
          </table>

          <!-- Small Screen View: Semantic Stacked List -->
          <ul class="content-indented small-screen-only" role="list">
            ${this.applications_!.map(item => html`
              <li class="connector-item" role="listitem">
                <div class="connector-item-title">
                  <strong>$i18n{applicationName}:</strong>
                  <div .title="${item.name}" role="presentation"
                      class="extension-title-container">
                    <img .src="${item.icon || ''}" alt="" aria-hidden="true">
                    <span>${item.name}</span>
                  </div>
                </div>
                <div class="connector-item-value">
                  <strong>$i18n{applicationPermissions}:</strong>
                  <ul>
                    ${item.permissions.map(permission => html`
                      <li>${permission}</li>
                    `)}
                  </ul>
                </div>
              </li>
            `)}
          </ul>
        </section>
      ` : ''}
    </div>
  </div>
</main>
<!--_html_template_end_-->`;
}
