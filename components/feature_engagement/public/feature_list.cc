// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/public/feature_list.h"

#include <vector>

#include "build/build_config.h"
#include "components/feature_engagement/public/feature_constants.h"

namespace feature_engagement {

namespace {
// Whenever a feature is added to |kAllFeatures|, it should also be added as
// DEFINE_VARIATION_PARAM in the header, and also added to the
// |kIPHDemoModeChoiceVariations| array.
const base::Feature* const kAllFeatures[] = {
    &kIPHDummyFeature,  // Ensures non-empty array for all platforms.
    // keep-sorted start case=no
    &kIPHiOSAddressPromoDesktopFeature,
    &kIPHiOSEnhancedBrowsingDesktopFeature,
    &kIPHiOSLensPromoDesktopFeature,
    &kIPHiOSPasswordPromoDesktopFeature,
    &kIPHiOSPaymentPromoDesktopFeature,
    &kIPHiOSPriceTrackingDesktopFeature,
    &kIPHiOSTabGroupsDesktopFeature,
// keep-sorted end



#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
    &kEsbDownloadRowPromoFeature,
#endif
#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
    &kIPHExtensionsMenuFeature,
    &kIPHExtensionsRequestAccessButtonFeature,
    &kIPHExtensionsZeroStatePromoFeature,
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)
    // keep-sorted start case=no
    &kIPH_VerticalTabsTabGroupsInOrganizerFeature,
    &kIPHBackNavigationMenuFeature,
    &kIPHBatterySaverModeFeature,
    &kIPHBookmarkBarSimplifiedFeature,
    &kIPHCompanionSidePanelFeature,
    &kIPHCompanionSidePanelRegionSearchFeature,
    &kIPHComposeMSBBSettingsFeature,
    &kIPHComposeNewBadgeFeature,
    &kIPHContextualTasksEphemeralToolbarButtonFeature,
    &kIPHContextualTasksPinnedIconChangedFeature,
    &kIPHCriticalActionAppMenuFeature,
    &kIPHCriticalActionFilterChipFeature,
    &kIPHDesktopCustomizeChromeAutoOpenFeature,
    &kIPHDesktopCustomizeChromeExperimentFeature,
    &kIPHDesktopPwaInstallFeature,
    &kIPHDesktopRealboxContextualSearchFeature,
    &kIPHDesktopSharedHighlightingFeature,
    &kIPHDictationFeature,
    &kIPHDictationHotkeyFeature,
    &kIPHDiscardRingFeature,
    &kIPHDownloadEsbPromoFeature,
    &kIPHExplicitBrowserSigninPreferenceRememberedFeature,
    &kIPHFocusHelpBubbleScreenReaderPromoFeature,
    &kIPHGlassFrameOptInFeature,
    &kIPHGlassFrameOptOutFeature,
    &kIPHGlicPromoFeature,
    &kIPHGlicTrustFirstOnboardingShortcutSnoozePromoFeature,
    &kIPHGlicTryItFeature,
    &kIPHGMCCastStartStopFeature,
    &kIPHGMCLocalMediaCastingFeature,
    &kIPHGMCSaveVideoFrameFeature,
    &kIPHIndigoContextMenuSnoozeFeature,
    &kIPHIndigoContextMenuToastFeature,
    &kIPHLensOverlayFeature,
    &kIPHMemorySaverModeFeature,
    &kIPHMultistepFilterPromoFeature,
    &kIPHOmniboxAskGLensIconFeature,
    &kIPHOmniboxEverywhereLensPromoFeature,
    &kIPHPasswordManagerShortcutFeature,
    &kIPHPasswordSharingFeature,
    &kIPHPasswordsManagementBubbleAfterSaveFeature,
    &kIPHPasswordsManagementBubbleDuringSigninFeature,
    &kIPHPasswordsSavePrimingPromoFeature,
    &kIPHPasswordsSaveRecoveryPromoFeature,
    &kIPHPasswordsWebAppProfileSwitchFeature,
    &kIPHPdfGlicSummarizeFeature,
    &kIPHPdfInkSignaturesFeature,
    &kIPHPdfSearchifyFeature,
    &kIPHPdfTextAnnotationsFeature,
    &kIPHPdfTranslateBubbleFeature,
    &kIPHPerformanceInterventionDialogFeature,
    &kIPHPowerBookmarksSidePanelFeature,
    &kIPHPriceInsightsPageActionIconLabelFeature,
    &kIPHPriceTrackingEmailConsentFeature,
    &kIPHPriceTrackingInSidePanelFeature,
    &kIPHPriceTrackingPageActionIconLabelFeature,
    &kIPHProfileSwitchFeature,
    &kIPHPwaQuietNotificationFeature,
    &kIPHReadingListDiscoveryFeature,
    &kIPHReadingListEntryPointFeature,
    &kIPHReadingListInSidePanelFeature,
    &kIPHReadingModeAiPlaybackFeature,
    &kIPHReadingModeAiPlaybackSettingsFeature,
    &kIPHReadingModeKeyboardShortcutFeature,
    &kIPHReadingModeLineFocusFeature,
    &kIPHReadingModePageActionLabelFeature,
    &kIPHReadingModePresentationModeFeature,
    &kIPHSendTabToSelfTutorialFeature,
    &kIPHShoppingCollectionFeature,
    &kIPHSideBySidePinnableFeature,
    &kIPHSideBySideTabSwitchFeature,
    &kIPHSidePanelContextualTasksPinnableFeature,
    &kIPHSidePanelGenericPinnableFeature,
    &kIPHSidePanelLensOverlayPinnableFeature,
    &kIPHSidePanelLensOverlayPinnableFollowupFeature,
    &kIPHSideSearchAutoTriggeringFeature,
    &kIPHSideSearchPageActionLabelFeature,
    &kIPHSmartTabSharingDefaultOnFeature,
    &kIPHSmartTabSharingFeature,
    &kIPHSmartTabSharingTryItFeature,
    &kIPHSplitViewHorizontalIndirectAccessFeature,
    &kIPHTabAudioMutingFeature,
    &kIPHTabGroupsFocusingFeature,
    &kIPHTabGroupsNonGroupFocusingFeature,
    &kIPHTabGroupsSaveV2CloseGroupFeature,
    &kIPHTabGroupsSaveV2IntroFeature,
    &kIPHTabGroupsSharedTabChangedFeature,
    &kIPHTabGroupsSharedTabFeedbackFeature,
    &kIPHTabGroupsUnfocusingFeature,
    &kIPHTabScrollButtonFeature,
    &kIPHVerticalTabsExpandOnHoverFeature,
    &kIPHVerticalTabstripTutorialFeature,
    &kIPHWebUiHelpBubbleTestFeature,
// keep-sorted end
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_FUCHSIA)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)
    // keep-sorted start case=no
    &kIPH_AutofillAmbientAutofillSpiiDisclaimer,
    &kIPHAutofillAccountNameEmailSuggestionFeature,
    &kIPHAutofillAiValuablesFeature,
    &kIPHAutofillAtMemoryFeature,
    &kIPHAutofillBnplAffirmOrZipSuggestionFeature,
    &kIPHAutofillBnplAffirmZipOrKlarnaSuggestionFeature,
    &kIPHAutofillCardInfoRetrievalSuggestionFeature,
    &kIPHAutofillCreditCardBenefitFeature,
    &kIPHAutofillDisabledVirtualCardSuggestionFeature,
    &kIPHAutofillDownstreamCardAwarenessFeature,
    &kIPHAutofillEnableLoyaltyCardsFeature,
    &kIPHAutofillExternalAccountProfileSuggestionFeature,
    &kIPHAutofillHomeWorkProfileSuggestionFeature,
    &kIPHAutofillOmniboxPaymentChipFeature,
    &kIPHAutofillVirtualCardCVCSuggestionFeature,
    &kIPHAutofillVirtualCardSuggestionFeature,
    &kIPHAutofillWalletDirectOffersFeature,
// keep-sorted end
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID) ||
        // BUILDFLAG(IS_FUCHSIA)


#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    // keep-sorted start case=no
    &kIPHDesktopPWAsLinkCapturingLaunch,
    &kIPHDesktopPWAsLinkCapturingLaunchAppInTab,
// keep-sorted end
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
    &kIPHExtensionsPinnedByDefaultFeature,
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    // keep-sorted start case=no
    &kIPHSupervisedUserProfileSigninFeature,
// keep-sorted end
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
    // keep-sorted start case=no
    &kIPHSearchPromotionFeature,
// keep-sorted end
#endif  // BUILDFLAG(IS_WIN)

};
}  // namespace

std::vector<const base::Feature*> GetAllFeatures() {
  return std::vector<const base::Feature*>(std::begin(kAllFeatures),
                                           std::end(kAllFeatures));
}

}  // namespace feature_engagement
