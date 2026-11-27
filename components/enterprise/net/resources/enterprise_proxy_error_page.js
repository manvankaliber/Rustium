// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Should match enterprise_net::EnterpriseProxyErrorData::ErrorCategory.
const EnterpriseProxyErrorCategory = {
  AUTHENTICATION: '0',
  AUTHORIZATION: '1',
  OTHER: '2',
};


function goBackButtonClicked() {
  if (window.history.length > 1) {
    window.history.back();
  } else {
    window.close();
  }
}

function signinButtonClicked() {
  if (window.errorPageController) {
    window.errorPageController.portalSigninButtonClick();
  } else {
    window.location.reload();
  }
}

document.addEventListener('DOMContentLoaded', () => {
  // TODO(crbug.com/563039777): Set the Learn More link URL once available.
  const learnMoreLink = document.getElementById('learn-more-link');
  if (learnMoreLink) {
    learnMoreLink.addEventListener('click', (e) => {
      e.preventDefault();
    });
  }

  const category = String(window.loadTimeDataRaw?.['error_category'] ?? '');
  if (category === EnterpriseProxyErrorCategory.AUTHENTICATION) {
    document.body.classList.add('category-authentication');
  } else if (category === EnterpriseProxyErrorCategory.AUTHORIZATION) {
    document.body.classList.add('category-authorization');
  } else {
    document.body.classList.add('category-other');
  }

  const primaryButton = document.getElementById('primary-button');
  if (primaryButton) {
    primaryButton.addEventListener(
        'click',
        category === EnterpriseProxyErrorCategory.AUTHENTICATION ?
            signinButtonClicked :
            goBackButtonClicked);
  }
});
