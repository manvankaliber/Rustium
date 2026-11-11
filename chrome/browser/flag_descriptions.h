// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FLAG_DESCRIPTIONS_H_
#define CHROME_BROWSER_FLAG_DESCRIPTIONS_H_

#include "base/debug/debugging_buildflags.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/common/buildflags.h"
#include "components/compose/buildflags.h"
#include "components/paint_preview/buildflags/buildflags.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "components/webui/flags/feature_entry.h"
#include "device/vr/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"
#include "media/gpu/buildflags.h"
#include "media/media_buildflags.h"
#include "net/net_buildflags.h"
#include "pdf/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/blink/public/common/buildflags.h"

// This file declares strings used in chrome://flags. These messages are not
// translated, because instead of end-users they target Chromium developers and
// testers. See https://crbug.com/40457200 and https://crbug.com/41308302 for
// more details.
//
// Comments are not necessary. The contents of the strings (which appear in the
// UI) should be good enough documentation for what flags do and when they
// apply. If they aren't, fix them.
//
// Sort flags in each section alphabetically by the k...Name constant. Follow
// that by the k...Description constant and any special values associated with
// that.
//
// Please do NOT include preprocessor directives - these are no longer required.

namespace flag_descriptions {

inline constexpr char kAccelerated2dCanvasName[] = "Accelerated 2D canvas";
inline constexpr char kAccelerated2dCanvasDescription[] =
    "Enables the use of the GPU to perform 2d canvas rendering instead of "
    "using software rendering.";

inline constexpr char kAim3pEntrypointName[] = "AIM 3P entrypoint";
inline constexpr char kAim3pEntrypointDescription[] =
    "Enables the omnibox AI Mode entrypoint for third party search engines.";

inline constexpr char kAiModeEntryPointAlwaysNavigatesName[] =
    "AI Mode Omnibox Entrypoint always navigates";
inline constexpr char kAiModeEntryPointAlwaysNavigatesDescription[] =
    "If enabled, clicking aim button in omnibox always navigates directly to "
    "google.com/aimode.";

inline constexpr char kAiSubscriptionAvatarRingFollowupName[] =
    "AI Subscription Avatar Ring follow-up";
inline constexpr char kAiSubscriptionAvatarRingFollowupDescription[] =
    "Shows the AI subscription avatar decorations on more surfaces. Requires "
    "#enable-ai-subscription-avatar-ring.";

inline constexpr char kOmniboxDynamicAimSubmitName[] = "Dynamic Aim Submit";
inline constexpr char kOmniboxDynamicAimSubmitDescription[] =
    "Enables dynamic aim button submission variants.";

inline constexpr char kOmniboxAimServerEligibilityName[] =
    "AIM Server Eligibility";
inline constexpr char kOmniboxAimServerEligibilityDescription[] =
    "Enable AIM server eligibility checks.";

inline constexpr char kOmniboxLoomName[] = "Enable Loom";
inline constexpr char kOmniboxLoomDescription[] =
    "Enables experimental Loom features.";

inline constexpr char kOmniboxTruncateSearchUrlName[] =
    "Omnibox Truncate Search URL";
inline constexpr char kOmniboxTruncateSearchUrlDescription[] =
    "Truncates search URLs for the original query ('oq') and query ('q') "
    "parameters across various length limits.";

inline constexpr char kZeroSuggestPrefetchingForComposeboxName[] =
    "Zero suggest prefetching for composebox";
inline constexpr char kZeroSuggestPrefetchingForComposeboxDescription[] =
    "Enables prefetching of the zero prefix suggestions for composebox "
    "contexts.";

inline constexpr char kAimServerEligibilityIncludeClientLocaleName[] =
    "AIM Server Eligibility: Include Client Locale";
inline constexpr char kAimServerEligibilityIncludeClientLocaleDescription[] =
    "Configures whether to include the client locale in the AIM eligibility "
    "request.";

inline constexpr char kCameraAndMicrophoneElementsName[] =
    "Camera and Microphone elements";
inline constexpr char kCameraAndMicrophoneElementsDescription[] =
    "Enables the <camera> and <microphone> elements, allowing sites to embed "
    "camera and microphone permission request controls directly in the page.";

inline constexpr char kCanvasHibernationName[] = "Hibernation for 2D canvas";
inline constexpr char kCanvasHibernationDescription[] =
    "Enables canvas hibernation for 2D canvas.";

inline constexpr char kCapabilityDelegationDigitalCredentialsName[] =
    "Capability Delegation for Digital Credentials";
inline constexpr char kCapabilityDelegationDigitalCredentialsDescription[] =
    "Allows a web page to delegate the capability of requesting digital "
    "credentials (via both create and get requests of the Digital Credentials "
    "API) to trusted cross-origin iframes.";

inline constexpr char kCapabilityElementIconOnlyModeName[] =
    "CapabilityElementIconOnlyMode";
inline constexpr char kCapabilityElementIconOnlyModeDescription[] =
    "Enables icon-only mode for capability elements.";

inline constexpr char kCaptureHandleForStandalonePwasAndIwasName[] =
    "Capture Handle for Standalone PWAs and IWAs";
inline constexpr char kCaptureHandleForStandalonePwasAndIwasDescription[] =
    "Enables Capture Handle support for standalone PWAs and IWAs "
    "when their windows are captured.";

inline constexpr char kCentralizedInfoBarFrameworkName[] =
    "Centralized InfoBar Framework";
inline constexpr char kCentralizedInfoBarFrameworkDescription[] =
    "Enables the centralized desktop infobars framework.";

inline constexpr char kChroMenuSearchName[] = "ChroMenu Search";
inline constexpr char kChroMenuSearchDescription[] =
    "Enables search in the app Chrome menu. Only takes effect when "
    "#app-menu-glow-up is also enabled.";

inline constexpr char kCredentialManagementUnifiedUiName[] =
    "Credential Management Unified UI";
inline constexpr char kCredentialManagementUnifiedUiDescription[] =
    "Enables the Unified UI for the credential management password APIs.";

inline constexpr char kCrossDeviceSigninFromDesktopName[] =
    "Cross-Device Sign-in From Desktop";
inline constexpr char kCrossDeviceSigninFromDesktopDescription[] =
    "Enables entry points to sign in on a mobile phone using provided QR code.";

inline constexpr char kCrossDeviceSigninFromDesktopFollowupName[] =
    "Cross-Device Sign-in From Desktop Follow-up";
inline constexpr char kCrossDeviceSigninFromDesktopFollowupDescription[] =
    "Enables follow-up entry points (e.g. the What's New page) to sign in on a "
    "mobile phone using provided QR code. Requires "
    "#cross-device-signin-from-desktop.";

inline constexpr char kCrossTabElementCaptureName[] =
    "Element Capture cross-tab";
inline constexpr char kCrossTabElementCaptureDescription[] =
    "Allows the Element Capture API to be used cross-tab. (Only has an effect "
    "if Element Capture is generally enabled.)";

inline constexpr char kCrossTabRegionCaptureName[] = "Region Capture cross-tab";
inline constexpr char kCrossTabRegionCaptureDescription[] =
    "Allows the Region Capture API to be used cross-tab. (Only has an effect "
    "if Region Capture is generally enabled.)";

inline constexpr char kAcceleratedVideoDecodeName[] =
    "Hardware-accelerated video decode";
inline constexpr char kAcceleratedVideoDecodeDescription[] =
    "Hardware-accelerated video decode where available.";

inline constexpr char kAcceleratedVideoEncodeName[] =
    "Hardware-accelerated video encode";
inline constexpr char kAcceleratedVideoEncodeDescription[] =
    "Hardware-accelerated video encode where available.";

inline constexpr char kAlignWakeUpsName[] = "Align delayed wake ups at 125 Hz";
inline constexpr char kAlignWakeUpsDescription[] =
    "Run most delayed tasks with a non-zero delay (including DOM Timers) on a "
    "periodic 125Hz tick, instead of as soon as their delay has passed.";

inline constexpr char kAllowInsecureLocalhostName[] =
    "Allow invalid certificates for resources loaded from localhost.";
inline constexpr char kAllowInsecureLocalhostDescription[] =
    "Allows requests to localhost over HTTPS even when an invalid certificate "
    "is presented.";

inline constexpr char kNewContentForCheckerboardedScrollsName[] =
    "Change scrolling scheduling to reduce checkerboarding";
inline constexpr char kNewContentForCheckerboardedScrollsDescription[] =
    "If enabled, scrolling that would generate blank frames will now "
    "prioritize the new content over scrolling with the intention of "
    "decreasing the amount of checkerboarded frames.";

inline constexpr char kApiContextualTasksPrivateName[] =
    "Extensions Contextual Tasks API";
inline constexpr char kApiContextualTasksPrivateDescription[] =
    "Enables the contextualTasksPrivate API and contextual tasks component "
    "extension.";

inline constexpr char kApiMimeHandlerName[] =
    "Enable chrome.mimeHandler extension API";
inline constexpr char kApiMimeHandlerDescription[] =
    "Enables the chrome.mimeHandler extension API for extensions with a "
    "mime_types_handler manifest entry. The API is currently limited to "
    "a small set of MIME types (e.g. application/pdf).";

inline constexpr char kAppMenuGlowUpName[] = "App Menu Glow Up";
inline constexpr char kAppMenuGlowUpDescription[] =
    "Enables the App Menu Glow Up feature.";

inline constexpr char kApproximateGeolocationPermissionName[] =
    "Approximate Geolocation Permission";
inline constexpr char kApproximateGeolocationPermissionDescription[] =
    "Enables the approximate geolocation permission prompt.";

inline constexpr char kAudioRendererMixerImmediatePauseName[] =
    "AudioRendererMixer Immediate Pause";
inline constexpr char kAudioRendererMixerImmediatePauseDescription[] =
    "Immediately pauses the underlying audio output stream on explicit media "
    "player pauses instead of waiting for the 10-second mixer pause delay.";

inline constexpr char kAutomaticUsbDetachName[] =
    "Automatically detach USB kernel drivers";
inline constexpr char kAutomaticUsbDetachDescription[] =
    "Automatically detach kernel drivers when a USB interface is busy.";

inline constexpr char kBackForwardCacheForComponentExtensionFramesName[] =
    "BackForwardCache for component extension frames";
inline constexpr char
    kBackForwardCacheForComponentExtensionFramesDescription[] =
        "Allows pages embedding opted-in component extension frames to "
        "enter BackForwardCache.";

inline constexpr char kBackgroundResourceFetchName[] =
    "Background Resource Fetch";
inline constexpr char kBackgroundResourceFetchDescription[] =
    "Process resource requests in a background thread inside Blink.";

inline constexpr char kByDateHistoryInSidePanelName[] =
    "By Date History in Side Panel";
inline constexpr char kByDateHistoryInSidePanelDescription[] =
    "If enabled, shows the 'By Date' History in Side Panel";

inline constexpr char kBlockingFocusWithoutUserActivationName[] =
    "Blocking Focus Without User Activation";
inline constexpr char kBlockingFocusWithoutUserActivationDescription[] =
    "When enabled, the focus-without-user-activation permissions policy can "
    "be used by embedders to block programmatic focus calls "
    "(element.focus(), window.focus(), autofocus) from iframes unless "
    "triggered by a user gesture.";

inline constexpr char kBrowsingHistoryImprovedHostnameSuffixMatchingName[] =
    "Browsing History Improved Hostname Suffix Matching";
inline constexpr char
    kBrowsingHistoryImprovedHostnameSuffixMatchingDescription[] =
        "Improves hostname suffix matching for browsing history, so that "
        "example.com matches example.com as well as subdomains like "
        "www.example.com.";

inline constexpr char kBrowsingHistoryFilterByActorName[] =
    "Filter Browsing History By Actor";
inline constexpr char kBrowsingHistoryFilterByActorDescription[] =
    "Enables browser history filtering by actor on Android. On desktop this "
    "changes the existing actor filter UI to dropdown menu.";

inline constexpr char kBrowsingHistoryFilterByDeviceName[] =
    "Filter Browsing History By Device";
inline constexpr char kBrowsingHistoryFilterByDeviceDescription[] =
    "Enables browser history filtering by device.";

inline constexpr char kBrowsingHistoryFilterByDomainName[] =
    "Filter Browsing History By Domain";
inline constexpr char kBrowsingHistoryFilterByDomainDescription[] =
    "Enables browser history filtering by domain.";

inline constexpr char kBrowsingHistoryFilterByTimeName[] =
    "Filter Browsing History By Time";
inline constexpr char kBrowsingHistoryFilterByTimeDescription[] =
    "Enables browser history filtering by time.";

inline constexpr char kBrowsingHistorySimilarVisitsGroupingName[] =
    "Browsing History Grouping Improvements";
inline constexpr char kBrowsingHistorySimilarVisitsGroupingDescription[] =
    "Enables improvements to grouping of similar browsing history on the "
    "history page.";

inline constexpr char kBundledSecuritySettingsName[] =
    "Bundled Security Settings";
inline constexpr char kBundledSecuritySettingsDescription[] =
    "Enables new Bundled Security Settings UI on chrome://settings/security. "
    "This new UI bundles all security settings into either an enhanced or "
    "standard bundle which should simplify the security settings page and also "
    "help simplify the user's decision.";

inline constexpr char kBundledSecuritySettingsAskBeforeHttpName[] =
    "Ask-Before-HTTP in Bundled Security Settings";
inline constexpr char kBundledSecuritySettingsAskBeforeHttpDescription[] =
    "Includes Ask-Before-HTTP (HTTPS-First Mode) in the Enhanced security "
    "bundle.";

inline constexpr char kBundledSecuritySettingsSecureDnsV2Name[] =
    "Bundled Security Settings Secure Dns V2";
inline constexpr char kBundledSecuritySettingsSecureDnsV2Description[] =
    "Enables Secure DNS V2 setting in the bundles on the "
    "chrome://settings/security page. The setting will move from the advanced "
    "section to the bundled section.";

inline constexpr char kCanvasDrawElementName[] = "HTML-in-Canvas";
inline constexpr char kCanvasDrawElementDescription[] =
    "Enables the Canvas 2D drawElement API and the WebGL texElement2D API for "
    "drawing HTML content into a canvas. "
    "See: https://github.com/WICG/html-in-canvas";

inline constexpr char kChangePasswordToolName[] = "Change Password Tool";
inline constexpr char kChangePasswordToolDescription[] =
    "Enables the actor change password tool for filling and submitting change "
    "password forms.";

inline constexpr char kChildProcessSecurityPolicyRustName[] =
    "Rust-based ChildProcessSecurityPolicy";
inline constexpr char kChildProcessSecurityPolicyRustDescription[] =
    "Enables the experimental Rust implementation of "
    "ChildProcessSecurityPolicy. Provides options to run just the Rust "
    "implementation or Rust and C++ implementations side-by-side for "
    "validation. The default mode when enabled is Rust-only. Portions of "
    "ChildProcessSecurityPolicy that haven't been implemented in Rust yet will "
    "fall back to their C++ implementations. This can also be configured to "
    "control whether the Rust implementation manages per-process security "
    "state.";
inline constexpr char kChildProcessSecurityPolicyRustChoiceRustOnly[] =
    "Rust only";
inline constexpr char kChildProcessSecurityPolicyRustChoiceRustAndCpp[] =
    "Rust and C++";
inline constexpr char
    kChildProcessSecurityPolicyRustChoiceRustOnlyWithProcessState[] =
        "Rust only (with ProcessState)";
inline constexpr char
    kChildProcessSecurityPolicyRustChoiceRustAndCppWithProcessState[] =
        "Rust and C++ (with ProcessState)";

inline constexpr char kConnectionAllowlistsName[] = "Connection Allowlists";
inline constexpr char kConnectionAllowlistsDescription[] =
    "Enables a prototype implementation of `Connection-Allowlist` header "
    "parsing and enforcement. See https://github.com/mikewest/anti-exfil/";

inline constexpr char kCriticalActionHistoryName[] =
    "Critical Actions in History";
inline constexpr char kCriticalActionHistoryDescription[] =
    "Surfaces critical actions, such as password fills and file downloads, "
    "performed during user-initiated AI agent browsing sessions alongside "
    "page visits on the Chrome History page for transparency and auditing.";

inline constexpr char kStylusHandwritingWinName[] =
    "Stylus Handwriting for Windows.";
inline constexpr char kStylusHandwritingWinDescription[] =
    "Enables an experimental feature that lets users handwrite into text "
    "fields using a compatible stylus. Only supported on Windows builds 22621 "
    "(patch 5126 and newer), 22631 (patch 5126 and newer) and all builds equal "
    "to or newer than 26100.3624";

inline constexpr char kPermissionsGestureGatedPromptsName[] =
    "Permissions Gesture Gated Prompts";
inline constexpr char kPermissionsGestureGatedPromptsDescription[] =
    "Mute non-user-initiated permission requests.";

inline constexpr char kCryptographyComplianceCnsaName[] =
    "Cryptography Compliance (CNSA)";
inline constexpr char kCryptographyComplianceCnsaDescription[] =
    "If enabled, Chrome will configure its preferred algorithms for TLS to "
    "prefer algorithms that satisfy the requirements of the Commercial "
    "National Security Algorithm Suite (CNSA) versions 1.0 and 2.0. Enabling "
    "this flag does not guarantee that any specific algorithms will be "
    "negotiated. This flag is not required for security.";

inline constexpr char kCssGamutMappingName[] = "CSS Gamut Mapping";
inline constexpr char kCssGamutMappingDescription[] =
    "Enable experimental CSS gamut mapping implementation.";

inline constexpr char kCSSGridLanesLayoutName[] = "CSS Grid Lanes Layout";
inline constexpr char kCSSGridLanesLayoutDescription[] =
    "Enable experimental CSS Grid Lanes Layout implementation. Simple layouts "
    "with grid-lanes are supported. Subgrid and fragmentation "
    "are not supported yet. The syntax to use CSS Grid Lanes is `display: "
    "grid-lanes` together with grid properties (i.e. `grid-column`, "
    "`grid-row`, "
    "etc.). More details on grid-lanes syntax can be found at "
    "https://www.w3.org/TR/css-grid-3/#masonry-model.";

inline constexpr char kCustomizeChromeSidePanelExtensionsCardName[] =
    "Customize Chrome Side Panel Extension Card";
inline constexpr char kCustomizeChromeSidePanelExtensionsCardDescription[] =
    "If enabled, shows an extension card within the Customize Chrome Side "
    "Panel for access to the Chrome Web Store extensions.";

inline constexpr char kCustomizeChromeWallpaperSearchName[] =
    "Customize Chrome Wallpaper Search";
inline constexpr char kCustomizeChromeWallpaperSearchDescription[] =
    "Enables wallpaper search in Customize Chrome Side Panel.";

inline constexpr char kCustomizeChromeWallpaperSearchButtonName[] =
    "Customize Chrome Wallpaper Search Button";
inline constexpr char kCustomizeChromeWallpaperSearchButtonDescription[] =
    "Enables entry point on Customize Chrome Side Panel's Appearance page for "
    "Wallpaper Search.";

inline constexpr char kCustomizeChromeWallpaperSearchInspirationCardName[] =
    "Customize Chrome Wallpaper Search Inspiration Card";
inline constexpr char
    kCustomizeChromeWallpaperSearchInspirationCardDescription[] =
        "Shows inspiration card in Customize Chrome Side Panel Wallpaper "
        "Search. "
        "Requires #customize-chrome-wallpaper-search to be enabled too.";

inline constexpr char kEnableCancelUploadOnContentAnalysisName[] =
    "Cancel file uploads on content analysis";
inline constexpr char kEnableCancelUploadOnContentAnalysisDescription[] =
    "Allows users to cancel file uploads while content analysis is in "
    "progress.";

inline constexpr char kDataControlsSearchWithName[] =
    "Data Controls enforcement for search context menu item";
inline constexpr char kDataControlsSearchWithDescription[] =
    "Enables the Enterprise Data Controls for restricting data exfiltration "
    "with the \"Search with...\" context menu item.";

inline constexpr char kDataSharingName[] = "Data Sharing";
inline constexpr char kDataSharingDescription[] =
    "Enabled all Data Sharing related UI and features.";

inline constexpr char kDataSharingJoinOnlyName[] = "Data Sharing Join Only";
inline constexpr char kDataSharingJoinOnlyDescription[] =
    "Enabled Data Sharing Joining flow related UI and features.";

inline constexpr char kDataSharingNonProductionEnvironmentName[] =
    "Data Sharing server environment";
inline constexpr char kDataSharingNonProductionEnvironmentDescription[] =
    "Sets data sharing server environment.";

// LINT.IfChange(DataSharingVersioning)
// Data Sharing versioning test scenarios.
inline constexpr char kDataSharingVersioningStatesName[] =
    "Data Sharing Versioning Test Scenarios";
inline constexpr char kDataSharingVersioningStatesDescription[] =
    "Testing multiple scenarios for versioning.";
inline constexpr char kDataSharingSharedDataTypesEnabled[] =
    "Version out-of-date, no UI";
inline constexpr char kDataSharingSharedDataTypesEnabledWithUi[] =
    "Version out-of-date, show UI ";
// LINT.ThenChange(//ios/chrome/browser/flags/ios_chrome_flag_descriptions.cc:DataSharingVersioning)

inline constexpr char kDefaultBrowserChangedOsNotificationDescription[] =
    "Enables the framework to show OS Notification when Chrome is no longer "
    "the default browser.";
inline constexpr char kDefaultBrowserChangedOsNotificationName[] =
    "Default Browser Changed OS Notification";

inline constexpr char kDefaultBrowserFrameworkDescription[] =
    "Enables the default browser framework.";
inline constexpr char kDefaultBrowserFrameworkName[] =
    "Default Browser Framework";

inline constexpr char kDefaultBrowserPromptSurfacesDescription[] =
    "Enables using various default browser prompt surfaces.";
inline constexpr char kDefaultBrowserPromptSurfacesName[] =
    "Default Browser Prompt Surfaces";

inline constexpr char kDefaultBrowserSetterSelectionDescription[] =
    "Enables selecting a default browser setter (e.g. shell integration, "
    "visual guide, or on Windows 11 shell integration annotated with a bubble "
    "pointing at the Settings \"Set default\" button).";
inline constexpr char kDefaultBrowserSetterSelectionName[] =
    "Default Browser Setter Selection";

inline constexpr char kDefaultBrowserStickyModalDescription[] =
    "Enables the sticky modal dialog for the default browser prompt. Requires "
    "#default-browser-prompt-surfaces to be set to a modal dialog variation.";
inline constexpr char kDefaultBrowserStickyModalName[] =
    "Default Browser Sticky Modal";

inline constexpr char kDefaultSearchEnginePrewarmName[] =
    "Default search engine prewarm";
inline constexpr char kDefaultSearchEnginePrewarmDescription[] =
    "Performance optimization to prewarm the default search engine used in the "
    "Omnibox";

inline constexpr char kDictationName[] = "Dictation";
inline constexpr char kDictationDescription[] =
    "Enables the Dictation feature.";

inline constexpr char kDisableAutofillStrikeSystemName[] =
    "Disable the Autofill strike system";
inline constexpr char kDisableAutofillStrikeSystemDescription[] =
    "When enabled, the Autofill strike system will not block a feature from "
    "being offered.";

inline constexpr char kFieldClassificationModelCachingName[] =
    "Enable caching field classification predictions";
inline constexpr char kFieldClassificationModelCachingDescription[] =
    "When enabled, the field classification model uses runtime caching to not "
    "run models on the same inputs multiple times.";

inline constexpr char kHistorySyncAlternativeIllustrationName[] =
    "History Sync Alternative Illustration";
inline constexpr char kHistorySyncAlternativeIllustrationDescription[] =
    "Enables history sync alternative illustration.";

inline constexpr char kDeprecateUnloadName[] = "Deprecate the unload event";
inline constexpr char kDeprecateUnloadDescription[] =
    "Controls the default for Permissions-Policy unload. If enabled, unload "
    "handlers are deprecated and will not receive the unload event unless a "
    "Permissions-Policy to enable them has been explicitly set. If  disabled, "
    "unload handlers will continue to receive the unload event unless "
    "explicitly disabled by Permissions-Policy, even during the gradual "
    "rollout of their deprecation.";

inline constexpr char kTabFaviconChipsToCoinsName[] =
    "Tab favicon chips to coins";
inline constexpr char kTabFaviconChipsToCoinsDescription[] =
    "Removes existing tab chips from the composebox and instead adds favicon "
    "coins next to the \"+\" button.";

inline constexpr char kTabStripDeclutterName[] = "Tab Strip Declutter";
inline constexpr char kTabStripDeclutterDescription[] =
    "Enables the Tab Strip Declutter feature on Desktop.";

inline constexpr char kTabStripUnificationName[] = "Tab Strip Unification";
inline constexpr char kTabStripUnificationDescription[] =
    "Unifies horizontal and vertical tab strips to share infrastructure.";

inline constexpr char kNewHorizontalPinnedTabStylingName[] =
    "New Horizontal Pinned Tab Styling";
inline constexpr char kNewHorizontalPinnedTabStylingDescription[] =
    "Enables the updated pinned tab style in the new horizontal tab strip.";

inline constexpr char kEnergyEffectName[] = "Energy Effect";
inline constexpr char kEnergyEffectDescription[] =
    "Enables the Energy Effect for the Realbox.";

inline constexpr char kEnergyEffectAnimationName[] = "Energy Effect Animation";
inline constexpr char kEnergyEffectAnimationDescription[] =
    "Enables animations for the Energy Effect for the Realbox.";

inline constexpr char kEnableBenchmarkingName[] = "Enable benchmarking";
inline constexpr char kEnableBenchmarkingDescription[] =
    "Sets all features to a fixed state; that is, disables randomization for "
    "feature states. If '(Default Feature States)' is selected, sets all "
    "features to their default state. If '(Match Field Trial Testing Config)' "
    "is selected, sets all features to the state configured in the field trial "
    "testing config. This is used by developers and testers "
    "to diagnose whether an observed problem is caused by a non-default "
    "base::Feature configuration. This flag is automatically reset "
    "after 3 restarts and will be off from the 4th restart. On the 3rd "
    "restart, the flag will appear to be off but the effect is still active.";
inline constexpr char kEnableBenchmarkingChoiceDisabled[] = "Disabled";
inline constexpr char kEnableBenchmarkingChoiceDefaultFeatureStates[] =
    "Default Feature States";
inline constexpr char kEnableBenchmarkingChoiceMatchFieldTrialTestingConfig[] =
    "Match Field Trial Testing Config";

inline constexpr char kUseUnexportableKeyServiceInBrowserProcessName[] =
    "Enable UnexportableKeyService mojo service in the browser process.";
inline constexpr char kUseUnexportableKeyServiceInBrowserProcessDescription[] =
    "When enabled, the browser process will create an Unexportable Key "
    "Service which can be used by other, less privileged processes. This "
    "enables DBSC in platforms where access to TPM-like features is "
    "privileged.";

inline constexpr char kEnableUnexportableKeysSpareKeyPoolName[] =
    "Enable Unexportable Keys Spare Key Pool";
inline constexpr char kEnableUnexportableKeysSpareKeyPoolDescription[] =
    "Enables in-memory spare key pooling for unexportable keys to reduce "
    "latency.";

inline constexpr char kEnableCrossDevicePrefTrackerName[] =
    "Enable Cross-Device Pref Tracker";
inline constexpr char kEnableCrossDevicePrefTrackerDescription[] =
    "Enables the tracking and sharing of select non-syncing preference values "
    "across a user's signed-in devices.";

inline constexpr char kEnableCrossDeviceThemeTrackerName[] =
    "Cross-Device Theme Tracker";
inline constexpr char kEnableCrossDeviceThemeTrackerDescription[] =
    "Enables tracking and querying theme configurations across devices "
    "to facilitate theme imports and badge indicators.";

inline constexpr char kEnableExtensionInstallPolicyFetchingName[] =
    "Enable Extension Install Policy Fetching";
inline constexpr char kEnableExtensionInstallPolicyFetchingDescription[] =
    "Enables fetching of extension install policies from the cloud for the "
    "ExtensionInstallCloudPolicyChecksEnabled policy.";

inline constexpr char kD3D12VideoEncoderName[] = "Use D3D12 video encoder";
inline constexpr char kD3D12VideoEncoderDescription[] =
    "Enables D3D12 video encoding. The system might still fall back to "
    "Media Foundation video encoder if D3D12 encoder is not available "
    "or fails to initialize.";

inline constexpr char kPrefetchActivationBeaconName[] =
    "Prefetch Activation Beacon";
inline constexpr char kPrefetchActivationBeaconDescription[] =
    "If enabled, prefetch activation beacon will be sent when a prefetch is "
    "activated.";

inline constexpr char kPrerender2Name[] = "Prerendering";
inline constexpr char kPrerender2Description[] =
    "If enabled, browser features and the speculation rules API can trigger "
    "prerendering. If disabled, all prerendering APIs still exist, but a "
    "prerender will never successfully take place.";

inline constexpr char kPrerender2ReuseHostName[] = "Prerender Reuse Host";
inline constexpr char kPrerender2ReuseHostDescription[] =
    "If enabled, the browser will reuse the prerender host and the underlying"
    "process for future prerendered pages when possible.";

inline constexpr char kPrerender2CrossOriginIframesName[] =
    "Prerender2 Cross-Origin Iframes";
inline constexpr char kPrerender2CrossOriginIframesDescription[] =
    "Enable Prerender2 for cross-origin iframes.";

inline constexpr char kPrerenderUntilScriptName[] = "Prerender Until Script";
inline constexpr char kPrerenderUntilScriptDescription[] =
    "Prerenders pages until a script is about to be executed. The script "
    "execution and the page parsing will be deferred until action.";

inline constexpr char kPrerenderActivationByFormSubmissionName[] =
    "Prerender Activation By Form Submission";
inline constexpr char kPrerenderActivationByFormSubmissionDescription[] =
    "Allow Prerender activations by form submission.";

inline constexpr char kBookmarkBarPrefetchName[] = "BookmarkBarPrefetch";
inline constexpr char kBookmarkBarPrefetchDescription[] =
    "If enabled, bookmarkbar can trigger prefetch";

inline constexpr char kBookmarkBarAreaKeyboardNavigationName[] =
    "Bookmark bar area keyboard navigation";
inline constexpr char kBookmarkBarAreaKeyboardNavigationDescription[] =
    "Splits the bookmarks bar into keyboard navigation areas (apps and managed "
    "bookmarks, saved tab groups, bookmarks). Tab stops once per area and "
    "Left/Right arrow keys move between buttons within the area.";

inline constexpr char kNewTabPagePrefetchName[] = "NewTabPagePrefetch";
inline constexpr char kNewTabPagePrefetchDescription[] =
    "If enabled, NewTabPage can trigger prefetch";

inline constexpr char kNewTabPagePrerender2Name[] = "NewTabPagePrerender2";
inline constexpr char kNewTabPagePrerender2Description[] =
    "If enabled, NewTabPage can trigger prerendering";

inline constexpr char kEnableDrDcName[] =
    "Enables Display Compositor to use a new gpu thread.";
inline constexpr char kEnableDrDcDescription[] =
    "When enabled, chrome uses 2 gpu threads instead of 1. "
    " Display compositor uses new dr-dc gpu thread and all other clients "
    "(raster, webgl, video) "
    " continues using the gpu main thread.";

inline constexpr char kIsolatedSandboxedIframesName[] =
    "Isolated sandboxed iframes";
inline constexpr char kIsolatedSandboxedIframesDescription[] =
    "When enabled, applies process isolation to iframes with the 'sandbox' "
    "attribute and without the 'allow-same-origin' permission set on that "
    "attribute. This also applies to documents with a similar CSP sandbox "
    "header, even in the main frame. The affected sandboxed documents can be "
    "grouped into processes based on their URL's site or origin. The default "
    "grouping when enabled is per-site.";

inline constexpr char kAutofillEnableAutofillSettingsEnterprisePolicyName[] =
    "Enable Autofill settings enterprise policy";
inline constexpr char
    kAutofillEnableAutofillSettingsEnterprisePolicyDescription[] =
        "When enabled, the Autofill settings enterprise policy takes effect, "
        "and forms ai settings are decoupled from Autofill profile settings. ";

inline constexpr char kAutofillCustomFormControlScriptsName[] =
    "Autofill custom form control scripts";
inline constexpr char kAutofillCustomFormControlScriptsDescription[] =
    "When enabled, Autofill runs site-specific scripts to detect and fill "
    "custom form controls, such as select-like widgets that aren't <select> "
    "elements.";

inline constexpr char kAutofillCustomFormControlScriptsDirName[] =
    "Autofill custom form control scripts directory";
inline constexpr char kAutofillCustomFormControlScriptsDirDescription[] =
    "Absolute path of a local directory with an index.json file and the "
    "custom form control scripts it references. Only used if "
    "#autofill-custom-form-control-scripts is enabled.";

inline constexpr char kAutofillDisableBnplCountryCheckForTestingName[] =
    "Disable the country check for BNPL testing";
inline constexpr char kAutofillDisableBnplCountryCheckForTestingDescription[] =
    "Enables testing BNPL in countries where it would otherwise be disabled.";

inline constexpr char
    kAutofillDisableWalletDirectOffersLocaleCountryCheckForTestingName[] =
        "Disable the locale and country check for Wallet direct offers testing";
inline constexpr char
    kAutofillDisableWalletDirectOffersLocaleCountryCheckForTestingDescription
        [] = "Enables testing Wallet direct offers with locales and in "
             "countries where they would otherwise be disabled. Requires "
             "#autofill-enable-wallet-direct-offers.";

inline constexpr char kAutofillEnableAiBasedAmountExtractionName[] =
    "Enable AI-based checkout amount extraction on Chrome";
inline constexpr char kAutofillEnableAiBasedAmountExtractionDescription[] =
    "When enabled, Chrome will extract the checkout amount from the checkout "
    "page using server-side AI.";
inline constexpr char
    kAutofillAiBasedAmountExtractionIgnoreSeenTermsForTestingName[] =
        "Enable ignore AI-based Amount Extraction Terms Acknowledgement";
inline constexpr char
    kAutofillAiBasedAmountExtractionIgnoreSeenTermsForTestingDescription[] =
        "When enabled, the BNPL flow acts as if the user has not yet seen "
        "or the AI terms. This allows the AI terms to be shown repeatedly "
        "for testing purposes, regardless of the actual stored user "
        "preference.";

inline constexpr char kAutofillAiAlwaysShowPrivateAiNoticeName[] =
    "Autofill AI always show Private AI notice";
inline constexpr char kAutofillAiAlwaysShowPrivateAiNoticeDescription[] =
    "Forces the Autofill AI Private AI notice suggestion to be shown for "
    "testing, skipping cool-off timers and impression count checks.";

inline constexpr char kAutofillAiDedupeEntitiesName[] =
    "Autofill AI dedupe entities";
inline constexpr char kAutofillAiDedupeEntitiesDescription[] =
    "Enables periodic deduplication of Autofill AI entities.";

inline constexpr char kAutofillAiNoFillingIconsExperimentName[] =
    "Autofill AI no filling icons experiment";
inline constexpr char kAutofillAiNoFillingIconsExperimentDescription[] =
    "If enabled, Autofill AI filling suggestions do not have an icon.";

inline constexpr char kAutofillAiReauthRequiredName[] =
    "Autofill AI re-authentication required";
inline constexpr char kAutofillAiReauthRequiredDescription[] =
    "Requires re-authentication when filling or viewing sensitive fields with "
    "Autofill AI.";

inline constexpr char kAutofillAiServerModelName[] = "Autofill AI server model";
inline constexpr char kAutofillAiServerModelDescription[] =
    "Allows the client to trigger the server model for Autofill AI type "
    "predictions.";

inline constexpr char kAutofillAiSpiiVerificationName[] =
    "Autofill AI SPII Verification";
inline constexpr char kAutofillAiSpiiVerificationDescription[] =
    "Enables in-context source verification when filling sensitive personal "
    "context entities.";

inline constexpr char kAutofillAiValuablesIPHName[] =
    "IPH Autofill AI Valuables";
inline constexpr char kAutofillAiValuablesIPHDescription[] =
    "Enables the In-Product Help for Autofill AI valuables.";

inline constexpr char kAutofillAiWalletFlightReservationName[] =
    "Autofill AI Google Wallet flight reservations";
inline constexpr char kAutofillAiWalletFlightReservationDescription[] =
    "Enables Autofill AI support for flight reservation entities from Google "
    "Wallet.";

inline constexpr char kAutofillAiWalletPassBranding2026Name[] =
    "Autofill AI Google Wallet Pass Branding Updates";
inline constexpr char kAutofillAiWalletPassBranding2026Description[] =
    "When enabled, brings Autofill AI strings and images referencing passes "
    "in Google Wallet into consistency with 2026 branding requirements.";

inline constexpr char kAutofillAiWalletPrivatePassesName[] =
    "Autofill AI Google Wallet private passes";
inline constexpr char kAutofillAiWalletPrivatePassesDescription[] =
    "Enables Autofill AI support for private passes from Google Wallet.";

inline constexpr char kAutofillAiWalletPrivatePassesDeepLinkName[] =
    "Autofill AI Google Wallet private passes deep link";
inline constexpr char kAutofillAiWalletPrivatePassesDeepLinkDescription[] =
    "Enables Autofill AI support for deep linking to private passes from "
    "Google Wallet.";

inline constexpr char kAutofillAiWalletVehicleRegistrationName[] =
    "Autofill AI Google Wallet vehicle registration";
inline constexpr char kAutofillAiWalletVehicleRegistrationDescription[] =
    "Enables Autofill AI support for vehicle registration entities from Google "
    "Wallet.";

inline constexpr char kAutofillAiWithDataSchemaName[] =
    "Autofill AI with data schema";
inline constexpr char kAutofillAiWithDataSchemaDescription[] =
    "Enables the second iteration of Autofill AI, which uses a more advanced "
    "data schema.";

inline constexpr char kAutofillEnableAiCardRecommendationName[] =
    "Enable AI-based card recommendations on Autofill";
inline constexpr char kAutofillEnableAiCardRecommendationDescription[] =
    "When enabled, AI-based card recommendations will be offered on Autofill "
    "when at least two credit card suggestions are shown.";

inline constexpr char kAutofillEnableAmountExtractionName[] =
    "Enable checkout amount extraction.";
inline constexpr char kAutofillEnableAmountExtractionDescription[] =
    "When enabled, Chrome will extract the checkout amount from the checkout "
    "page of the allowlisted merchant websites.";
inline constexpr char kAutofillEnableAmountExtractionTestingName[] =
    "Enable amount extraction testing";
inline constexpr char kAutofillEnableAmountExtractionTestingDescription[] =
    "Enables testing of the result of checkout amount extraction. This flag "
    "will allow amount extraction to run on any website when a CC form is "
    "clicked.";

inline constexpr char kAutofillEnableBnplAffirmInternationalizationName[] =
    "Enable BNPL Affirm Internationalization";
inline constexpr char
    kAutofillEnableBnplAffirmInternationalizationDescription[] =
        "When enabled, users will have the option to pay with BNPL Affirm "
        "in international markets.";

inline constexpr char kAutofillEnableBnplKlarnaInternationalizationName[] =
    "Enable BNPL Klarna Internationalization";
inline constexpr char
    kAutofillEnableBnplKlarnaInternationalizationDescription[] =
        "When enabled, users will have the option to pay with BNPL Klarna "
        "in international markets.";

inline constexpr char kAutofillEnableBuyNowPayLaterName[] =
    "Enable buy now pay later on Autofill";
inline constexpr char kAutofillEnableBuyNowPayLaterDescription[] =
    "When enabled, users will have the option to pay with buy now pay later on "
    "specific merchant webpages.";

inline constexpr char kAutofillEnableBuyNowPayLaterForExternallyLinkedName[] =
    "Enable buy now pay later for externally linked BNPL issuer";
inline constexpr char
    kAutofillEnableBuyNowPayLaterForExternallyLinkedDescription[] =
        "When enabled, users will have the option to pay with buy now pay "
        "later "
        "with externally linked issuer on specific merchant webpages.";

inline constexpr char kAutofillEnableBuyNowPayLaterForKlarnaName[] =
    "Enable buy now pay later on Autofill for Klarna";
inline constexpr char kAutofillEnableBuyNowPayLaterForKlarnaDescription[] =
    "When enabled, users will have the option to pay with buy now pay later "
    "with Klarna on specific merchant webpages.";

inline constexpr char kAutofillEnableBuyNowPayLaterSyncingName[] =
    "Enable syncing buy now pay later user data.";
inline constexpr char kAutofillEnableBuyNowPayLaterSyncingDescription[] =
    "When enabled, Chrome will sync user data related to buy now pay later.";

inline constexpr char
    kAutofillEnableBuyNowPayLaterUpdatedSuggestionSecondLineStringName[] =
        "Enable issuer names in the second line of a BNPL suggestion";
inline constexpr char
    kAutofillEnableBuyNowPayLaterUpdatedSuggestionSecondLineStringDescription
        [] = "When enabled, the second line of a BNPL suggestion is updated to "
             "include the issuer names for better brand recognition.";

inline constexpr char kAutofillEnableCardOnDeviceVerificationEnforcementName[] =
    "Enable Card-on-Device Verification Enforcement for Payments Autofill";
inline constexpr char
    kAutofillEnableCardOnDeviceVerificationEnforcementDescription[] =
        "When enabled, server card retrieval will ensure a card is verified "
        "via CVC on a device before proceeding with risk-based or biometric "
        "authentication.";

inline constexpr char kAutofillEnableCardholderNameFixFlowName[] =
    "Enable Autofill Cardholder Name Fix Flow";
inline constexpr char kAutofillEnableCardholderNameFixFlowDescription[] =
    "When enabled, prompts users to confirm their cardholder name when "
    "checking out with a server card that is missing a name, if a name was "
    "found.";

inline constexpr char
    kAutofillEnableCvcStorageAndFillingStandaloneFormEnhancementName[] =
        "Enable CVC storage and filling standalone form enhancement for "
        "payments "
        "autofill";
inline constexpr char
    kAutofillEnableCvcStorageAndFillingStandaloneFormEnhancementDescription[] =
        "When enabled, this will enhance the CVV storage project. The "
        "enhancement will enable CVV storage suggestions for standalone CVC "
        "fields.";

inline constexpr char kAutofillEnableFpanRiskBasedAuthenticationName[] =
    "Enable risk-based authentication for FPAN retrieval";
inline constexpr char kAutofillEnableFpanRiskBasedAuthenticationDescription[] =
    "When enabled, server card retrieval will begin with a risk-based check "
    "instead of jumping straight to CVC or biometric auth.";

inline constexpr char kIPHAutofillCreditCardBenefitFeatureName[] =
    "Enable Card Benefits in-product help bubble";
inline constexpr char kIPHAutofillCreditCardBenefitFeatureDescription[] =
    "Enables In-Product-Help that appears when at least one autofill credit "
    "card suggestion includes card benefits.";

inline constexpr char kIPHDesktopRealboxContextualSearchFeatureName[] =
    "IPH Desktop Realbox Contextual Search";
inline constexpr char kIPHDesktopRealboxContextualSearchFeatureDescription[] =
    "Enables In-Product Help for Desktop Realbox Contextual Search.";

inline constexpr char kContextHubName[] = "Context Hub";
inline constexpr char kContextHubDescription[] =
    "Enables the experimental chrome://context-hub page.";

inline constexpr char kContextHubTopicsName[] = "Context Hub Topics";
inline constexpr char kContextHubTopicsDescription[] =
    "Enables the experimental Topics view in chrome://context-hub. Requires "
    "#context-hub, #sync-journey and #use-sync-sandbox.";

inline constexpr char kContextMenuAnimationLimitingName[] =
    "Context Menu Animation Limiting";
inline constexpr char kContextMenuAnimationLimitingDescription[] =
    "When enabled, the impressions of the context menu animation "
    "will be limited.";

inline constexpr char kAutofillEnableExpandIbanRegexPatternName[] =
    "Enable Expand IBAN Regex Pattern";
inline constexpr char kAutofillEnableExpandIbanRegexPatternDescription[] =
    "When enabled, IBAN regex pattern matching is expanded to support more "
    "formats.";

inline constexpr char kAutofillEnableGradientGoogleLogosName[] =
    "Enable gradient-style GPay and Wallet logos";
inline constexpr char kAutofillEnableGradientGoogleLogosDescription[] =
    "When enabled, gradient-style GPay and Wallet branding logos will be used.";

inline constexpr char kAutofillGmailOtpPreLaunchMetricsName[] =
    "Enable Gmail OTP Pre-launch Metrics";
inline constexpr char kAutofillGmailOtpPreLaunchMetricsDescription[] =
    "Enables pre-launch metrics for one-time passwords (OTPs) received in "
    "Gmail.";

inline constexpr char kAutofillShowGmailOtpSuggestionsName[] =
    "Show Gmail OTP Suggestions";
inline constexpr char kAutofillShowGmailOtpSuggestionsDescription[] =
    "When enabled, shows autofill suggestions for one-time passwords (OTPs) "
    "received in Gmail.";

inline constexpr char kAutofillEnableOAuth2TokenFixName[] =
    "Enable Autofill OAuth2 token fix";
inline constexpr char kAutofillEnableOAuth2TokenFixDescription[] =
    "When enabled, Autofill payments requests fetch OAuth2 tokens from the "
    "identity service on each request, instead of keeping local cache in "
    "Autofill components.";

inline constexpr char kAutofillEnableOffersAndLoyaltyCardsSettingsName[] =
    "Enable Offers and loyalty cards settings";
inline constexpr char
    kAutofillEnableOffersAndLoyaltyCardsSettingsDescription[] =
        "When enabled, loyalty cards and Wallet direct offers are moved from "
        "the Payments settings page into a new Offers and loyalty cards "
        "settings page, with a separate toggle to control them.";

inline constexpr char kAutofillEnableOmniboxAutofillName[] =
    "Enable Omnibox Autofill chip for payments";
inline constexpr char kAutofillEnableOmniboxAutofillDescription[] =
    "When enabled, an Autofill payments omnibox chip will appear for relevant "
    "payment checkout forms.";

inline constexpr char kAutofillEnablePayNowPayLaterTabsName[] =
    "Enable Pay Now Pay Later tabs UI for payments autofill on Chrome";
inline constexpr char kAutofillEnablePayNowPayLaterTabsDescription[] =
    "When enabled, Chrome will use the Pay Now Pay Later tabs UI for payments "
    "autofill when buy now pay later options are available for the merchant "
    "webpage.";

inline constexpr char kAutofillEnableResurrectingPaymentsUsersName[] =
    "Enable resurrecting churned payments users";
inline constexpr char kAutofillEnableResurrectingPaymentsUsersDescription[] =
    "When enabled, users that have previously turned off payments autofill "
    "will be prompted to turn it back on in instances where they can benefit "
    "from it.";

inline constexpr char
    kAutofillEnableTravelCategoryAndMerchantBenefitsFromCurinosName[] =
        "Enable travel category and merchant benefits from Curinos";
inline constexpr char
    kAutofillEnableTravelCategoryAndMerchantBenefitsFromCurinosDescription[] =
        "When enabled, travel category and merchant benefits sourced from "
        "Curinos will be shown in Autofill.";

inline constexpr char kAutofillEnableVcn3dsAuthenticationName[] =
    "Enable 3DS authentication for virtual cards";
inline constexpr char kAutofillEnableVcn3dsAuthenticationDescription[] =
    "When enabled, Chrome will trigger 3DS authentication during a virtual "
    "card retrieval if a challenge is required, 3DS authentication is "
    "available for the card, and FIDO is not.";

inline constexpr char kAutofillEnableWalletDirectOffersName[] =
    "Enable Autofill for Wallet Direct Offers";
inline constexpr char kAutofillEnableWalletDirectOffersDescription[] =
    "When enabled, direct offers synced via Google Wallet will be available "
    "for autofill into merchant promo code fields during checkout.";

inline constexpr char
    kAutofillEnableWalletDirectOffersNotificationBubbleName[] =
        "Enable Wallet Direct Offers Notification Bubble";
inline constexpr char
    kAutofillEnableWalletDirectOffersNotificationBubbleDescription[] =
        "When enabled, an offer notification bubble will be shown for wallet "
        "direct offers if any are available for the current merchant page.";

inline constexpr char kAutofillEnableWalletDisclosureNoticePublicPassName[] =
    "Enable Wallet Disclosure Notice for public passes";
inline constexpr char
    kAutofillEnableWalletDisclosureNoticePublicPassDescription[] =
        "When enabled, shows the Wallet Disclosure Notice at the end of the "
        "save prompt for public passes.";

inline constexpr char kAutofillEnableWalletGmailOffersName[] =
    "Enable Wallet Gmail offers";
inline constexpr char kAutofillEnableWalletGmailOffersDescription[] =
    "When enabled, offers from Gmail are synced via Google Wallet in addition "
    "to direct offers.";

inline constexpr char kAutofillEnableWalletReminderNoticeName[] =
    "Enable Wallet Reminder Notice for cards";
inline constexpr char kAutofillEnableWalletReminderNoticeDescription[] =
    "When enabled, shows the Wallet Reminder Notice prompt or bottom sheet "
    "for cards after payment form submission.";

inline constexpr char kAutofillEnableWalletReminderNoticePublicPassName[] =
    "Enable Wallet Reminder Notice for public passes";
inline constexpr char
    kAutofillEnableWalletReminderNoticePublicPassDescription[] =
        "When enabled, shows the Wallet Reminder Notice prompt or bottom sheet "
        "after submitting a form with a public pass.";

inline constexpr char
    kAutofillIgnorePaymentsChurnedUsersStrikesForTestingName[] =
        "Ignore payments churned users strikes for testing";
inline constexpr char
    kAutofillIgnorePaymentsChurnedUsersStrikesForTestingDescription[] =
        "When enabled, ignores the strike database checks for the payments "
        "churned users resurrecting bubble feature. For testing purposes.";

inline constexpr char kAutofillPaymentsFieldSwappingName[] =
    "Swap credit card suggestions";
inline constexpr char kAutofillPaymentsFieldSwappingDescription[] =
    "When enabled, swapping autofilled payment suggestions would result"
    "in overriding all of the payments fields with the swapped profile data";

inline constexpr char kAutofillPreferBuyNowPayLaterBlocklistsName[] =
    "Prefer blocklists instead of allowlists for Payments Autofill Buy Now Pay "
    "Later (BNPL)";
inline constexpr char kAutofillPreferBuyNowPayLaterBlocklistsDescription[] =
    "When enabled, Payments Autofill Buy Now Pay Later (BNPL) will use each "
    "corresponding issuer's blocklist instead of allowlist to check for "
    "website eligibility.";

inline constexpr char kAutofillUpstreamEnforceStrikeDelayName[] =
    "Require a week between offers to save credit cards";
inline constexpr char kAutofillUpstreamEnforceStrikeDelayDescription[] =
    "When enabled, users should not see offers to save the same credit card "
    "twice in a week, as the strike database enforces a 7-day delay between "
    "strikes.";

inline constexpr char kAutofillEnableWalletBrandingName[] =
    "Update Google Pay branding to Wallet where applicable";
inline constexpr char kAutofillEnableWalletBrandingDescription[] =
    "When enabled, certain strings and logos referencing Google Account, "
    "Google Payments, and Google Pay will instead reference Google Wallet.";

inline constexpr char kAutofillEnableWalletBrandingV2Name[] =
    "Further update Google Pay and Google Wallet branding where applicable";
inline constexpr char kAutofillEnableWalletBrandingV2Description[] =
    "When enabled, further brings certain strings and images referencing "
    "Google Pay and Google Wallet into consistency with branding requirements.";

inline constexpr char kAutoPictureInPictureOnWindowOccludedName[] =
    "Auto picture in picture on window occluded";
inline constexpr char kAutoPictureInPictureOnWindowOccludedDescription[] =
    "Allows auto picture-in-picture to trigger when the window is occluded "
    "by another window.";

inline constexpr char kBindCookiesToPortName[] =
    "Bind cookies to their setting origin's port";
inline constexpr char kBindCookiesToPortDescription[] =
    "If enabled, cookies will only be accessible by origins with the same port "
    "as the one that originally set the cookie.";

inline constexpr char kBindCookiesToSchemeName[] =
    "Bind cookies to their setting origin's scheme";
inline constexpr char kBindCookiesToSchemeDescription[] =
    "If enabled, cookies will only be accessible by origins with the same "
    "scheme as the one that originally set the cookie";

inline constexpr char kBlockCrossPartitionBlobUrlFetchingName[] =
    "Block Cross Partition Blob URL Fetching";
inline constexpr char kBlockCrossPartitionBlobUrlFetchingDescription[] =
    "Blocks fetching of cross-partitioned Blob URL.";

inline constexpr char kBookmarksEncryptionName[] = "Bookmarks Encryption";
inline constexpr char kBookmarksEncryptionDescription[] =
    "Enable encryption for bookmarks";

inline constexpr char kSessionEncryptionName[] = "Session Encryption";
inline constexpr char kSessionEncryptionDescription[] =
    "Enable encryption for sessions";

inline constexpr char kBrowserActuatorName[] = "Browser Actuator";
inline constexpr char kBrowserActuatorDescription[] =
    "Enables the Browser Actuator service for remote browser automation.";

inline constexpr char kBrowserActuatorInternalsName[] =
    "Browser Actuator Internals";
inline constexpr char kBrowserActuatorInternalsDescription[] =
    "Enables the chrome://browser-actuator-internals diagnostic dashboard.";


// For UX reasons we prefer "enabled", but that is used internally to refer to
// whether borealis is installed or not, so the name of the variable is a bit
// different to the user-facing name.
inline constexpr char kTabsFromOtherDevicesEnhancementName[] =
    "Tabs from other devices";
inline constexpr char kTabsFromOtherDevicesEnhancementDescription[] =
    "Enables updated cross-device tab sharing and visibility features.";

inline constexpr char kGetDisplayMediaConfersActivationName[] =
    "getDisplayMedia() confers transient activation.";
inline constexpr char kGetDisplayMediaConfersActivationDescription[] =
    "When getDisplay() is invoked by the application, the user is shown a "
    "dialog which allows them to share a tab, a window or a screen. If this "
    "flag is enabled, then after the user chooses what to share, transient "
    "activation is conferred on the Web application.";

inline constexpr char kDeclarativePerformanceObserverName[] =
    "Declarative Performance Observer";
inline constexpr char kDeclarativePerformanceObserverDescription[] =
    "Enables the Declarative Performance Observer API, a browser-resident "
    "telemetry system that captures and reliably reports end-to-end user "
    "journey metrics using HTTP response headers.";

inline constexpr char kDevicePostureName[] = "Device Posture API";
inline constexpr char kDevicePostureDescription[] =
    "Enables Device Posture API (foldable devices)";

inline constexpr char kDocumentPictureInPictureAnimateResizeName[] =
    "Document Picture-in-Picture Animate Resize";
inline constexpr char kDocumentPictureInPictureAnimateResizeDescription[] =
    "Use an animation when programmatically resizing a document"
    "picture-in-picture window";

inline constexpr char kDsePreload2Name[] = "Default Search Engine preload 2";
inline constexpr char kDsePreload2Description[] =
    "Enables new DSE preload instead of existing one, which uses //content "
    "prefetch";

inline constexpr char kDsePreload2OnPressName[] =
    "Default Search Engine preload 2, on-press triggers";
inline constexpr char kDsePreload2OnPressDescription[] =
    "Enables on-press triggers of DsePreload2";

inline constexpr char kHttpCacheCustomBackendName[] =
    "Use custom disk cache backend for HTTP Cache";
inline constexpr char kHttpCacheCustomBackendDescription[] =
    "Enables the experimental disk cache backend for HTTP Cache. This flag is "
    "overridden by the CacheEncryptionEnabled enterprise policy on supported "
    "platforms.";

inline constexpr char kHttpHeadersInjectionName[] = "HTTP Headers Injection";
inline constexpr char kHttpHeadersInjectionDescription[] =
    "Enables injection of custom HTTP headers in network requests.";

inline constexpr char kVariableSystemFontSupportOnWindowsName[] =
    "Variable system font support on Windows";
inline constexpr char kVariableSystemFontSupportOnWindowsDescription[] =
    "Enables axis-aware matching and application of variation settings "
    "for variable system fonts on Windows.";

inline constexpr char kViewportSegmentsName[] = "Viewport Segments API";
inline constexpr char kViewportSegmentsDescription[] =
    "Enable the viewport segment API, giving information about the logical "
    "segments of the device (dual screen and foldable devices)";

inline constexpr char kVisitedURLRankingServiceDeduplicationName[] =
    "Visited URL ranking deduplication strategy";
inline constexpr char kVisitedURLRankingServiceDeduplicationDescription[] =
    "Enables visited url ranking service to use one of various deduplication "
    "strategies.";

inline constexpr char
    kVisitedURLRankingServiceHistoryVisibilityScoreFilterName[] =
        "Enable visited URL aggregates visibility score based filtering";
inline constexpr char
    kVisitedURLRankingServiceHistoryVisibilityScoreFilterDescription[] =
        "Enables filtering of visited URL aggregates based on history URL "
        "visibility scores.";

inline constexpr char kVisualGuidedSetterDockingName[] =
    "Visual Guided Setter Docking";
inline constexpr char kVisualGuidedSetterDockingDescription[] =
    "Enables continuous docking in visual guided setter.";

inline constexpr char kMagiChromePasskeySignInName[] =
    "MagiChrome Passkey Sign-in";
inline constexpr char kMagiChromePasskeySignInDescription[] =
    "Enables the MagiChrome passkey sign-in experiment. Each variation below "
    "matches one arm of the experiment.";

inline constexpr char kCollaborationEntrepriseV2Name[] =
    "Collaboration Entreprise V2";
inline constexpr char kCollaborationEntrepriseV2Description[] =
    "Enables the collaboration feature for entreprise users within the same "
    "domain.";

inline constexpr char kCollaborationMessagingName[] = "Collaboration Messaging";
inline constexpr char kCollaborationMessagingDescription[] =
    "Enables the messaging framework within the collaboration feature, "
    "including features such as recent activity, dirty dots, and description "
    "action chips.";

inline constexpr char kComposeboxRichImageSuggestionsRealboxName[] =
    "Composebox Rich Image Suggestions (Realbox)";
inline constexpr char kComposeboxRichImageSuggestionsRealboxDescription[] =
    "Enables richer image suggestions in the Realbox Composebox for image "
    "generation.";

inline constexpr char kComposeboxRichImageSuggestionsOmniboxName[] =
    "Composebox Rich Image Suggestions (Omnibox)";
inline constexpr char kComposeboxRichImageSuggestionsOmniboxDescription[] =
    "Enables richer image suggestions in the Omnibox Composebox for image "
    "generation.";

inline constexpr char kComposeboxPersistentAimButtonRealboxName[] =
    "Composebox Persistent AIM Button (Realbox)";
inline constexpr char kComposeboxPersistentAimButtonRealboxDescription[] =
    "Persists the AIM button in the Realbox Composebox.";

inline constexpr char kComposeboxPersistentAimButtonOmniboxName[] =
    "Composebox Persistent AIM Button (Omnibox)";
inline constexpr char kComposeboxPersistentAimButtonOmniboxDescription[] =
    "Persists the AIM button in the Omnibox Composebox.";

inline constexpr char kComposeboxPersistentAimButtonWithXName[] =
    "Composebox Persistent AIM Button With X";
inline constexpr char kComposeboxPersistentAimButtonWithXDescription[] =
    "Shows an X icon on the persistent AIM button. Only effective if the "
    "persistent AIM button is also enabled for the surface.";

inline constexpr char kComposeboxPersonalizedHintTextName[] =
    "Composebox Personalized Hint Text";
inline constexpr char kComposeboxPersonalizedHintTextDescription[] =
    "Enable Composebox Personalized Hint Text";

inline constexpr char kComposeboxRealboxOutsideClickName[] =
    "Composebox Outside Click (Realbox)";
inline constexpr char kComposeboxRealboxOutsideClickDescription[] =
    "Clicking outside the Realbox Composebox closes the suggestions dropdown "
    "and restores page interaction while keeping the Composebox open.";

inline constexpr char kComposeboxSkillsContextualTasksName[] =
    "Composebox/Searchbox Skills (Contextual Tasks)";
inline constexpr char kComposeboxSkillsContextualTasksDescription[] =
    "Enables skills inside the composebox/searchbox input area for Contextual "
    "Tasks.";

inline constexpr char kComposeboxSkillsNtpName[] =
    "Composebox/Searchbox Skills (NTP)";
inline constexpr char kComposeboxSkillsNtpDescription[] =
    "Enables skills inside the composebox/searchbox input area for NTP.";

inline constexpr char kComposeboxSkillsOmniboxEverywhereName[] =
    "Composebox/Searchbox Skills (Omnibox Everywhere)";
inline constexpr char kComposeboxSkillsOmniboxEverywhereDescription[] =
    "Enables skills inside the composebox/searchbox input area for Omnibox "
    "Everywhere.";

inline constexpr char kComposeboxSkillsOmniboxPopupName[] =
    "Composebox/Searchbox Skills (Omnibox Popup)";
inline constexpr char kComposeboxSkillsOmniboxPopupDescription[] =
    "Enables skills inside the composebox/searchbox input area for Omnibox "
    "Popup.";

inline constexpr char kCompressionDictionaryTransportName[] =
    "Compression dictionary transport";
inline constexpr char kCompressionDictionaryTransportDescription[] =
    "Enables compression dictionary transport.";

inline constexpr char kCompressionDictionaryTTLName[] =
    "Compression dictionary transport ttl";
inline constexpr char kCompressionDictionaryTTLDescription[] =
    "Enables support for the 'ttl' parameter in the 'use-as-dictionary' HTTP "
    "response header.";

inline constexpr char kContextManagementInComposeboxName[] =
    "Context management in the Composebox";
inline constexpr char kContextManagementInComposeboxDescription[] =
    "Enables an updated tab context management UI in the plus button and "
    "composebox context menu.";

inline constexpr char kContextManagementInOmniboxName[] =
    "Context management in the Omnibox";
inline constexpr char kContextManagementInOmniboxDescription[] =
    "Enables context menu and favicon coins for omnibox";

inline constexpr char kContextualCueingName[] = "Contextual cueing";
inline constexpr char kContextualCueingDescription[] =
    "Enables the contextual cueing system to support showing actions.";


inline constexpr char kContextualCueingV2Name[] = "Contextual cueing V2";
inline constexpr char kContextualCueingV2Description[] =
    "Enables the contextual cueing system version 2.";

inline constexpr char kContextualCueingV2MultiSourceName[] =
    "Contextual cueing V2 multi-source";
inline constexpr char kContextualCueingV2MultiSourceDescription[] =
    "Enables parallel evaluation of all registered cue targets and selects the "
    "winner with UCB scoring for contextual cueing V2.";

inline constexpr char kGeicEnabledName[] = "GEiC enabled";
inline constexpr char kGeicEnabledDescription[] =
    "Controls Gemini Enterprise in Chrome (GEiC) side panel integration. Set "
    "to Enabled to apply a GeminiEnterpriseUrl policy even if this "
    "profile is not detected as a managed account. Set to Disabled to opt out "
    "of GEiC and fall back to consumer Gemini, even if a Gemini Enterprise "
    "policy or experiment applies to this profile.";

inline constexpr char kGeicFreName[] = "GEiC first-run experience";
inline constexpr char kGeicFreDescription[] =
    "Enables the Gemini Enterprise in Chrome (GEiC) first-run experience. "
    "Requires GEiC to be enabled.";

inline constexpr char kGeicGuestUrlName[] = "GEiC guest URL";
inline constexpr char kGeicGuestUrlDescription[] =
    "Configures the guest URL loaded into the Gemini Enterprise in Chrome "
    "(GEiC) side panel.";

inline constexpr char kGlassFrameName[] = "Glass Frame";
inline constexpr char kGlassFrameDescription[] =
    "Enables a glass effect on the browser frame.";



inline constexpr char kActorLoginObservationStartDelayName[] =
    "Actor login observation start delay";
inline constexpr char kActorLoginObservationStartDelayDescription[] =
    "Controls how long the actor waits after a login action before it starts "
    "waiting for the page to stabilize. Defaults to 3s; Disabled removes the "
    "delay.";

inline constexpr char kTtcName[] = "TTC";
inline constexpr char kTtcDescription[] = "Enables the TTC feature.";

inline constexpr char kTtcLiveRecipeName[] = "TTC Live Recipe";
inline constexpr char kTtcLiveRecipeDescription[] =
    "Selects the live streaming model recipe for TTC.";

inline constexpr char kTtcFreName[] = "TTC FRE";
inline constexpr char kTtcFreDescription[] =
    "Enables the first run experience for TTC.";

inline constexpr char kActorObserveScreenshotDefaultName[] =
    "Actor observe screenshot default";
inline constexpr char kActorObserveScreenshotDefaultDescription[] =
    "If enabled, the actor framework will capture screenshots by default when "
    "requested.";

inline constexpr char kActorObservePageContentDefaultName[] =
    "Actor observe page content default";
inline constexpr char kActorObservePageContentDefaultDescription[] =
    "If enabled, the actor framework will capture page content by default when "
    "requested.";

inline constexpr char kActorPageStabilityName[] = "Actor page stability";
inline constexpr char kActorPageStabilityDescription[] =
    "Overrides how long the actor waits for the page to stabilize after an "
    "action: the minimum wait and the initial and subsequent paint timeouts. "
    "Each defaults to 1s.";

inline constexpr char kActorScriptToolSkipScreenshotName[] =
    "Actor script tool skip screenshot";
inline constexpr char kActorScriptToolSkipScreenshotDescription[] =
    "If enabled, script tools will explicitly vote to skip screenshots.";

inline constexpr char kActorScriptToolSkipPageContentName[] =
    "Actor script tool skip page content";
inline constexpr char kActorScriptToolSkipPageContentDescription[] =
    "If enabled, script tools will explicitly vote to skip page content.";

inline constexpr char kActorTypeToolObservationStartDelayName[] =
    "Actor type tool observation start delay";
inline constexpr char kActorTypeToolObservationStartDelayDescription[] =
    "Controls how long the actor waits after a type action before it starts "
    "waiting for the page to stabilize. Defaults to 1s; Disabled removes the "
    "delay.";











inline constexpr char kContextualSearchContextualCuesHandleEduName[] =
    "Contextual Search Contextual Cues Handle Edu";
inline constexpr char kContextualSearchContextualCuesHandleEduDescription[] =
    "Enables Contextual Search contextual cues for education pages.";









inline constexpr char kEnterprisePublishedSkillsPolicyEnabledName[] =
    "Enable EnterprisePublishedSkills policy";
inline constexpr char kEnterprisePublishedSkillsPolicyEnabledDescription[] =
    "Enables the EnterprisePublishedSkills policy in Chrome.";

inline constexpr char kSkillsEnabledName[] = "Skills";
inline constexpr char kSkillsEnabledDescription[] =
    "Enables Skills in Gemini in Chrome";

inline constexpr char kSkillsAndExtensionsAppMenuName[] =
    "Skills and Extensions App Menu";
inline constexpr char kSkillsAndExtensionsAppMenuDescription[] =
    "Shows the combined Extensions and Skills sub-menu item in the Chrome app "
    "menu.";

inline constexpr char kForceAiSubscriptionTierName[] =
    "Force AI Subscription Tier";
inline constexpr char kForceAiSubscriptionTierDescription[] =
    "Overrides the AI subscription tier for the profile to a specific tier.";
inline constexpr char kForceAiSubscriptionTier1[] = "Tier 1";
inline constexpr char kForceAiSubscriptionTier2[] = "Tier 2";
inline constexpr char kForceAiSubscriptionTier3[] = "Tier 3";

inline constexpr char kForceColorProfileSRGB[] = "sRGB";
inline constexpr char kForceColorProfileP3[] = "Display P3 D65";
inline constexpr char kForceColorProfileRec2020[] = "ITU-R BT.2020";
inline constexpr char kForceColorProfileColorSpin[] =
    "Color spin with gamma 2.4";
inline constexpr char kForceColorProfileSCRGBLinear[] =
    "scRGB linear (HDR where available)";
inline constexpr char kForceColorProfileHDR10[] = "HDR10 (HDR where available)";

inline constexpr char kForceColorProfileName[] = "Force color profile";
inline constexpr char kForceColorProfileDescription[] =
    "Forces Chrome to use a specific color profile instead of the color "
    "of the window's current monitor, as specified by the operating system.";

inline constexpr char kDarkModeSurfaceTokensName[] = "Dark Mode Surface Tokens";
inline constexpr char kDarkModeSurfaceTokensDescription[] =
    "When enabled, updates the surface tokens for dark mode.";

inline constexpr char kDisableProcessReuse[] = "Disable subframe process reuse";
inline constexpr char kDisableProcessReuseDescription[] =
    "Prevents out-of-process iframes from reusing compatible processes from "
    "unrelated tabs. This is an experimental mode that will result in more "
    "processes being created.";

inline constexpr char kDisallowDocWrittenScriptsUiName[] =
    "Block scripts loaded via document.write";
inline constexpr char kDisallowDocWrittenScriptsUiDescription[] =
    "Disallows fetches for third-party parser-blocking scripts inserted into "
    "the main frame via document.write.";

inline constexpr char kImageDescriptionsAlternateRoutingName[] =
    "Use alternative route for image descriptions.";
inline constexpr char kImageDescriptionsAlternateRoutingDescription[] =
    "When adding automatic captions to images, use a different route to "
    "acquire descriptions.";

inline constexpr char kEnableAccountPreviewDataName[] =
    "Enable Account Preview Data";
inline constexpr char kEnableAccountPreviewDataDescription[] =
    "Enables fetching and storing account preview data for signed-in accounts.";

inline constexpr char kEnableAccountPreviewEntityPreviewsName[] =
    "Enable Account Preview Entity Previews";
inline constexpr char kEnableAccountPreviewEntityPreviewsDescription[] =
    "Enables fetching and storing entity preview data for signed-in accounts.";

inline constexpr char kEnableAccountPreviewPreferredAccountName[] =
    "Enable Account Preview Preferred Account";
inline constexpr char kEnableAccountPreviewPreferredAccountDescription[] =
    "Controls whether storing and exposing the preferred account is enabled.";

inline constexpr char kEnableAccountPreviewPreferredAccountFollowupName[] =
    "Enable Account Preview Preferred Account Followup";
inline constexpr char
    kEnableAccountPreviewPreferredAccountFollowupDescription[] =
        "Controls whether followup features for preferred account preview "
        "(promos, and updated strings) are enabled.";

inline constexpr char kEnableAccountPreviewSwitchingAccountName[] =
    "Enable Account Preview Switching Account";
inline constexpr char kEnableAccountPreviewSwitchingAccountDescription[] =
    "Controls whether storing and exposing the preferred account for "
    "switching is enabled.";

inline constexpr char kEnableAiSubscriptionAvatarRingName[] =
    "Enable AI Subscription Avatar Ring";
inline constexpr char kEnableAiSubscriptionAvatarRingDescription[] =
    "Enables the AI subscription level decorative ring around the user's "
    "avatar.";

inline constexpr char kEnableContentAnalysisClipboardCopyName[] =
    "Enable content analysis clipboard copy";
inline constexpr char kEnableContentAnalysisClipboardCopyDescription[] =
    "Enables content analysis checks for the clipboard copy action.";

inline constexpr char kEnableExperimentalCookieFeaturesName[] =
    "Enable experimental cookie features";
inline constexpr char kEnableExperimentalCookieFeaturesDescription[] =
    "Enable new features that affect setting, sending, and managing cookies. "
    "The enabled features are subject to change at any time.";

inline constexpr char kEnableDelegatedCompositingName[] =
    "Enable delegated compositing";
inline constexpr char kEnableDelegatedCompositingDescription[] =
    "When enabled and applicable, the act of compositing is delegated to the "
    "system compositor.";

inline constexpr char kEnableDesktopQrCodeDetectionName[] =
    "Enable payment QR code detection";
inline constexpr char kEnableDesktopQrCodeDetectionDescription[] =
    "When enabled, QR code detection for facilitated payments will be "
    "supported on Desktop and Android.";

inline constexpr char kEnableProcessIsolationUiName[] =
    "Enable Process Isolation UI";
inline constexpr char kEnableProcessIsolationUiDescription[] =
    "Shows the Enable Process Isolation toggle in chrome://settings/system.";

inline constexpr char kDesktopGlowUpName[] = "Desktop Glow Up";
inline constexpr char kDesktopGlowUpDescription[] =
    "Enables all Desktop Glow Up features.";

inline constexpr char kCollapseTabGroupDuringDragName[] =
    "Collapse Tab Group During Drag";
inline constexpr char kCollapseTabGroupDuringDragDescription[] =
    "Enables programmatically collapsing tab groups while dragging vertical "
    "tabs.";

inline constexpr char kDesktopPWAsAdditionalWindowingControlsName[] =
    "Desktop PWA Additional Windowing Controls";
inline constexpr char kDesktopPWAsAdditionalWindowingControlsDescription[] =
    "Enable PWAs to: (1) manually recreate the minimize, maximize and restore "
    "window functionalities, (2) set windows (non-/)resizable and (3) listen "
    "to window's move events with respective APIs.";

inline constexpr char kDesktopPWAsAppTitleName[] =
    "Desktop PWA Application Title";
inline constexpr char kDesktopPWAsAppTitleDescription[] =
    "Enable PWAs to set a custom title for their windows.";

inline constexpr char kDesktopPWAsElidedExtensionsMenuName[] =
    "Desktop PWAs elided extensions menu";
inline constexpr char kDesktopPWAsElidedExtensionsMenuDescription[] =
    "Moves the Extensions \"puzzle piece\" icon from the title bar into the "
    "app menu for web app windows.";

inline constexpr char kDesktopPWAsTabStripName[] = "Desktop PWA tab strips";
inline constexpr char kDesktopPWAsTabStripDescription[] =
    "Tabbed application mode - enables the `tabbed` display mode which allows "
    "web apps to add a tab strip to their app.";

inline constexpr char kDesktopPWAsTabStripSettingsName[] =
    "Desktop PWA tab strips settings";
inline constexpr char kDesktopPWAsTabStripSettingsDescription[] =
    "Experimental UI for selecting whether a PWA should open in tabbed mode.";

inline constexpr char kDesktopPWAsTabStripCustomizationsName[] =
    "Desktop PWA tab strip customizations";
inline constexpr char kDesktopPWAsTabStripCustomizationsDescription[] =
    "Enable PWAs to customize their tab strip when in tabbed mode by adding "
    "the `tab_strip` manifest field.";

inline constexpr char kDevToolsAriaLiveRecordingName[] =
    "A11y Announcements recording in DevTools";
inline constexpr char kDevToolsAriaLiveRecordingDescription[] =
    "Enables the ARIA-Live and JavaScript announcements recording subpane in "
    "DevTools.";

inline constexpr char kDevToolsEnableDurableMessagesName[] =
    "Preserve HTTP message bodies across navigations for DevTools";
inline constexpr char kDevToolsEnableDurableMessagesDescription[] =
    "Enables a durable message storing mechanism, which allows preserving "
    "HTTP message bodies for debugging.";

inline constexpr char kDevToolsInstrumentationBreakpointsName[] =
    "Instrumentation breakpoints";
inline constexpr char kDevToolsInstrumentationBreakpointsDescription[] =
    "Enables instrumentation breakpoints in DevTools.";

inline constexpr char kDevToolsMobileSafeAreaEmulationName[] =
    "Mobile safe area emulation in DevTools";
inline constexpr char kDevToolsMobileSafeAreaEmulationDescription[] =
    "Enables mobile safe area emulation in DevTools.";

inline constexpr char kDevToolsNetworkBackendLinkingName[] =
    "Network backend linking in DevTools";
inline constexpr char kDevToolsNetworkBackendLinkingDescription[] =
    "Enables backend linking for network requests in DevTools.";

inline constexpr char kDevToolsPlusButtonName[] =
    "Enable the Plus Button in DevTools";
inline constexpr char kDevToolsPlusButtonDescription[] =
    "Shows a Plus Button in main DevTools panel/drawer tabbed locations, "
    "replacing the \"More tools\" overflow menu.";

inline constexpr char kDevToolsProtocolMonitorName[] =
    "Enable protocol monitor in DevTools";
inline constexpr char kDevToolsProtocolMonitorDescription[] =
    "Enables the protocol monitor panel, which displays the Chrome DevTools "
    "Protocol (CDP) traffic between DevTools and the browser.";

inline constexpr char kDevToolsSourceMapScopesInSourcesPanelName[] =
    "Source map scopes in the Sources panel";
inline constexpr char kDevToolsSourceMapScopesInSourcesPanelDescription[] =
    "Enables source map scopes in the DevTools Sources panel.";

inline constexpr char kDomStorageSqliteName[] = "DOM Storage SQLite Backend";
inline constexpr char kDomStorageSqliteDescription[] =
    "Uses a SQLite-powered backing store for local and session storage. No "
    "data is migrated from existing backing stores, including LevelDB stores "
    "or SQLite stores with older schemas; use at your own peril.";

inline constexpr char kEnableTLS13EarlyDataName[] = "TLS 1.3 Early Data";
inline constexpr char kEnableTLS13EarlyDataDescription[] =
    "This option enables TLS 1.3 Early Data, allowing GET requests to be sent "
    "during the handshake when resuming a connection to a compatible TLS 1.3 "
    "server.";

inline constexpr char kApplyClientsideModelPredictionsForPasswordTypesName[] =
    "Apply clientside model predictions for password forms.";
inline constexpr char
    kApplyClientsideModelPredictionsForPasswordTypesDescription[] =
        "Enable using clientside model predictions to fill password forms.";

inline constexpr char kEnableGamepadMultitouchName[] = "Gamepad Multitouch";
inline constexpr char kEnableGamepadMultitouchDescription[] =
    "Enables the ability to receive input from multitouch surface "
    "on the gamepad object.";

inline constexpr char kEnableWindowsGameInputDataFetcherName[] =
    "Enable GameInput data fetcher";
inline constexpr char kEnableWindowsGameInputDataFetcherDescription[] =
    "Enables Gamepad API to use GameInput for gamepad access.";

inline constexpr char kEnableGpuServiceLoggingName[] =
    "Enable gpu service logging";
inline constexpr char kEnableGpuServiceLoggingDescription[] =
    "Enable printing the actual GL driver calls.";

inline constexpr char kEnterpriseIsolatedModeName[] =
    "Force Enterprise Isolated Mode";
inline constexpr char kEnterpriseIsolatedModeDescription[] =
    "Force Enterprise Isolated Mode to replace Incognito mode.";

inline constexpr char kEnableIsolatedWebAppsName[] = "Enable Isolated Web Apps";
inline constexpr char kEnableIsolatedWebAppsDescription[] =
    "Enables experimental support for Isolated Web Apps. "
    "See https://github.com/reillyeon/isolated-web-apps for more information.";

inline constexpr char kDirectSocketsInServiceWorkersName[] =
    "Direct Sockets API in Service Workers";
inline constexpr char kDirectSocketsInServiceWorkersDescription[] =
    "Enables access to the Direct Sockets API in service workers. See "
    "https://github.com/WICG/direct-sockets for details.";

inline constexpr char kDirectSocketsInSharedWorkersName[] =
    "Direct Sockets API in Shared Workers";
inline constexpr char kDirectSocketsInSharedWorkersDescription[] =
    "Enables access to the Direct Sockets API in shared workers. See "
    "https://github.com/WICG/direct-sockets for details.";

inline constexpr char
    kDirectSocketsUdpSendRequireMulticastPermissionPolicyName[] =
        "Direct Sockets UDP send require multicast permission policy";
inline constexpr char
    kDirectSocketsUdpSendRequireMulticastPermissionPolicyDescription[] =
        "Requires the 'direct-sockets-multicast' permissions policy to send or "
        "connect to multicast addresses using the Direct Sockets API.";

inline constexpr char kEnableIsolatedWebAppDevModeName[] =
    "Enable Isolated Web App Developer Mode";
inline constexpr char kEnableIsolatedWebAppDevModeDescription[] =
    "Enables the installation of unverified Isolated Web Apps";

inline constexpr char kEnableIsolatedWebAppFastUpdateCheckName[] =
    "Enable Isolated Web App Fast Update Check";
inline constexpr char kEnableIsolatedWebAppFastUpdateCheckDescription[] =
    "Enables fast update checks for Isolated Web Apps, reducing the update "
    "check interval to 1 minute.";

inline constexpr char kEnableIwaKeyDistributionComponentName[] =
    "Enable the Iwa Key Distribution component";
inline constexpr char kEnableIwaKeyDistributionComponentDescription[] =
    "Enables the Iwa Key Distribution component that supplies key rotation "
    "data for Isolated Web Apps.";

inline constexpr char kIwaKeyDistributionComponentExpCohortName[] =
    "Experimental cohort for the Iwa Key Distribution component";
inline constexpr char kIwaKeyDistributionComponentExpCohortDescription[] =
    "Specifies the experimental cohort for the Iwa Key Distribution component.";
inline constexpr char kIwaKeyDistributionComponentExpCohortStagingName[] =
    "Staging";

inline constexpr char kEnableUnframedIwaName[] =
    "Unframed display mode for Isolated Web Apps";
inline constexpr char kEnableUnframedIwaDescription[] =
    "Enables the unframed display mode for Isolated Web Apps.";

inline constexpr char kEnableControlledFrameName[] = "Enable Controlled Frame";
inline constexpr char kEnableControlledFrameDescription[] =
    "Enables experimental support for Controlled Frame. See "
    "https://github.com/WICG/controlled-frame/blob/main/EXPLAINER.md "
    "for more information.";

inline constexpr char kEnableNetworkLoggingToFileName[] =
    "Enable network logging to file";
inline constexpr char kEnableNetworkLoggingToFileDescription[] =
    "Enables network logging to a file named netlog.json in the user data "
    "directory. The file can be imported into chrome://net-internals.";

inline constexpr char kEnablePerfettoSystemTracingName[] =
    "Enable Perfetto system tracing";
inline constexpr char kEnablePerfettoSystemTracingDescription[] =
    "When enabled, Chrome will attempt to connect to the system tracing "
    "service";

inline constexpr char kOptimizeWebRequestProxyName[] =
    "Optimize WebRequest proxying";
inline constexpr char kOptimizeWebRequestProxyDescription[] =
    "Enables optimization for WebRequest proxying by skipping "
    "unnecessary proxy evaluations when possible.";

inline constexpr char kWebRequestSecurityInfoName[] =
    "Enable SecurityInfo in WebRequest API";
inline constexpr char kWebRequestSecurityInfoDescription[] =
    "Enables SecurityInfo in WebRequest API for extensions, allowing "
    "listeners to retrieve certificate details of web requests.";

inline constexpr char kGamepadRawInputChangeEventName[] =
    "Gamepad Raw Input Change Event";
inline constexpr char kGamepadRawInputChangeEventDescription[] =
    "Enables raw input change events in Gamepad API.";

inline constexpr char kEnableDevtoolsDeepLinkViaExtensibilityApiName[] =
    "Extensibility API support for deep-links within DevTools";
inline constexpr char kEnableDevtoolsDeepLinkViaExtensibilityApiDescription[] =
    "Extends console.timestamp to support adding deep-links into the DevTools "
    "Performance Panel, which (when clicked) call into a DevTools extension";

inline constexpr char kEnableNtpBrowserPromosName[] =
    "Enable new tab page browser feature suggestions";
inline constexpr char kEnableNtpBrowserPromosDescription[] =
    "Shows suggestions to explore browser capabilities (eg. signing in) on the "
    "new tab page.";

inline constexpr char kEnableLensStandaloneName[] =
    "Enable Lens features in Chrome.";
inline constexpr char kEnableLensStandaloneDescription[] =
    "Enables Lens image and region search to learn about the visual content "
    "you see while you browse and shop on the web.";

inline constexpr char kManagedConfigurationRecoverFromFailedFetchesName[] =
    "Managed Configuration API recover from failed fetches";
inline constexpr char
    kManagedConfigurationRecoverFromFailedFetchesDescription[] =
        "Enables retry with exponential backoff and network reconnection "
        "recovery for the Managed Configuration Web API.";

inline constexpr char kEnablePixelCanvasRecordingName[] =
    "Enable pixel canvas recording";
inline constexpr char kEnablePixelCanvasRecordingDescription[] =
    "Pixel canvas recording allows the compositor to raster contents aligned "
    "with the pixel and improves text rendering. This should be enabled when a "
    "device is using fractional scale factor.";

inline constexpr char kEnableProcessPerSiteUpToMainFrameThresholdName[] =
    "Enable ProcessPerSite up to main frame threshold";
inline constexpr char kEnableProcessPerSiteUpToMainFrameThresholdDescription[] =
    "Proactively reuses same-site renderer processes to host multiple main "
    "frames, up to a certain threshold.";

inline constexpr char kAlignPdfDefaultPrintSettingsWithHTMLName[] =
    "Align PDF default print settings with HTML";
inline constexpr char kAlignPdfDefaultPrintSettingsWithHTMLDescription[] =
    "Align the default print settings for PDFs with those for HTML, "
    "including scaling and centering.";

inline constexpr char kBoundaryEventDispatchTracksNodeRemovalName[] =
    "Boundary Event Dispatch Tracks Node Removal";
inline constexpr char kBoundaryEventDispatchTracksNodeRemovalDescription[] =
    "Mouse and Pointer boundary event dispatch (i.e. dispatch of enter, leave, "
    "over, out events) tracks DOM node removal to fix event pairing on "
    "ancestor nodes.";

inline constexpr char
    kEnableResamplingScrollEventsExperimentalPredictionName[] =
        "Enable experimental prediction for scroll events";
inline constexpr char
    kEnableResamplingScrollEventsExperimentalPredictionDescription[] =
        "Predicts the scroll amount after the vsync time to more closely match "
        "when the frame is visible.";

inline constexpr char kExcludePipFromScreenCaptureName[] =
    "Exclude Picture-in-Picture windows from screen capture";
inline constexpr char kExcludePipFromScreenCaptureDescription[] =
    "When enabled, Picture-in-Picture windows will be excluded from screen "
    "captures.";

inline constexpr char kExperimentalWebAssemblyFeaturesName[] =
    "Experimental WebAssembly";
inline constexpr char kExperimentalWebAssemblyFeaturesDescription[] =
    "Enable web pages to use experimental WebAssembly features.";

inline constexpr char kExperimentalWebAssemblySharedEverythingName[] =
    "Experimental WebAssembly Shared-Everything Threads";
inline constexpr char kExperimentalWebAssemblySharedEverythingDescription[] =
    "Enable web pages to use the experimental WebAssembly Shared-Everything "
    "Threads feature. Note that this only covers a subset of the proposal.";

inline constexpr char kEnableUnrestrictedUsbName[] =
    "Enable Isolated Web Apps to bypass USB restrictions";
inline constexpr char kEnableUnrestrictedUsbDescription[] =
    "When enabled, allows Isolated Web Apps to access blocklisted "
    "devices and protected interfaces through WebUSB API.";

inline constexpr char kEnableWasmBaselineName[] =
    "WebAssembly baseline compiler";
inline constexpr char kEnableWasmBaselineDescription[] =
    "Enables WebAssembly baseline compilation and tier up.";

inline constexpr char kEnableWasmLazyCompilationName[] =
    "WebAssembly lazy compilation";
inline constexpr char kEnableWasmLazyCompilationDescription[] =
    "Enables lazy (JIT on first call) compilation of WebAssembly modules.";

inline constexpr char kEnableWasmStackSwitchingName[] =
    "WebAssembly stack switching";
inline constexpr char kEnableWasmStackSwitchingDescription[] =
    "Enables WasmFX.";

inline constexpr char kEnableWasmTieringName[] = "WebAssembly tiering";
inline constexpr char kEnableWasmTieringDescription[] =
    "Enables tiered compilation of WebAssembly (will tier up to TurboFan if "
    "#enable-webassembly-baseline is enabled).";

inline constexpr char kExperimentalWebPlatformFeaturesName[] =
    "Experimental Web Platform features";
inline constexpr char kExperimentalWebPlatformFeaturesDescription[] =
    "Enables experimental Web Platform features that are in development.";

inline constexpr char kSafeBrowsingLocalListsUseSBv5Name[] =
    "Safe Browsing Local Lists use v5 API";
inline constexpr char kSafeBrowsingLocalListsUseSBv5Description[] =
    "Fetch and check local lists using the Safe Browsing v5 API instead of the "
    "v4 Update API.";

inline constexpr char kSafeBrowsingNoProtectionTemporaryName[] =
    "Safe Browsing Temporary No Protection";
inline constexpr char kSafeBrowsingNoProtectionTemporaryDescription[] =
    "Enables temporary 24-hour Safe Browsing opt-out mode with automatic "
    "reversion to the previous protection level.";

inline constexpr char kSafeBrowsingNoProtectionSuppressWarningsName[] =
    "Safe Browsing Suppress Dangerous File Warnings in No Protection";
inline constexpr char kSafeBrowsingNoProtectionSuppressWarningsDescription[] =
    "Suppresses fallback heuristic dangerous file warnings when Safe Browsing "
    "is in No Protection mode.";

inline constexpr char kXSLTName[] = "XSLT";
inline constexpr char kXSLTDescription[] =
    "Toggles whether or not XSLT is supported by the browser.";

inline constexpr char kSymphoniaAudioDecodingName[] =
    "Symphonia Audio Decoding";
inline constexpr char kSymphoniaAudioDecodingDescription[] =
    "Enables using the experimental Symphonia audio decoder instead of using "
    "FFMPEG for decoding audio.";

inline constexpr char kSymphoniaDemuxingName[] = "Symphonia Demuxing";
inline constexpr char kSymphoniaDemuxingDescription[] =
    "Enables using the experimental Symphonia container demuxer instead of "
    "using FFmpeg for demuxing media (AAC, FLAC, MP4/ISOM, Matroska/WebM, "
    "MP3, Ogg, and RIFF/WAV).";

inline constexpr char kEnableWebHidInWebViewName[] = "Web HID in WebView";
inline constexpr char kEnableWebHidInWebViewDescription[] =
    "Enable WebViews to access Web HID upon embedder's permission.";

inline constexpr char kExperimentalOmniboxLabsName[] =
    "Enable extension permission omnibox.directInput";
inline constexpr char kExperimentalOmniboxLabsDescription[] =
    "Allows extensions to request permission omnibox.directInput, which "
    "enables unscoped mode in the Omnibox";

inline constexpr char kExportPlatformPoliciesJsonName[] =
    "Export Platform Policies as JSON";
inline constexpr char kExportPlatformPoliciesJsonDescription[] =
    "Enables a button on chrome://policy to export platform-source policies in "
    "a custom flat JSON configuration schema.";

inline constexpr char kExtensionAiDataCollectionName[] =
    "Enables AI Data collection via extension";
inline constexpr char kExtensionAiDataCollectionDescription[] =
    "Enables an extension API to allow specific extensions to collect data "
    "from browser process. This data may contain profile specific information "
    " and may be otherwise unavailable to an extension.";
inline constexpr char kExtensionTabContextMenuName[] =
    "Extension Tab Context Menu";
inline constexpr char kExtensionTabContextMenuDescription[] =
    "Allowing extensions to add custom items when right-clicking a tab.";

inline constexpr char
    kExtensionTelemetryEnterpriseShortReportingIntervalName[] =
        "Extension Telemetry Enterprise Short Reporting Interval";
inline constexpr char
    kExtensionTelemetryEnterpriseShortReportingIntervalDescription[] =
        "When enabled, reduces the extension telemetry service's enterprise "
        "telemetry reporting interval to 30 seconds.";

inline constexpr char kExtensionsCollapseMainMenuName[] =
    "Collapse Extensions Submenu";
inline constexpr char kExtensionsCollapseMainMenuDescription[] =
    "Enables a mode where if the current profile has no extensions, the "
    "extensions submenu in the application menu is replaced by a single item, "
    "e.g. \"Explore Extensions\".";
inline constexpr char kExtensionInstallSiteAccessOptionsName[] =
    "Extension Install Site Access Options";
inline constexpr char kExtensionInstallSiteAccessOptionsDescription[] =
    "Enables an 'Allow site access' section in the extension install dialog to "
    "let users choose default site permissions on install.";

inline constexpr char kExtensionsMenuAccessControlName[] =
    "Extensions Menu Access Control";
inline constexpr char kExtensionsMenuAccessControlDescription[] =
    "Enables a redesigned extensions menu that allows the user to control "
    "extensions site access.";
inline constexpr char kIPHExtensionsMenuFeatureName[] = "IPH Extensions Menu";
inline constexpr char kIPHExtensionsMenuFeatureDescription[] =
    "Enables In-Product-Help that appears when at least one extension has "
    "access to the current page. This feature is gated by "
    "extensions-menu-access-control.";
inline constexpr char kIPHExtensionsRequestAccessButtonFeatureName[] =
    "IPH Extensions Request Access Button Feature";
inline constexpr char kIPHExtensionsRequestAccessButtonFeatureDescription[] =
    "Enables In-Product-Help that appears when at least one extension is "
    "requesting access to the current page. This feature is gated by "
    "extensions-menu-access-control.";

inline constexpr char kCWSInfoFastCheckName[] = "CWS Info Fast Check";
inline constexpr char kCWSInfoFastCheckDescription[] =
    "When enabled, Chrome checks and fetches metadata for installed extensions "
    "more frequently.";

inline constexpr char kCWSReviewPromptingNativeUIName[] =
    "CWS Review Prompting Native UI";
inline constexpr char kCWSReviewPromptingNativeUIDescription[] =
    "Enables native UI affordances (e.g., in the Extensions menu and "
    "management page) for leaving reviews on installed Chrome Web Store "
    "extensions.";

inline constexpr char kExtensionsOnChromeUrlsName[] =
    "Extensions on chrome:// URLs";
inline constexpr char kExtensionsOnChromeUrlsDescription[] =
    "Enables running extensions on chrome:// URLs, where extensions explicitly "
    "request this permission.";

inline constexpr char kExtensionsOnExtensionUrlsName[] =
    "Extensions on chrome-extension:// URLs";
inline constexpr char kExtensionsOnExtensionUrlsDescription[] =
    "Enables running extensions on chrome-extension:// URLs, where extensions "
    "explicitly request this permission.";

inline constexpr char kFractionalScrollOffsetsName[] =
    "Fractional Scroll Offsets";
inline constexpr char kFractionalScrollOffsetsDescription[] =
    "Enables fractional scroll offsets inside Blink, exposing non-integer "
    "offsets to web APIs.";

inline constexpr char kFedCmAutofillName[] = "FedCmAutofill";
inline constexpr char kFedCmAutofillDescription[] =
    "Allows RPs to enhance autofill with FedCM.";

inline constexpr char kFedCmAmbientBubbleName[] = "FedCM Ambient Bubble";
inline constexpr char kFedCmAmbientBubbleDescription[] =
    "Shows FedCM accounts for passive mode requests in an ambient bubble, "
    "instead of the regular FedCM dialog.";

inline constexpr char kFedCmDelegationName[] = "FedCM with delegation support";
inline constexpr char kFedCmDelegationDescription[] =
    "Enables IdPs to delegate presentation to the browser.";

inline constexpr char kEmailVerificationProtocolName[] =
    "Email Verification Protocol";
inline constexpr char kEmailVerificationProtocolDescription[] =
    "Enables the Email Verification Protocol in Autofill.";

inline constexpr char kFedCmActiveModeMultipleIdentityProvidersName[] =
    "FedCmActiveModeMultipleIdentityProviders";
inline constexpr char kFedCmActiveModeMultipleIdentityProvidersDescription[] =
    "Enables Active Mode support with multiple Identity Providers in FedCM.";

inline constexpr char kFedCmIdpLinksName[] = "FedCM IdP links";
inline constexpr char kFedCmIdpLinksDescription[] =
    "When FedCM asks for permission to share data with a site, shows links to "
    "the identity provider's account management page, privacy policy and "
    "terms of service, from the account_management_url, privacy_policy_url and "
    "terms_of_service_url keys of its config file.";

inline constexpr char kFedCmIdPRegistrationName[] =
    "FedCM with IdP Registration support";
inline constexpr char kFedCmIdPRegistrationDescription[] =
    "Enables RPs to get identity credentials from registered IdPs.";

inline constexpr char kFedCmLightweightModeName[] = "FedCmLightweightMode";
inline constexpr char kFedCmLightweightModeDescription[] =
    "Enables IdPs to store user profile information using the login status "
    "API.";

inline constexpr char kFedCmMetricsEndpointName[] = "FedCmMetricsEndpoint";
inline constexpr char kFedCmMetricsEndpointDescription[] =
    "Allows the FedCM API to send performance measurement to the metrics "
    "endpoint on the identity provider side. Requires FedCM to be enabled.";

inline constexpr char kFedCmNonceInParamsName[] = "FedCmNonceInParams";
inline constexpr char kFedCmNonceInParamsDescription[] =
    "Removes nonce as an explicit parameter of the FedCM API. When enabled, a "
    "nonce may be passed in params.";

inline constexpr char kFedCmWebIdentitySubdomainName[] =
    "FedCmWebIdentitySubdomain";
inline constexpr char kFedCmWebIdentitySubdomainDescription[] =
    "Enables subdomain-first discovery for the FedCM well-known file. "
    "Fetches https://web-identity.<eTLD+1>/.well-known/web-identity first "
    "and falls back to the apex URL on failure.";

inline constexpr char kFedCmWellKnownEndpointValidationName[] =
    "FedCmWellKnownEndpointValidation";
inline constexpr char kFedCmWellKnownEndpointValidationDescription[] =
    "When enabled, accounts_endpoint and login_url must be present in "
    ".well-known/web-identity if client_metadata is used.";

inline constexpr char kFedCmWithoutWellKnownEnforcementName[] =
    "FedCmWithoutWellKnownEnforcement";
inline constexpr char kFedCmWithoutWellKnownEnforcementDescription[] =
    "Supports configURL that's not in the IdP's .well-known file.";

inline constexpr char kFedCmSegmentationPlatformName[] =
    "FedCmSegmentationPlatform";
inline constexpr char kFedCmSegmentationPlatformDescription[] =
    "Enables the segmentation platform service to provide UI volume "
    "recommendations to FedCM.";

inline constexpr char kFedCmNavigationInterceptionName[] =
    "FedCmNavigationInterception";
inline constexpr char kFedCmNavigationInterceptionDescription[] =
    "Allows IdP to intercept navigations by initiating a FedCM request.";

inline constexpr char kWebIdentityDigitalCredentialsName[] =
    "DigitalCredentials";
inline constexpr char kWebIdentityDigitalCredentialsDescription[] =
    "Enables the three-party verifier/holder/issuer identity model.";

inline constexpr char kWebIdentityDigitalCredentialsCreationName[] =
    "DigitalCredentialsCreation";
inline constexpr char kWebIdentityDigitalCredentialsCreationDescription[] =
    "Enables the Digital Credentials Creation API.";

inline constexpr char kFillOnAccountSelectName[] =
    "Fill passwords on account selection";
inline constexpr char kFillOnAccountSelectDescription[] =
    "Filling of passwords when an account is explicitly selected by the user "
    "rather than autofilling credentials on page load.";

inline constexpr char kEnterpriseSigninDialogsRefreshName[] =
    "Enterprise Signin Dialogs Refresh";
inline constexpr char kEnterpriseSigninDialogsRefreshDescription[] =
    "Enables the visually refreshed enterprise signin dialogs (enterprise "
    "profile welcome/creation, OIDC and the device signals disclaimer). It is "
    "no-op if `first-run-desktop-refresh` is disabled.";

inline constexpr char kFirstRunDesktopChoiceScreenRefreshName[] =
    "First Run Desktop Choice Screen Refresh";
inline constexpr char kFirstRunDesktopChoiceScreenRefreshDescription[] =
    "Enables the visually refreshed first run and profile creation flows, "
    "including the choice screen (FRE and dialog versions), for "
    "users in search engine choice screen regions. It is no-op if "
    "`first-run-desktop-refresh` is disabled.";

inline constexpr char kFirstRunDesktopRefreshName[] =
    "First Run Desktop Refresh";
inline constexpr char kFirstRunDesktopRefreshDescription[] =
    "Enables the visually refreshed first run and profile creation flows in "
    "users outside of search engine choice screen regions. To enable the "
    "refresh in search engine choice screen regions, enable "
    "`first-run-desktop-choice-screen-refresh` as well.";

inline constexpr char kFirstRunDesktopRevampName[] = "First Run Desktop Revamp";
inline constexpr char kFirstRunDesktopRevampDescription[] =
    "Enables the first run revamp (new UI and additional effects). It is no-op "
    "if `first-run-desktop-refresh` is disabled.";

inline constexpr char kForceTextDirectionName[] = "Force text direction";
inline constexpr char kForceTextDirectionDescription[] =
    "Explicitly force the per-character directionality of UI text to "
    "left-to-right (LTR) or right-to-left (RTL) mode, overriding the default "
    "direction of the character language.";
inline constexpr char kForceDirectionLtr[] = "Left-to-right";
inline constexpr char kForceDirectionRtl[] = "Right-to-left";

inline constexpr char kForceUiDirectionName[] = "Force UI direction";
inline constexpr char kForceUiDirectionDescription[] =
    "Explicitly force the UI to left-to-right (LTR) or right-to-left (RTL) "
    "mode, overriding the default direction of the UI language.";

inline constexpr char kCastMessageLoggingName[] =
    "Enables logging of all Cast messages.";
inline constexpr char kCastMessageLoggingDescription[] =
    "Enables logging of all messages exchanged between websites, Chrome, "
    "and Cast receivers in chrome://media-router-internals.";

inline constexpr char kCastStreaming60fpsName[] = "Cast Streaming 60fps";
inline constexpr char kCastStreaming60fpsDescription[] =
    "Enables Cast Streaming at 60 frames per second.";

inline constexpr char kCastStreamingAv1Name[] =
    "Enable AV1 video encoding for Cast Streaming";
inline constexpr char kCastStreamingAv1Description[] =
    "Offers the AV1 video codec when negotiating Cast Streaming, and uses AV1 "
    "if selected for the session.";

inline constexpr char kCastStreamingHardwareAv1Name[] =
    "Toggle hardware accelerated AV1 video encoding for Cast Streaming";
inline constexpr char kCastStreamingHardwareAv1Description[] =
    "The default is to allow hardware AV1 encoding when recommended for the "
    "platform. If enabled, hardware AV1 encoding will always be allowed when "
    "supported by the platform. If disabled, hardware AV1 encoding will "
    "never be used.";

inline constexpr char kCastStreamingHardwareH264Name[] =
    "Toggle hardware accelerated H.264 video encoding for Cast Streaming";
inline constexpr char kCastStreamingHardwareH264Description[] =
    "The default is to allow hardware H.264 encoding when recommended for the "
    "platform. If enabled, hardware H.264 encoding will always be allowed when "
    "supported by the platform. If disabled, hardware H.264 encoding will "
    "never be used.";

inline constexpr char kCastStreamingHardwareHevcName[] =
    "Toggle hardware accelerated HEVC video encoding for Cast Streaming";
inline constexpr char kCastStreamingHardwareHevcDescription[] =
    "The default is to allow hardware HEVC encoding when recommended for the "
    "platform. If enabled, hardware HEVC encoding will always be allowed when "
    "supported by the platform. If disabled, hardware HEVC encoding will "
    "never be used.";

inline constexpr char kCastStreamingHardwareVp8Name[] =
    "Toggle hardware accelerated VP8 video encoding for Cast Streaming";
inline constexpr char kCastStreamingHardwareVp8Description[] =
    "The default is to allow hardware VP8 encoding when recommended for the "
    "platform. If enabled, hardware VP8 encoding will always be allowed when "
    "supported by the platform (regardless of recommendation). If disabled, "
    "hardware VP8 encoding will never be used.";

inline constexpr char kCastStreamingHardwareVp9Name[] =
    "Toggle hardware accelerated VP9 video encoding for Cast Streaming";
inline constexpr char kCastStreamingHardwareVp9Description[] =
    "The default is to allow hardware VP9 encoding when recommended for the "
    "platform. If enabled, hardware VP9 encoding will always be allowed when "
    "supported by the platform (regardless of recommendation). If disabled, "
    "hardware VP9 encoding will never be used.";

inline constexpr char kCastStreamingOfferHardwareFirstName[] =
    "Enables Cast Streaming to offer only hardware accelerated codecs first";
inline constexpr char kCastStreamingOfferHardwareFirstDescription[] =
    "When enabled, the Cast Streaming session negotiates only the hardware "
    "accelerated codecs first, and only offers software encoding if the "
    "receiver rejects the initial offer.";

inline constexpr char kCastStreamingPerformanceOverlayName[] =
    "Toggle a performance metrics overlay while Cast Streaming";
inline constexpr char kCastStreamingPerformanceOverlayDescription[] =
    "When enabled, a text overlay is rendered on top of each frame sent while "
    "Cast Streaming that includes frame duration, resolution, timestamp, "
    "low latency mode, capture duration, target playout delay, target bitrate, "
    "and encoder utilization.";

inline constexpr char kCastStreamingVp8Name[] =
    "Enable VP8 video encoding for Cast Streaming";
inline constexpr char kCastStreamingVp8Description[] =
    "Offers the VP8 video codec when negotiating Cast Streaming, and uses VP8 "
    "if selected for the session. If true, software VP8 encoding will be "
    "offered and hardware VP8 encoding may be offered if enabled and available "
    "on this platform. If false, software VP8 will not be offered and hardware "
    "VP8 will only be offered if #cast-streaming-hardware-vp8 is explicitly "
    "set to true.";

inline constexpr char kCastStreamingVp9Name[] =
    "Enable VP9 video encoding for Cast Streaming";
inline constexpr char kCastStreamingVp9Description[] =
    "Offers the VP9 video codec when negotiating Cast Streaming, and uses VP9 "
    "if selected for the session.";

inline constexpr char kCastStreamingMaxVideoBitrateName[] =
    "Cast Streaming Max Video Bitrate Limit";
inline constexpr char kCastStreamingMaxVideoBitrateDescription[] =
    "Enables configuring the maximum video bitrate limit for Cast Streaming "
    "to experiment with different network quality ceilings.";

inline constexpr char kCastStreamingExponentialVideoBitrateAlgorithmName[] =
    "Cast Streaming Exponential Video Bitrate Algorithm";
inline constexpr char
    kCastStreamingExponentialVideoBitrateAlgorithmDescription[] =
        "Enables the exponential video bitrate algorithm for Cast Streaming. "
        "This algorithm uses an exponential increase and decrease factor to "
        "adjust the bitrate based on network conditions.";

inline constexpr char kCastStreamingMacHardwareH264Name[] =
    "Enable hardware H264 video encoding on for Cast Streaming on macOS";
inline constexpr char kCastStreamingMacHardwareH264Description[] =
    "Offers the H264 video codec when negotiating Cast Streaming, and uses "
    "hardware-accelerated H264 encoding if selected for the session";

inline constexpr char kCastStreamingWinHardwareH264Name[] =
    "Enable hardware H264 video encoding on for Cast Streaming on Windows";
inline constexpr char kCastStreamingWinHardwareH264Description[] =
    "Offers the H264 video codec when negotiating Cast Streaming, and uses "
    "hardware-accelerated H264 encoding if selected for the session";

inline constexpr char kCastEnableStreamingWithHiDPIName[] =
    "HiDPI tab capture support for Cast Streaming";
inline constexpr char kCastEnableStreamingWithHiDPIDescription[] =
    "Enables HiDPI tab capture during Cast Streaming mirroring sessions. May "
    "reduce performance on some platforms and also improve quality of video "
    "frames.";

inline constexpr char kChromeWebStoreNavigationThrottleName[] =
    "Chrome Web Store navigation throttle";
inline constexpr char kChromeWebStoreNavigationThrottleDescription[] =
    "When enabled, passes DM Token to the Chrome Web Store.";

inline constexpr char kChromeDarkNeutrals26Name[] = "Chrome Dark Neutrals 26";
inline constexpr char kChromeDarkNeutrals26Description[] =
    "When enabled, updates the neutral palette for dark mode default theme";

inline constexpr char kGpuRasterizationName[] = "GPU rasterization";
inline constexpr char kGpuRasterizationDescription[] =
    "Use GPU to rasterize web content.";

inline constexpr char kHappyEyeballsV3Name[] = "Happy Eyeballs Version 3";
inline constexpr char kHappyEyeballsV3Description[] =
    "Enables the Happy Eyeballs Version 3 algorithm. See "
    "https://datatracker.ietf.org/doc/draft-pauly-v6ops-happy-eyeballs-v3/";

inline constexpr char kHardwareMediaKeyHandling[] =
    "Hardware Media Key Handling";
inline constexpr char kHardwareMediaKeyHandlingDescription[] =
    "Enables using media keys to control the active media session. This "
    "requires MediaSessionService to be enabled too";

inline constexpr char kHeavyAdPrivacyMitigationsName[] =
    "Heavy ad privacy mitigations";
inline constexpr char kHeavyAdPrivacyMitigationsDescription[] =
    "Enables privacy mitigations for the heavy ad intervention. Disabling "
    "this makes the intervention deterministic. Defaults to enabled.";

inline constexpr char kTabAudioMutingName[] = "Tab audio muting UI control";
inline constexpr char kTabAudioMutingDescription[] =
    "When enabled, the audio indicators in the tab strip double as tab audio "
    "mute controls.";

inline constexpr char kStartupLaunchName[] =
    "Enable launching Chrome on Windows startup";

inline constexpr char kStartupLaunchDescription[] =
    "When enabled, Launch Chrome on Startup can be turned on from Settings";

inline constexpr char kStartupLaunchInfoBarName[] =
    "Enable Launch Chrome on Startup prompt";

inline constexpr char kStartupLaunchInfoBarDescription[] =
    "When enabled, shows an infobar prompting to launch Chrome on Windows "
    "startup";

inline constexpr char kHttpsFirstBalancedModeName[] =
    "Allow enabling Balanced Mode for HTTPS-First Mode.";
inline constexpr char kHttpsFirstBalancedModeDescription[] =
    "Enable tri-state HTTPS-First Mode setting in chrome://settings/security.";

inline constexpr char kHttpsFirstDialogUiName[] =
    "Dialog UI for HTTPS-First Modes";
inline constexpr char kHttpsFirstDialogUiDescription[] =
    "Use a dialog-based UI for HFM";

inline constexpr char kHttpsFirstModeIncognitoName[] =
    "HTTPS-First Mode in Incognito";
inline constexpr char kHttpsFirstModeIncognitoDescription[] =
    "Enable HTTPS-First Mode in Incognito as default setting.";

inline constexpr char kHttpsFirstModeV2ForEngagedSitesName[] =
    "HTTPS-First Mode V2 For Engaged Sites";
inline constexpr char kHttpsFirstModeV2ForEngagedSitesDescription[] =
    "Enable Site-Engagement based HTTPS-First Mode. Shows HTTPS-First Mode "
    "interstitial on sites whose HTTPS URLs have high Site Engagement scores. "
    "Requires #https-upgrades feature to be enabled";

inline constexpr char kHttpsFirstModeForTypicallySecureUsersName[] =
    "HTTPS-First Mode For Typically Secure Users";
inline constexpr char kHttpsFirstModeForTypicallySecureUsersDescription[] =
    "Automatically enables HTTPS-First Mode if the user has a typically secure "
    "browsing pattern.";

inline constexpr char kHttpsUpgradesName[] = "HTTPS Upgrades";
inline constexpr char kHttpsUpgradesDescription[] =
    "Enable automatically upgrading all top-level navigations to HTTPS with "
    "fast fallback to HTTP.";

inline constexpr char
    kHttpsUpgradesTypedSchemelessNavigationNoTimeoutFallbackName[] =
        "HTTPS Upgrades Typed Schemeless Navigation No Timeout Fallback";
inline constexpr char
    kHttpsUpgradesTypedSchemelessNavigationNoTimeoutFallbackDescription[] =
        "When enabled, typed schemeless navigations upgraded to HTTPS will not "
        "fallback to HTTP if the upgraded HTTPS navigation fails due to a "
        "timeout.";

inline constexpr char kIgnoreGpuBlocklistName[] =
    "Override software rendering list";
inline constexpr char kIgnoreGpuBlocklistDescription[] =
    "Overrides the built-in software rendering list and enables "
    "GPU-acceleration on unsupported system configurations.";

inline constexpr char kImportExportFlagsName[] = "Import and export flags";
inline constexpr char kImportExportFlagsDescription[] =
    "Enables importing and exporting flags from chrome://flags.";

inline constexpr char kInfoBarQueuedCountPillName[] =
    "InfoBar queued count pill";
inline constexpr char kInfoBarQueuedCountPillDescription[] =
    "Shows a \"+N\" count pill in the bottom-most InfoBar while lower-priority "
    "InfoBars wait in the prioritization queue.";

inline constexpr char kInProductHelpDemoModeChoiceName[] =
    "In-Product Help Demo Mode";
inline constexpr char kInProductHelpDemoModeChoiceDescription[] =
    "Selects the In-Product Help demo mode.";

inline constexpr char kInStockNotificationName[] =
    "Enable the In Stock Notification Manager";
inline constexpr char kInStockNotificationDescription[] =
    "Enables detecting, showing, and subscribing to back-in-stock "
    "notifications on eligible product pages.";

inline constexpr char kJavascriptHarmonyName[] = "Experimental JavaScript";
inline constexpr char kJavascriptHarmonyDescription[] =
    "Enable web pages to use experimental JavaScript features.";

inline constexpr char kJavascriptHarmonyShippingName[] =
    "Latest stable JavaScript features";
inline constexpr char kJavascriptHarmonyShippingDescription[] =
    "Some web pages use legacy or non-standard JavaScript extensions that may "
    "conflict with the latest JavaScript features. This flag allows disabling "
    "support of those features for compatibility with such pages.";

inline constexpr char kExtractRelatedSearchesFromPrefetchedZPSResponseName[] =
    "Extract Related Searches from Prefetched ZPS Response";
inline constexpr char
    kExtractRelatedSearchesFromPrefetchedZPSResponseDescription[] =
        "Enables page annotation logic to source related searches data from "
        "prefetched ZPS responses";

inline constexpr char kLensComposeboxIdentityDelegationName[] =
    "Lens Composebox Identity Delegation";
inline constexpr char kLensComposeboxIdentityDelegationDescription[] =
    "Enables identity delegation in the Lens composebox.";















inline constexpr char kLensAimSuggestionsName[] =
    "Lens AIM M3 Side Panel Suggestions";
inline constexpr char kLensAimSuggestionsDescription[] =
    "Enables suggestions in the Lens composebox. This will have an effect "
    "only when the Lens search AIM M3 flag is also enabled.";

inline constexpr char kLensAimSuggestionsGradientBackgroundName[] =
    "Lens AIM M3 Side Panel Suggestions Gradient Background";
inline constexpr char kLensAimSuggestionsGradientBackgroundDescription[] =
    "Enables a gradient background for the Lens composebox dropdown that does "
    "not cover the entire side panel.";


inline constexpr char kLensVideoCitationsName[] = "Lens video citations";
inline constexpr char kLensVideoCitationsDescription[] =
    "Enables special handling for video citations in Lens.";

inline constexpr char kLensUpdatedFeedbackEntrypointName[] =
    "Lens updated feedback entrypoint";
inline constexpr char kLensUpdatedFeedbackEntrypointDescription[] =
    "Enables an updated feedback entry point in the Lens side panel.";

inline constexpr char kLensUseSeparateRequestIdForViewportImagesName[] =
    "Lens Use Separate Request ID for Viewport Images";
inline constexpr char kLensUseSeparateRequestIdForViewportImagesDescription[] =
    "Enables using separate request IDs for page contents vs page viewport "
    "uploads.";

inline constexpr char kLensSendDriveTabUrlsInComposeboxesName[] =
    "Lens Send Drive Tab URLs in Composeboxes";
inline constexpr char kLensSendDriveTabUrlsInComposeboxesDescription[] =
    "Enables sending Google Drive / Docs tab URLs as unresolved URLs in "
    "composeboxes.";

inline constexpr char kLensSidePanelUnificationName[] =
    "Lens Side Panel Unification";
inline constexpr char kLensSidePanelUnificationDescription[] =
    "Unifies the Lens and Contextual Tasks side panels into a single panel.";

inline constexpr char kLensBypassCompressionForC2paName[] =
    "Lens Bypass Compression for C2PA";
inline constexpr char kLensBypassCompressionForC2paDescription[] =
    "Bypasses image downscaling and compression if a C2PA metadata urn is "
    "detected, ensuring provenance is preserved during upload. Works across "
    "all surfaces.";

inline constexpr char kMacAccessibilityTextOperationName[] =
    "Enable text editing through accessibility APIs";
inline constexpr char kMacAccessibilityTextOperationDescription[] =
    "Enables support for performing text editing operations through "
    "the AXTextOperation accessibility API.";

inline constexpr char kMediaCaptureElementPreviewName[] =
    "Media capture element preview";
inline constexpr char kMediaCaptureElementPreviewDescription[] =
    "Enables camera and microphone preview for <camera>, <microphone>, and "
    "<usermedia> elements.";

inline constexpr char kMediaRouterCastAllowAllIPsName[] =
    "Connect to Cast devices on all IP addresses";
inline constexpr char kMediaRouterCastAllowAllIPsDescription[] =
    "Have the Media Router connect to Cast devices on all IP addresses, not "
    "just RFC1918/RFC4193 private addresses.";

inline constexpr char kMigrateSyncingUserToSignedInName[] =
    "Migrate syncing user to signed in state";
inline constexpr char kMigrateSyncingUserToSignedInDescription[] =
    "When enabled, a syncing user is migrated to the signed in non-syncing "
    "state on the next browser startup.";

inline constexpr char kMobilePromoOnDesktopForcePromoTypeName[] =
    "Force iOS Promo Type";
inline constexpr char kMobilePromoOnDesktopForcePromoTypeDescription[] =
    "Force the iOS promo to be a specific type (QRCode or Reminder) for "
    "testing purposes.";

inline constexpr char kMobileNTPPromoOnDesktopName[] =
    "Mobile NTP Promo on Desktop";
inline constexpr char kMobileNTPPromoOnDesktopDescription[] =
    "Enables the Mobile NTP Promo on Desktop.";

inline constexpr char kMobilePromoOnDesktopWithQRCodeName[] =
    "Mobile Promo On Desktop - QRCode";
inline constexpr char kMobilePromoOnDesktopWithQRCodeDescription[] =
    "When enabled, shows a mobile promo with a QR code on desktop for eligible "
    "users.";

inline constexpr char kMobilePromoOnDesktopWithQRCodeWave1Name[] =
    "Mobile Promo On Desktop - QR Code (Wave 1)";

inline constexpr char kMobilePromoOnDesktopWithQRCodeWave1Description[] =
    "When enabled, shows a mobile promo with a QR code on desktop for eligible "
    "users. This version highlights features not included in the existing "
    "Mobile promos.";

inline constexpr char kMobilePromoOnDesktopWithReminderName[] =
    "Mobile Promo On Desktop - Reminder";
inline constexpr char kMobilePromoOnDesktopWithReminderDescription[] =
    "When enabled, shows a mobile promo with a reminder flow on desktop for "
    "eligible users.";

inline constexpr char kMobilePromoOnDesktopWithReminderWave1Name[] =
    "Mobile Promo On Desktop - Reminder (Wave 1)";

inline constexpr char kMobilePromoOnDesktopWithReminderWave1Description[] =
    "When enabled, shows a mobile promo with a reminder flow on desktop for "
    "eligible users. This version highlights features not included in the "
    "existing mobile promos.";

inline constexpr char kMobilePromoOnDesktopWithQRCodeWave2Name[] =
    "Mobile Promo On Desktop - QR Code (Wave 2)";

inline constexpr char kMobilePromoOnDesktopWithQRCodeWave2Description[] =
    "When enabled, shows a mobile promo with a QR code on desktop for eligible "
    "users. This version highlights thematic areas as opposed to specific "
    "features.";

inline constexpr char kMobilePromoOnDesktopWithReminderWave2Name[] =
    "Mobile Promo On Desktop - Reminder (Wave 2)";

inline constexpr char kMobilePromoOnDesktopWithReminderWave2Description[] =
    "When enabled, shows a mobile promo with a reminder flow on desktop for "
    "eligible users. This version highlights thematic areas as opposed to "
    "specific features.";

inline constexpr char kMostVisitedTilesNewScoringName[] =
    "Most Visited Tile: New scoring function";
inline constexpr char kMostVisitedTilesNewScoringDescription[] =
    "When showing MV tiles, use a new scoring function to compute the score of "
    "each segment.";

inline constexpr char kMenuSimplificationName[] = "Menu Simplification";
inline constexpr char kMenuSimplificationDescription[] =
    "Enables the menu simplification feature.";

inline constexpr char kSourceSpecificMulticastInDirectSocketsName[] =
    "Source-Specific Multicast in Direct Sockets API";
inline constexpr char kSourceSpecificMulticastInDirectSocketsDescription[] =
    "Enables Source-Specific Multicast support in Direct Sockets API. See "
    "https://github.com/WICG/direct-sockets/blob/main/docs/"
    "multicast-explainer.md for "
    "details.";

inline constexpr char kCanvas2DLayersName[] =
    "Enables canvas 2D methods BeginLayer and EndLayer";
inline constexpr char kCanvas2DLayersDescription[] =
    "Enables the canvas 2D methods BeginLayer and EndLayer.";

inline constexpr char kWebMachineLearningNeuralNetworkName[] =
    "Enables WebNN API";
inline constexpr char kWebMachineLearningNeuralNetworkDescription[] =
    "Enables the Web Machine Learning Neural Network (WebNN) API. Spec at "
    "https://www.w3.org/TR/webnn/";

inline constexpr char kExperimentalWebMachineLearningNeuralNetworkName[] =
    "Enables experimental WebNN API features";
inline constexpr char
    kExperimentalWebMachineLearningNeuralNetworkDescription[] =
        "Enables additional, experimental features in Web Machine Learning "
        "Neural "
        "Network (WebNN) API. Requires the \"WebNN API\" flag to be enabled.";

inline constexpr char kWebMCPName[] = "WebMCP for testing";
inline constexpr char kWebMCPDescription[] = "Enables the WebMCP API.";

inline constexpr char kWebNNCoreMLName[] = "Core ML backend for WebNN";
inline constexpr char kWebNNCoreMLDescription[] =
    "Enables using Core ML for GPU and "
    "NPU inference with the WebNN API. Disabling this flag enables a "
    "fallback to TFLite.";

inline constexpr char kWebNNCoreMLExplicitGPUOrNPUName[] =
    "Instruct Core ML to use GPU or Neural Engine explicitly";
inline constexpr char kWebNNCoreMLExplicitGPUOrNPUDescription[] =
    "Maps the WebNN \"gpu\" and \"npu\" device types to "
    "MLComputeUnitsCPUAndGPU and MLComputeUnitsCPUAndNeuralEngine instead of "
    "MLComputeUnitsAll. Disabled by default due to crashes.";

inline constexpr char kWebNNOnnxRuntimeName[] =
    "ONNX Runtime backend for WebNN";
inline constexpr char kWebNNOnnxRuntimeDescription[] =
    "Enables using ONNX Runtime for CPU, GPU and NPU inference with the WebNN "
    "API. Disabling this flag enables a fallback to TFLite.";

inline constexpr char kOmitCorsClientCertName[] =
    "Omit TLS client certificates if credential mode disallows";
inline constexpr char kOmitCorsClientCertDescription[] =
    "Strictly conform the Fetch spec to omit TLS client certificates if "
    "credential mode disallows. Without this flag enabled, Chrome will always "
    "try sending client certificates regardless of the credential mode.";

inline constexpr char kOmniboxAllowAiModeMatchesName[] =
    "Omnibox Allow AI Mode Matches";
inline constexpr char kOmniboxAllowAiModeMatchesDescription[] =
    "Allow showing AI mode matches if returned from the search server.";

inline constexpr char kOmniboxCalcProviderName[] = "Omnibox calc provider";
inline constexpr char kOmniboxCalcProviderDescription[] =
    "When enabled, suggests recent calculator results in the omnibox.";

inline constexpr char kOmniboxCrossDeviceTabZeroSuggestName[] =
    "Omnibox Cross-Device Tab Zero Suggest";
inline constexpr char kOmniboxCrossDeviceTabZeroSuggestDescription[] =
    "Enables showing cross-device tabs in the omnibox zero-state.";

inline constexpr char kOmniboxDuiPrerenderingName[] =
    "Omnibox Direct URL Input Prerendering";
inline constexpr char kOmniboxDuiPrerenderingDescription[] =
    "Enables Direct URL Input (DUI) prerendering in the Omnibox.";

inline constexpr char kOmniboxForceAllowedToBeDefaultName[] =
    "Omnibox Force Allowed To Be Default";
inline constexpr char kOmniboxForceAllowedToBeDefaultDescription[] =
    "If enabled, all omnibox suggestions pretend to be inlineable. This likely "
    "has a bunch of problems.";

inline constexpr char kOmniboxGroupingFrameworkNonZPSName[] =
    "Omnibox Grouping Framework for Typed Suggestions";
inline constexpr char kOmniboxGroupingFrameworkDescription[] =
    "Enables an alternative grouping implementation for omnibox "
    "autocompletion.";

inline constexpr char kOmniboxToolbeltName[] = "Omnibox toolbelt";
inline constexpr char kOmniboxToolbeltDescription[] =
    "Adds a row of buttons at the bottom of the omnibox.";

inline constexpr char kOmniboxZeroSuggestPrefetchDebouncingName[] =
    "Omnibox Zero Prefix Suggest Prefetch Request Debouncing";
inline constexpr char kOmniboxZeroSuggestPrefetchDebouncingDescription[] =
    "Enables the use of a request debouncer to throttle the volume of ZPS "
    "prefetch requests issued to the remote Suggest service.";

inline constexpr char kOmniboxZeroSuggestPrefetchingOnSRPName[] =
    "Omnibox Zero Prefix Suggestion Prefetching on SRP";
inline constexpr char kOmniboxZeroSuggestPrefetchingOnSRPDescription[] =
    "Enables prefetching of the zero prefix suggestions for eligible users "
    "on the Search Results page.";

inline constexpr char kOmniboxZeroSuggestPrefetchingOnWebName[] =
    "Omnibox Zero Prefix Suggestion Prefetching on the Web";
inline constexpr char kOmniboxZeroSuggestPrefetchingOnWebDescription[] =
    "Enables prefetching of the zero prefix suggestions for eligible users "
    "on the Web (i.e. non-NTP and non-SRP URLs).";

inline constexpr char kOmniboxOnDeviceHeadSuggestionsName[] =
    "Omnibox on device head suggestions (non-incognito only)";
inline constexpr char kOmniboxOnDeviceHeadSuggestionsDescription[] =
    "Google head non personalized search suggestions provided by a compact on "
    "device model for non-incognito. Turn off this feature if you have other "
    "apps running which affects local file access (e.g. anti-virus software) "
    "and are experiencing searchbox typing lag.";
inline constexpr char kOmniboxOnDeviceHeadSuggestionsIncognitoName[] =
    "Omnibox on device head suggestions (incognito only)";
inline constexpr char kOmniboxOnDeviceHeadSuggestionsIncognitoDescription[] =
    "Google head non personalized search suggestions provided by a compact on "
    "device model for incognito. Turn off this feature if you have other "
    "apps running which affects local file access (e.g. anti-virus software) "
    "and are experiencing searchbox typing lag.";
inline constexpr char kOmniboxOnDeviceTailSuggestionsName[] =
    "Omnibox on device tail suggestions";
inline constexpr char kOmniboxOnDeviceTailSuggestionsDescription[] =
    "Google tail non personalized search suggestions provided by a compact on "
    "device model.";

inline constexpr char kOmniboxRichAutocompletionPromisingName[] =
    "Omnibox Rich Autocompletion Promising Combinations";
inline constexpr char kOmniboxRichAutocompletionPromisingDescription[] =
    "Allow autocompletion for titles and non-prefixes. Suggestions whose "
    "titles or URLs contain the user input as a continuous chunk, but not "
    "necessarily a prefix, can be the default suggestion. Otherwise, only "
    "suggestions whose URLs are prefixed by the user input can be.";

inline constexpr char kOmniboxLocalHistoryZeroSuggestBeyondNTPName[] =
    "Allow local history zero-prefix suggestions beyond NTP";
inline constexpr char kOmniboxLocalHistoryZeroSuggestBeyondNTPDescription[] =
    "Enables local history zero-prefix suggestions in every context in which "
    "the remote zero-prefix suggestions are enabled.";

inline constexpr char kOmniboxMlLogUrlScoringSignalsName[] =
    "Log Omnibox URL Scoring Signals";
inline constexpr char kOmniboxMlLogUrlScoringSignalsDescription[] =
    "Enables Omnibox to log scoring signals of URL suggestions.";

inline constexpr char kOmniboxMlUrlPiecewiseMappedSearchBlendingName[] =
    "Omnibox ML Scoring with Piecewise Score Mapping";
inline constexpr char kOmniboxMlUrlPiecewiseMappedSearchBlendingDescription[] =
    "Specifies how to blend URL ML scores and search traditional scores using "
    "a piecewise ML score mapping function.";

inline constexpr char kOmniboxMlUrlScoreCachingName[] =
    "Omnibox ML URL Score Caching";
inline constexpr char kOmniboxMlUrlScoreCachingDescription[] =
    "Enables in-memory caching of ML URL scores.";

inline constexpr char kOmniboxMlUrlScoringName[] = "Omnibox ML URL Scoring";
inline constexpr char kOmniboxMlUrlScoringDescription[] =
    "Enables ML-based relevance scoring for Omnibox URL Suggestions.";

inline constexpr char kOmniboxMlUrlScoringModelName[] =
    "Omnibox URL Scoring Model";
inline constexpr char kOmniboxMlUrlScoringModelDescription[] =
    "Enables ML scoring model for Omnibox URL suggestions.";

inline constexpr char kOmniboxMlUrlSearchBlendingName[] =
    "Omnibox ML URL Search Blending";
inline constexpr char kOmniboxMlUrlSearchBlendingDescription[] =
    "Specifies how to blend URL ML scores and search traditional scores.";

inline constexpr char kOmniboxUIMaxAutocompleteMatchesName[] =
    "Omnibox UI Max Autocomplete Matches";
inline constexpr char kOmniboxUIMaxAutocompleteMatchesDescription[] =
    "Changes the maximum number of autocomplete matches displayed in the "
    "Omnibox UI.";

inline constexpr char kOmniboxStarterPackIPHName[] =
    "IPH message for the Site search starter pack";
inline constexpr char kOmniboxStarterPackIPHDescription[] =
    "Enables an informational IPH message for the  Site search starter pack "
    "feature";

inline constexpr char kOmniboxSearchAggregatorName[] =
    "Omnibox search aggregator";
inline constexpr char kOmniboxSearchAggregatorDescription[] =
    "Enables omnibox suggestions from the search aggregator provider";

inline constexpr char kContextualSearchBoxUsesContextualSearchProviderName[] =
    "Contextual search box uses contextual search provider";
inline constexpr char
    kContextualSearchBoxUsesContextualSearchProviderDescription[] =
        "Enables the contextual search box to use the ContextualSearchProvider "
        "instead of the ZeroSuggestProvider as the source for suggestions.";

inline constexpr char kControlledFrameWebRequestSecurityInfoName[] =
    "Enable SecurityInfo in WebRequest API for ControlledFrame";
inline constexpr char kControlledFrameWebRequestSecurityInfoDescription[] =
    "Enables SecurityInfo in WebRequest API for ControlledFrames, allowing "
    "listeners to retrieve certificate details of web requests.";

inline constexpr char kContextualSearchOpenLensActionUsesThumbnailName[] =
    "Contextual search open Lens action uses thumbnail";
inline constexpr char
    kContextualSearchOpenLensActionUsesThumbnailDescription[] =
        "Enables web content thumbnail image to override the Lens icon "
        "for the omnibox entry point action match.";

inline constexpr char kContextualSuggestionsAblateOthersWhenPresentName[] =
    "Contextual suggestions ablate others when present";
inline constexpr char
    kContextualSuggestionsAblateOthersWhenPresentDescription[] =
        "Makes contextual search suggestions exclusive in zero suggest.";

inline constexpr char kContextualSuggestionsUiImprovementsName[] =
    "Contextual suggestions UI improvements";
inline constexpr char kContextualSuggestionsUiImprovementsDescription[] =
    "Enables UI improvements for contextual suggestions (e.g. icon and "
    "animation).";

inline constexpr char kOmniboxContextualSearchOnFocusSuggestionsName[] =
    "Omnibox contextual search on focus suggestions";
inline constexpr char kOmniboxContextualSearchOnFocusSuggestionsDescription[] =
    "Enables omnibox contextual search suggestions in zero prefix suggest.";

inline constexpr char kOmniboxContextualSuggestionsName[] =
    "Omnibox contextual suggestions";
inline constexpr char kOmniboxContextualSuggestionsDescription[] =
    "Enables omnibox contextual suggestions.";

inline constexpr char kEmbeddedPermissionEnabledName[] =
    "Searchbox embedded permission dialogue";
inline constexpr char kEmbeddedPermissionEnabledDescription[] =
    "Enables embedded permission dialogues for New Tab Page, omnibox popup, "
    "and 'chrome://contextual-tasks' (pre-rearchitecture) contextual tasks' "
    "permission requests.";

inline constexpr char kVoiceSearchCoherenceComposeboxName[] =
    "Composebox Voice Search Coherence";
inline constexpr char kVoiceSearchCoherenceComposeboxDescription[] =
    "Enables the voice search coherence (new recording animation, "
    "new metrics, new stop/submit button, no live transcription) "
    "for all the composeboxes by default. Options include cobrowsing "
    "only.";

inline constexpr char kVoiceSearchCoherenceSearchboxName[] =
    "NTP Searchbox (Realbox) Voice Search Coherence";
inline constexpr char kVoiceSearchCoherenceSearchboxDescription[] =
    "Enables the voice search coherence experiment (new recording animation, "
    "new metrics, added stop/submit button) for the New Tab Page "
    "searchbox, with option for live transcription.";

inline constexpr char kOmniboxFocusTriggersWebAndSRPZeroSuggestName[] =
    "Omnibox on-focus suggestions on web and SRP";
inline constexpr char kOmniboxFocusTriggersWebAndSRPZeroSuggestDescription[] =
    "Enables zero-prefix suggestions on web and SRP when the omnibox is "
    "focused, subject to the same conditions and restrictions as on-clobber "
    "suggestions.";

inline constexpr char kOmniboxHideSuggestionGroupHeadersName[] =
    "Hide suggestion group headers in the Omnibox popup";
inline constexpr char kOmniboxHideSuggestionGroupHeadersDescription[] =
    "If enabled, suggestion group headers will be hidden in the Omnibox popup "
    "(e.g. to minimize visual clutter in the zero-prefix state)";

inline constexpr char kOmniboxHideContextualGroupHeadersName[] =
    "Hide contextual suggestion group headers in the Omnibox popup";
inline constexpr char kOmniboxHideContextualGroupHeadersDescription[] =
    "If enabled, contextual suggestion group headers will be hidden in the "
    "Omnibox popup "
    "(e.g. to minimize visual clutter in the zero-prefix state)";

inline constexpr char kOmniboxUrlSuggestionsOnFocus[] =
    "Omnibox on-focus URL suggestions on web and SRP";
inline constexpr char kOmniboxUrlSuggestionsOnFocusDescription[] =
    "Enables zero-prefix URL suggestions on web and SRP when the omnibox is "
    "focused.";

inline constexpr char kOmniboxShowPopupOnMouseReleasedName[] =
    "Show omnibox suggestions popup on mouse released";
inline constexpr char kOmniboxShowPopupOnMouseReleasedDescription[] =
    "Enables delaying presentation of the omnibox suggestions popup until the "
    "mouse is released.";

inline constexpr char kOmniboxZpsSuggestionLimit[] =
    "Omnibox suggestion limit for zero prefix suggestions";
inline constexpr char kOmniboxZpsSuggestionLimitDescription[] =
    "Enables limits on the total number of suggestions, as well as separate "
    "limits for search and URL suggestions in the omnibox.";

inline constexpr char kWebUIOmniboxAimPopupName[] = "WebUI Omnibox AIM Popup";
inline constexpr char kWebUIOmniboxAimPopupDescription[] =
    "If enabled, using certain trigger operations the omnibox suggestions "
    "popup transition to showing the AI-Mode "
    "compose-box input and suggestions.";

inline constexpr char kWebUIOmniboxAimPopupDisableAnimationName[] =
    "WebUI Omnibox AIM Popup Disable Animation";
inline constexpr char kWebUIOmniboxAimPopupDisableAnimationDescription[] =
    "If enabled, there will no longer be animation in the WebUI Omnibox AIM "
    "popup.";

inline constexpr char kWebUIOmniboxFullPopupName[] = "WebUI Omnibox Full Popup";
inline constexpr char kWebUIOmniboxFullPopupDescription[] =
    "If enabled, then both the input row and suggestions dropdown (in the "
    "Omnibox) will be rendered using the WebUI stack (i.e. the cutout for the "
    "location bar will be removed).";

inline constexpr char kWebUIOmniboxPopupName[] = "WebUI Omnibox Popup";
inline constexpr char kWebUIOmniboxPopupDescription[] =
    "If enabled, shows the omnibox suggestions in the popup in WebUI.";

inline constexpr char kWebUIOmniboxPopupDebugName[] =
    "WebUI Omnibox Popup Debug Mode";
inline constexpr char kWebUIOmniboxPopupDebugDescription[] =
    "Enables the WebUI for omnibox suggestions without modifying the popup UI.";

inline constexpr char kWebUIOmniboxPopupSelectionControlName[] =
    "WebUI Omnibox Popup Selection Control";
inline constexpr char kWebUIOmniboxPopupSelectionControlDescription[] =
    "If enabled, the WebUI omnibox popup will control its own selection state.";

inline constexpr char kWebUIOmniboxSimplificationName[] =
    "WebUI Omnibox Simplification";
inline constexpr char kWebUIOmniboxSimplificationDescription[] =
    "If enabled, the WebUI omnibox popup will have a different UX for the "
    "entrypoint to composebox.";

inline constexpr char kWebUiOmniboxAskGAboutThisPageName[] =
    "WebUI Omnibox Ask G About This Page";
inline constexpr char kWebUiOmniboxAskGAboutThisPageDescription[] =
    "If enabled, the 'Ask Google about this page' action will route to "
    "cobrowse.";

inline constexpr char kWebuiRefresh2026Name[] = "WebUI Refresh 2026";
inline constexpr char kWebuiRefresh2026Description[] =
    "Enables the WebUI Refresh 2026 feature.";

inline constexpr char kOmniboxDynamicMaxAutocompleteName[] =
    "Omnibox Dynamic Max Autocomplete";
inline constexpr char kOmniboxDynamicMaxAutocompleteDescription[] =
    "Configures the maximum number of autocomplete matches displayed in the "
    "Omnibox UI dynamically based on the number of URL matches.";

inline constexpr char kOnDeviceCategoryClassifierName[] =
    "On-device category classifier";
inline constexpr char kOnDeviceCategoryClassifierDescription[] =
    "Enables the on-device category classifier.";

inline constexpr char kOptimizationGuideDebugLogsName[] =
    "Enable optimization guide debug logs";
inline constexpr char kOptimizationGuideDebugLogsDescription[] =
    "Enables the optimization guide to log and save debug messages that can be "
    "shown in the internals page.";

inline constexpr char kOptimizationGuideEnableDogfoodLoggingName[] =
    "Enable optimization guide dogfood logging";
inline constexpr char kOptimizationGuideEnableDogfoodLoggingDescription[] =
    "If this client is a Google-internal dogfood client, overrides enterprise "
    "policy to enable model quality logs. Googlers: See "
    "go/chrome-mqls-debug-logging for details.";

inline constexpr char kOrganicRepeatableQueriesName[] =
    "Organic repeatable queries in Most Visited tiles";
inline constexpr char kOrganicRepeatableQueriesDescription[] =
    "Enables showing the most repeated queries, from the device browsing "
    "history, organically among the most visited sites in the MV tiles.";

inline constexpr char kOriginAgentClusterDefaultName[] =
    "Origin-keyed Agent Clusters by default";
inline constexpr char kOriginAgentClusterDefaultDescription[] =
    "Select the default behaviour for the Origin-Agent-Cluster http header. "
    "If enabled, an absent header will cause pages to be assigned to an "
    "origin-keyed agent cluster, and to a site-keyed agent cluster when "
    "disabled. Documents whose agent clusters are origin-keyed cannot set "
    "document.domain to relax the same-origin policy.";

inline constexpr char kOriginKeyedProcessesByDefaultName[] =
    "Origin-keyed Processes by default";
inline constexpr char kOriginKeyedProcessesByDefaultDescription[] =
    "Enables origin-keyed process isolation for most pages (i.e., those "
    "assigned to an origin-keyed agent cluster by default). This improves "
    "security but also increases the number of processes created. Note: "
    "enabling this feature also enables 'Origin-keyed Agent Clusters by "
    "default'.";

inline constexpr char kOverlayScrollbarsName[] = "Overlay Scrollbars";
inline constexpr char kOverlayScrollbarsDescription[] =
    "Override scrollbar mode to overlay or classic.";

inline constexpr char kOverlayScrollbarsFlashWhenMouseEnterName[] =
    "Flash Overlay Scrollbars When Mouse Enter";
inline constexpr char kOverlayScrollbarsFlashWhenMouseEnterDescription[] =
    "Flash Overlay Scrollbars When Mouse Enter a scrollable area. You must also"
    " enable Overlay Scrollbars.";

inline constexpr char kOverlayStrategiesName[] = "Select HW overlay strategies";
inline constexpr char kOverlayStrategiesDescription[] =
    "Select strategies used to promote quads to HW overlays. Note that "
    "strategies other than Default may break playback of protected content.";
inline constexpr char kOverlayStrategiesDefault[] = "Default";
inline constexpr char kOverlayStrategiesNone[] = "None";
inline constexpr char kOverlayStrategiesUnoccludedFullscreen[] =
    "Unoccluded fullscreen buffers (single-fullscreen)";
inline constexpr char kOverlayStrategiesUnoccluded[] =
    "Unoccluded buffers (single-fullscreen,single-on-top)";
inline constexpr char kOverlayStrategiesOccludedAndUnoccluded[] =
    "Occluded and unoccluded buffers "
    "(single-fullscreen,single-on-top,underlay)";

inline constexpr char kOverscrollEffectOnNonRootScrollersName[] =
    "Overscroll effect on non-root scrollers";
inline constexpr char kOverscrollEffectOnNonRootScrollersDescription[] =
    "Enables elastic overscroll effect on scrollers other than the root "
    "document (e.g. iframes and overflow areas).";

inline constexpr char kOverscrollHistoryNavigationName[] =
    "Overscroll history navigation";
inline constexpr char kOverscrollHistoryNavigationDescription[] =
    "History navigation in response to horizontal overscroll.";

inline constexpr char kPageActionsElevatedToolbarName[] =
    "Page Actions Elevated Toolbar";
inline constexpr char kPageActionsElevatedToolbarDescription[] =
    "Enables the elevated capsule container for page action icons in the "
    "location bar.";
inline constexpr char kParallelDownloadingName[] = "Parallel downloading";
inline constexpr char kParallelDownloadingDescription[] =
    "Enable parallel downloading to accelerate download speed.";

inline constexpr char kPartitionAllocSchedulerLoopQuarantineName[] =
    "PartitionAlloc Scheduler Loop Quarantine";
inline constexpr char kPartitionAllocSchedulerLoopQuarantineDescription[] =
    "Enables PartitionAlloc's scheduler loop quarantine. Takes a JSON "
    "free-form configuration.";

inline constexpr char kPartitionAllocWithAdvancedChecksName[] =
    "PartitionAlloc with Advanced Checks";
inline constexpr char kPartitionAllocWithAdvancedChecksDescription[] =
    "Enables an extra security layer on PartitionAlloc.";

inline constexpr char kPasskeyUnlockErrorUiName[] = "Passkey Unlock Error UI";
inline constexpr char kPasskeyUnlockErrorUiDescription[] =
    "Enables showing the passkey unlock error UI to passkey users in case when "
    "their access to passkeys is “locked” and when they have an available user "
    "verification mechanism (either a system UV or a GPM PIN). This flag "
    "requires the flag `PasskeyUnlockManager` to be active.";


inline constexpr char kPasswordCompromiseWarningInDetailsCardName[] =
    "Password compromise warning in details card";
inline constexpr char kPasswordCompromiseWarningInDetailsCardDescription[] =
    "Displays a warning banner in the password details card when a "
    "saved credential is compromised.";

inline constexpr char kPasswordManagerShowSuggestionsOnAutofocusName[] =
    "Showing password suggestions on autofocused password forms";
inline constexpr char kPasswordManagerShowSuggestionsOnAutofocusDescription[] =
    "Enables showing password suggestions without requiring the user to "
    "click on the already focused field if the field was autofocused on "
    "the page load.";

inline constexpr char kPasswordSaveInContextErrorResolutionName[] =
    "In-flow resolution of errors during password saving";
inline constexpr char kPasswordSaveInContextErrorResolutionDescription[] =
    "Starts the password manager error resolution flow after offering to "
    "save a password.";

inline constexpr char kPasswordSaveUpdateDropdownMenuExperimentName[] =
    "Password Save/Update Dropdown Menu Experiment";
inline constexpr char kPasswordSaveUpdateDropdownMenuExperimentDescription[] =
    "Enables an experimental dropdown menu for password save and update "
    "prompts.";

inline constexpr char kPasswordSignInInContextErrorResolutionName[] =
    "Password sign-in in-context Trusted Vault error resolution";
inline constexpr char kPasswordSignInInContextErrorResolutionDescription[] =
    "Enables resolution of Trusted Vault encryption errors directly within the "
    "sign-in autofill dropdown.";

inline constexpr char kPaymentRequestRejectTooSmallWindowsName[] =
    "Reject Payment Request in small windows";
inline constexpr char kPaymentRequestRejectTooSmallWindowsDescription[] =
    "Reject Payment Request and Payment Handler dialogs if the browser window "
    "is too small to contain them.";

inline constexpr char kPaymentHandlerCameraAccessUxName[] =
    "Payment Handler Camera Access UX";
inline constexpr char kPaymentHandlerCameraAccessUxDescription[] =
    "Enables camera access with integrated permission prompt and "
    "capture indicator in web-based Payment Handlers on desktop.";

inline constexpr char kPaymentHandlerCustomSizeName[] =
    "Payment Handler Custom Size";
inline constexpr char kPaymentHandlerCustomSizeDescription[] =
    "Lets a payment app customize the size of its Payment Handler window "
    "statically through the preferredSize option of "
    "PaymentRequestEvent.openWindow() and/or dynamically at runtime through "
    "window.resizeTo().";

inline constexpr char kSPCLocaleValidationName[] =
    "Secure Payment Confirmation Locale Validation";
inline constexpr char kSPCLocaleValidationDescription[] =
    "When enabled, Payment Request methods throw a NotSupportedError "
    "DOMException when the SPC request contains at least one locale and none "
    "of the supplied locales match against the locale used by the SPC dialog.";

inline constexpr char kPdfXfaFormsName[] = "PDF XFA support";
inline constexpr char kPdfXfaFormsDescription[] =
    "Enables support for XFA forms in PDFs. "
    "Has no effect if Chrome was not built with XFA support, or if controlled "
    "by an enterprise policy.";

inline constexpr char kAutoWebContentsDarkModeName[] =
    "Auto Dark Mode for Web Contents";
inline constexpr char kAutoWebContentsDarkModeDescription[] =
    "Automatically render all web contents using a dark theme.";

inline constexpr char kForcedColorsName[] = "Forced Colors";
inline constexpr char kForcedColorsDescription[] =
    "Enables forced colors mode for web content.";

inline constexpr char kPermissionsAIv4Name[] = "PermissionsAIv4";
inline constexpr char kPermissionsAIv4Description[] =
    "Use the Permission Predictions Service and the AIv4 model to surface "
    "permission notification requests using a quieter UI when the likelihood "
    "of the user granting the permission is predicted to be low. Requires "
    "`Make Searches and Browsing Better` to be enabled.";

inline constexpr char kPersistentWidgetAPIName[] = "Persistent Widget API";
inline constexpr char kPersistentWidgetAPIDescription[] =
    "Enables the <persistentwidget> element, which is like an <iframe> but "
    "can stay alive without reloading when its opener/embedder navigates "
    "from one same-origin document to another.";

inline constexpr char kSearchSettingsUpdateName[] = "Search Settings Update";
inline constexpr char kSearchSettingsUpdateDescription[] =
    "Enables refactoring of the search settings pages in "
    "chrome://settings/search";

inline constexpr char kSuspiciousSiteWarningsName[] =
    "Suspicious Site Warnings";
inline constexpr char kSuspiciousSiteWarningsDescription[] =
    "Enables warnings on sites that Safe Browsing deem to be suspicious.";

inline constexpr char kSearchEngineExplicitChoiceDialogName[] =
    "Use explicit-choice dialog confirming new search engine overrides";
inline constexpr char kSearchEngineExplicitChoiceDialogDescription[] =
    "When enabled, web searches with a newly-installed search engine-"
    "overriding extension will be deferred until a new explicit-choice dialog "
    "confirms the change is intentional.";

inline constexpr char kSearchSettingsWithMoreEnginesName[] =
    "Search Settings With More Prepopulated Engines";
inline constexpr char kSearchSettingsWithMoreEnginesDescription[] =
    "Populates the Search settings page with all prepopulated engines.";

inline constexpr char kSearchEngineUnconditionalDialogName[] =
    "Always show confirmation dialog for new search engine overrides";
inline constexpr char kSearchEngineUnconditionalDialogDescription[] =
    "When enabled, all newly-installed search engine-overriding extensions "
    "encounter the confirmation dialog (exceptions for some extensions are "
    "removed).";

inline constexpr char kGlobalPrivacyControlName[] =
    "Force Global Privacy Control signal";
inline constexpr char kGlobalPrivacyControlDescription[] =
    "Enables Global Privacy Control (GPC) signal. Sends Sec-GPC=1 header with "
    "requests to indicate privacy preferences to websites.";

inline constexpr char kGlobalPrivacyControlApiName[] =
    "Global Privacy Control API";
inline constexpr char kGlobalPrivacyControlApiDescription[] =
    "Enables Global Privacy Control (GPC) signal. Sends Sec-GPC=1 header "
    "with requests and sets navigator.globalPrivacyControl JS API to indicate "
    "privacy preferences to websites.";

inline constexpr char kPrerender2EarlyDocumentLifecycleUpdateName[] =
    "Prerender more document lifecycle phases";
inline constexpr char kPrerender2EarlyDocumentLifecycleUpdateDescription[] =
    "Allows prerendering pages to execute more lifecycle updates, such as "
    "prepaint, before activation";

inline constexpr char kPreserveDownloadLastModifiedName[] =
    "Preserve Last-Modified timestamp on downloaded files";
inline constexpr char kPreserveDownloadLastModifiedDescription[] =
    "When enabled, sets the modification timestamp of downloaded files to "
    "match the HTTP Last-Modified response header provided by the server.";

inline constexpr char kTreesInVizName[] = "Trees in viz";
inline constexpr char kTreesInVizDescription[] =
    "Enables the renderer to send a CC LayerTree to the viz/gpu process "
    "instead of a CompositorFrame. This allows viz to generate and submit "
    "the CompositorFrame directly.";

inline constexpr char kEnableOmniboxSearchPrefetchName[] =
    "Omnibox prefetch Search";
inline constexpr char kEnableOmniboxSearchPrefetchDescription[] =
    "Allows omnibox to prefetch likely search suggestions provided by the "
    "Default Search Engine";

inline constexpr char kEnableOmniboxClientSearchPrefetchName[] =
    "Omnibox client prefetch Search";
inline constexpr char kEnableOmniboxClientSearchPrefetchDescription[] =
    "Allows omnibox to prefetch search suggestions provided by the Default "
    "Search Engine that the client thinks are likely to be navigated. Requires "
    "chrome://flags/#omnibox-search-prefetch";

inline constexpr char kNtpComposeboxUsesChromeComposeClientName[] =
    "Composebox uses chrome-compose client";
inline constexpr char kNtpComposeboxUsesChromeComposeClientDescription[] =
    "Composebox will use chrome-compose client when querying suggest for "
    "unimodal typed inputs instead of chrome-omni.";

inline constexpr char kOrganizerPanelName[] = "Organizer Panel";
inline constexpr char kOrganizerPanelDescription[] =
    "Enables the Organizer Panel.";

inline constexpr char kOrganizerPanelForeignTabsName[] =
    "Organizer Panel Foreign Tabs";
inline constexpr char kOrganizerPanelForeignTabsDescription[] =
    "Enables cross-device tabs in the Organizer Panel.";

inline constexpr char kPwaUpdateDialogForAppIconName[] =
    "Enable PWA install update dialog for icon changes";
inline constexpr char kPwaUpdateDialogForAppIconDescription[] =
    "Enable a confirmation dialog that shows up when a PWA changes its icon";

inline constexpr char kRemoteActorCredentialSharingName[] =
    "Enable password sharing with remote actors";
inline constexpr char kRemoteActorCredentialSharingDescription[] =
    "Enables remote actors to request passwords from the signed in Google "
    "account. When requested, the user will be prompted to select a password "
    "and authenticate.";

inline constexpr char kRenderDocumentName[] = "Enable RenderDocument";
inline constexpr char kRenderDocumentDescription[] =
    "Enable swapping RenderFrameHosts on same-site navigations";

inline constexpr char kRendererSideContentDecodingName[] =
    "Renderer-side content decoding";
inline constexpr char kRendererSideContentDecodingDescription[] =
    "Enables renderer-side content decoding (decompression). When enabled, the "
    "network service sends compressed HTTP response bodies to the renderer "
    "process.";

inline constexpr char kRustyIcoName[] = "Rust-based ICO image handling";
inline constexpr char kRustyIcoDescription[] =
    "When enabled, uses Rust `image` crate to decode ICO images.";

inline constexpr char kRustyJpegName[] = "Rust-based JPEG image handling";
inline constexpr char kRustyJpegDescription[] =
    "When enabled, uses Rust crates to decode and encode JPEG images.";

inline constexpr char kQuicName[] = "Experimental QUIC protocol";
inline constexpr char kQuicDescription[] =
    "Enable experimental QUIC protocol support.";

inline constexpr char kSendTabToSelfEnhancedHandoffName[] =
    "Send Tab To Self enhanced handoff";
inline constexpr char kSendTabToSelfEnhancedHandoffDescription[] =
    "Enables an enhanced version of Send Tab To Self that propagates more "
    "information, such as form fields and scroll position.";

inline constexpr char kSendTabToSelfNoTargetDeviceQrCodeName[] =
    "Send Tab to Self No Target Device QR Code";
inline constexpr char kSendTabToSelfNoTargetDeviceQrCodeDescription[] =
    "Shows a QR code to download and sign in to Chrome on mobile when no "
    "target devices are found in Send Tab to Self.";

inline constexpr char kRecordWebAppDebugInfoName[] =
    "Record web app debug info";
inline constexpr char kRecordWebAppDebugInfoDescription[] =
    "Enables recording additional web app related debugging data to be "
    "displayed in: chrome://web-app-internals";

inline constexpr char kUnoPhase2FollowUpName[] = "Uno phase 2 follow-up";
inline constexpr char kUnoPhase2FollowUpDescription[] =
    "UI tweaks for signin surfaces";

inline constexpr char kRootScrollbarFollowsTheme[] =
    "Make scrollbar follow theme";
inline constexpr char kRootScrollbarFollowsThemeDescription[] =
    "If enabled makes the root scrollbar follow the browser's theme color.";

inline constexpr char kMBIModeName[] = "MBI Scheduling Mode";
inline constexpr char kMBIModeDescription[] =
    "Enables independent agent cluster scheduling, via the "
    "AgentSchedulingGroup infrastructure.";

inline constexpr char kSafetyCheckUnusedSitePermissionsName[] =
    "Permission Module for unused sites in Safety Check";
inline constexpr char kSafetyCheckUnusedSitePermissionsDescription[] =
    "When enabled, adds the unused sites permission module to Safety Check on "
    "desktop. The module will be shown depending on the browser state.";

inline constexpr char kSafetyHubUnusedPermissionRevocationForAllSurfacesName[] =
    "Safety Hub -  unused permission revocation from all surfaces";
inline constexpr char
    kSafetyHubUnusedPermissionRevocationForAllSurfacesDescription[] =
        "Enables autorevoking of unused permissions granted from all UI "
        "surfaces.";

inline constexpr char kTestRootStoreName[] =
    "Enable Test-only MTC Root Store (UNSAFE)";
inline constexpr char kTestRootStoreDescription[] =
    "Enable MTC certificate verification based on test-only roots. This is "
    "unsafe and may permit an attacker to intercept or modify your HTTPS "
    "connections. Do not use this flag on an instance containing personal "
    "data. Recommended for developer use only in isolated testing environments";

inline constexpr char kTestThirdPartyCookiePhaseoutName[] =
    "Test Third Party Cookie Phaseout";
inline constexpr char kTestThirdPartyCookiePhaseoutDescription[] =
    "Enable to test third-party cookie phaseout. "
    "Learn more: https://goo.gle/3pcd-flags";

inline constexpr char kTabGroupColorRefreshName[] = "Tab Group Color Refresh";
inline constexpr char kTabGroupColorRefreshDescription[] =
    "Update the color selection for tab groups.";

inline constexpr char kTabGroupsFocusingName[] = "Tab Groups Focusing";
inline constexpr char kTabGroupsFocusingDescription[] =
    "When a tab group is focused, the tabstrip constrains visiblity to the "
    "tabs in that group.";

inline constexpr char kTabGroupRibbonName[] = "Tab Group Ribbon";
inline constexpr char kTabGroupRibbonDescription[] =
    "Displays a vertical ribbon on the left side of the browser window for "
    "quickly switching between tab groups.";

inline constexpr char kFocusSelectionName[] = "Focus Selection";
inline constexpr char kFocusSelectionDescription[] =
    "Allows focusing a selection of ungrouped tabs in the tabstrip.";

inline constexpr char kSaasUsageReportingName[] = "Saas Usage Reporting";
inline constexpr char kSaasUsageReportingDescription[] =
    "Enable reporting of SaaS usage. This feature only takes effect if a "
    "related enterprise policy is also configured by an administrator.";

inline constexpr char kSegmentationSurveyPageName[] =
    "Segmentation survey internals page and model";
inline constexpr char kSegmentationSurveyPageDescription[] =
    "Enable internals page for survey and fetching model";

inline constexpr char kDeferOsClipboardReadCallToGetTypeName[] =
    "Defer OS Clipboard Read Call To GetType";
inline constexpr char kDeferOsClipboardReadCallToGetTypeDescription[] =
    "Defers Async Clipboard Read API data retrieval from the OS until "
    "ClipboardItem.getType() is called, instead of eagerly fetching all "
    "formats at read() time.";

inline constexpr char kServiceWorkerSyntheticResponseName[] =
    "ServiceWorkerSyntheticResponse";
inline constexpr char kServiceWorkerSyntheticResponseDescription[] =
    "Enable service worker synthetic response feature.";

inline constexpr char kSharingDesktopScreenshotsName[] = "Desktop Screenshots";
inline constexpr char kSharingDesktopScreenshotsDescription[] =
    "Enables taking"
    " screenshots from the desktop sharing hub.";

inline constexpr char kShowAutofillSignaturesName[] =
    "Show autofill signatures.";
inline constexpr char kShowAutofillSignaturesDescription[] =
    "Annotates web forms with Autofill signatures as HTML attributes. Also "
    "marks password fields suitable for password generation.";

inline constexpr char kShowAutofillTypePredictionsName[] =
    "Show Autofill predictions";
inline constexpr char kShowAutofillTypePredictionsDescription[] =
    "Annotates web forms with Autofill field type predictions as placeholder "
    "text.";

inline constexpr char kShowOverdrawFeedbackName[] = "Show overdraw feedback";
inline constexpr char kShowOverdrawFeedbackDescription[] =
    "Visualize overdraw by color-coding elements based on if they have other "
    "elements drawn underneath.";

inline constexpr char kAccessibilityOnScreenModeName[] =
    "On-Screen Only Accessibility Nodes";
inline constexpr char kAccessibilityOnScreenModeDescription[] =
    "Enable experimental accessibility mode to improve performance which "
    "allows assistive technologies to access only accessibility nodes that are "
    "on-screen";

inline constexpr char kCanvasAccessibilityName[] = "Canvas Accessibility";
inline constexpr char kCanvasAccessibilityDescription[] =
    "Enables canvas accessibility and allows selecting the algorithm to add "
    "text for inaccessible canvas content.";

inline constexpr char kCollectAccessibilityHeuristicInCanvasUkmName[] =
    "Collect accessibility heuristic result in Canvas UKM";
inline constexpr char kCollectAccessibilityHeuristicInCanvasUkmDescription[] =
    "Allows collecting canvas accessibility heuristic results in the "
    "Accessibility.Canvas UKM event.";

inline constexpr char kDefaultSiteInstanceGroupsName[] =
    "Default SiteInstanceGroups";
inline constexpr char kDefaultSiteInstanceGroupsDescription[] =
    "Put sites that don't need isolation in their own SiteInstance in a default"
    "SiteInstanceGroup (per BrowsingContextGroup) instead of in a default "
    "SiteInstance.";

inline constexpr char kPwaNavigationCapturingName[] =
    "Desktop PWA Link Capturing";
inline constexpr char kPwaNavigationCapturingDescription[] =
    "Enables opening links from Chrome in an installed PWA. Currently under "
    "reimplementation.";

inline constexpr char kPwaNavigationCapturingNameTesting[] =
    "Desktop PWA Link Capturing Default On For Testing";
inline constexpr char kPwaNavigationCapturingDescriptionTesting[] =
    "Enables opening links from Chrome in an installed PWA by default for "
    "testing.";

inline constexpr char kIsolateOriginsName[] = "Isolate additional origins";
inline constexpr char kIsolateOriginsDescription[] =
    "Requires dedicated processes for an additional set of origins, "
    "specified as a comma-separated list.";

inline constexpr char kSiteIsolationOptOutName[] = "Disable site isolation";
inline constexpr char kSiteIsolationOptOutDescription[] =
    "Disables site isolation "
    "(SitePerProcess, IsolateOrigins, etc). Intended for diagnosing bugs that "
    "may be due to out-of-process iframes. Opt-out has no effect if site "
    "isolation is force-enabled using a command line switch or using an "
    "enterprise policy. "
    "Caution: this disables important mitigations for the Spectre CPU "
    "vulnerability affecting most computers.";
inline constexpr char kSiteIsolationOptOutChoiceDefault[] = "Default";
inline constexpr char kSiteIsolationOptOutChoiceOptOut[] =
    "Disabled (not recommended)";

inline constexpr char kSkiaGraphiteName[] = "Skia Graphite";
inline constexpr char kSkiaGraphiteDescription[] =
    "Enable Skia Graphite. This will use the Dawn backend by default, but can "
    "be overridden with command line flags for testing on non-official "
    "developer builds. See --skia-graphite-dawn-backend flag in "
    "gpu_switches.h.";

inline constexpr char kSkiaGraphitePrecompilationName[] =
    "Skia Graphite Precompilation";
inline constexpr char kSkiaGraphitePrecompilationDescription[] =
    "Enable Skia Graphite Precompilation. This is only relevant when Graphite "
    "is enabled "
    "but can then be overridden via the "
    "--enable-skia-graphite-precompilation and "
    "--disable-skia-graphite-precompilation "
    "command line flags";

inline constexpr char kProfileCreationDeclineSigninCTAExperimentName[] =
    "Enable CTA experiment for sign-in level up";
inline constexpr char kProfileCreationDeclineSigninCTAExperimentDescription[] =
    "As part of the Sign In Level Up experiment, changes the decline "
    "sign in CTA string in profile creation entry points";

inline constexpr char kSmoothScrollingName[] = "Smooth Scrolling";
inline constexpr char kSmoothScrollingDescription[] =
    "Animate smoothly when scrolling page content.";

inline constexpr char kStrictOriginIsolationName[] = "Strict-Origin-Isolation";
inline constexpr char kStrictOriginIsolationDescription[] =
    "Experimental security mode that strengthens the site isolation policy. "
    "Controls whether site isolation should use origins instead of scheme and "
    "eTLD+1.";

inline constexpr char kSyncAIThreadsName[] = "Sync AI Threads";
inline constexpr char kSyncAIThreadsDescription[] =
    "Acquire AI Threads via sync.";

inline constexpr char kSyncAccountSettingsName[] = "Sync account settings";
inline constexpr char kSyncAccountSettingsDescription[] =
    "Enables syncing account settings to the server.";

inline constexpr char kSyncAutofillValuableMetadataName[] =
    "Sync autofill valuable metadata";
inline constexpr char kSyncAutofillValuableMetadataDescription[] =
    "Enables syncing valuable metadata for autofill to the server.";

inline constexpr char kSyncWalletFlightReservationsName[] =
    "Sync wallet flight reservations";
inline constexpr char kSyncWalletFlightReservationsDescription[] =
    "Enables syncing flight reservations in the wallet to the server.";

inline constexpr char kSyncWalletVehicleRegistrationsName[] =
    "Sync wallet vehicle registrations";
inline constexpr char kSyncWalletVehicleRegistrationsDescription[] =
    "Enables syncing vehicle registrations in the wallet to the server.";

inline constexpr char kSyncEncryptedTabContextName[] =
    "Sync Encrypted Tab Context";
inline constexpr char kSyncEncryptedTabContextDescription[] =
    "Enables syncing of Encrypted Tab Context across devices.";

inline constexpr char kSyncJourneyName[] = "Sync journeys";
inline constexpr char kSyncJourneyDescription[] =
    "Enables syncing history journeys.";

inline constexpr char kSyncSandboxName[] = "Use Chrome Sync sandbox";
inline constexpr char kSyncSandboxDescription[] =
    "Connects to the testing server for Chrome Sync.";

inline constexpr char kSystemKeyboardLockName[] =
    "Experimental system keyboard lock";
inline constexpr char kSystemKeyboardLockDescription[] =
    "Enables websites to use the keyboard.lock() API to intercept system "
    "keyboard shortcuts and have the events routed directly to the website "
    "when in fullscreen mode.";

inline constexpr char kDataSharingDebugLogsName[] =
    "Enable data sharing debug logs";
inline constexpr char kDataSharingDebugLogsDescription[] =
    "Enables the data sharing infrastructure to log and save debug messages "
    "that can be shown in the internals page.";

inline constexpr char kThreeButtonPasswordSaveDialogName[] =
    "Three Button Password Save Dialog";
inline constexpr char kThreeButtonPasswordSaveDialogDescription[] =
    "Provides a 'not now' button alongside the 'never' button on the save "
    "password dialog.";

inline constexpr char kThrottleMainTo60HzName[] =
    "throttle-main-thread-to-60hz";
inline constexpr char kThrottleMainTo60HzDescription[] =
    "Throttle main thread updates to 60fps, even when VSync rate is higher.";

inline constexpr char kThrottleRepeatedNoDamageFramesName[] =
    "Throttle repeated no-damage frames";
inline constexpr char kThrottleRepeatedNoDamageFramesDescription[] =
    "Throttle main thread updates after we get many frames in a row with no "
    "updates.";

inline constexpr char kUnthrottleAsyncTouchMovesName[] =
    "Unthrottle async touch moves";
inline constexpr char kUnthrottleAsyncTouchMovesDescription[] =
    "When enabled, touch move events to javascript handlers are unthrottled if "
    "they are sent as async to Renderer.";

inline constexpr char kTintCompositedContentName[] = "Tint composited content";
inline constexpr char kTintCompositedContentDescription[] =
    "Tint contents composited using Viz with a shade of red to help debug and "
    "study overlay support.";

inline constexpr char kTLSTrustAnchorIDsName[] = "TLS Trust Anchor IDs";
inline constexpr char kTLSTrustAnchorIDsDescription[] =
    "This option configures TLS Trust Anchor IDs, allowing compatible servers "
    "to select between available certificates issued by different CAs.";

inline constexpr char kToolbarGlowUpName[] = "Toolbar Glow Up";
inline constexpr char kToolbarGlowUpDescription[] =
    "Enables the Toolbar Glow Up feature.";

inline constexpr char kRoundedIconsName[] = "Rounded Icons";
inline constexpr char kRoundedIconsDescription[] = "Enables rounded icons.";

inline constexpr char kWebUIRoundedIconsName[] = "WebUI Rounded Icons";
inline constexpr char kWebUIRoundedIconsDescription[] =
    "Enables rounded icons in WebUI pages.";

inline constexpr char kTopChromeTouchUiName[] = "Touch UI Layout";
inline constexpr char kTopChromeTouchUiDescription[] =
    "Enables touch UI layout in the browser's top chrome.";

inline constexpr char kTouchDragDropName[] = "Touch initiated drag and drop";
inline constexpr char kTouchDragDropDescription[] =
    "Touch drag and drop can be initiated through long press on a draggable "
    "element.";

inline constexpr char kTouchTextEditingRedesignName[] =
    "Touch Text Editing Redesign";
inline constexpr char kTouchTextEditingRedesignDescription[] =
    "Enables new touch text editing features.";

inline constexpr char kTranslatePdfName[] = "Translate PDF documents";
inline constexpr char kTranslatePdfDescription[] =
    "Enables PDF translation in Chrome.";

inline constexpr char kTreatInsecureOriginAsSecureName[] =
    "Insecure origins treated as secure";
inline constexpr char kTreatInsecureOriginAsSecureDescription[] =
    "Treat given (insecure) origins as secure origins. Multiple origins can be "
    "supplied as a comma-separated list. Origins must have their protocol "
    "specified e.g. \"http://example.com\". For the definition of secure "
    "contexts, see https://w3c.github.io/webappsec-secure-contexts/";

inline constexpr char kUnsafeWebGPUName[] = "Unsafe WebGPU Support";
inline constexpr char kUnsafeWebGPUDescription[] =
    "Convenience flag for WebGPU development. Enables best-effort WebGPU "
    "support on unsupported configurations and more! Note that this flag could "
    "expose security issues to websites so only use it for your own "
    "development.";

inline constexpr char kForceHighPerformanceGPUName[] =
    "Force High Performance GPU";
inline constexpr char kForceHighPerformanceGPUDescription[] =
    "Forces use of high performance GPU if available. Warning: this flag may "
    "increase power consumption leading to shorter battery time.";

inline constexpr char kUiPartialSwapName[] = "Partial swap";
inline constexpr char kUiPartialSwapDescription[] =
    "Sets partial swap behavior.";

inline constexpr char kUndoMigrationOfSyncingUserToSignedInName[] =
    "Undo the migration of syncing users to signed-in state";
inline constexpr char kUndoMigrationOfSyncingUserToSignedInDescription[] =
    "When enabled, reverts the migration of syncing users who were previously "
    "migrated to the signed-in, non-syncing state.";

inline constexpr char kNotificationOneTapUnsubscribeOnDesktopName[] =
    "Notification one-tap unsubscribe on Desktop";
inline constexpr char kNotificationOneTapUnsubscribeOnDesktopDescription[] =
    "Enables an experimental UX  on Desktop that replaces the [Site settings]"
    "button on web push notifications with an [Unsubscribe] button.";

inline constexpr char kVerifyMTCsName[] = "Verify MTCs";
inline constexpr char kVerifyMTCsDescription[] =
    "Enables verification of Merkle Tree Certificates as described in "
    "https://datatracker.ietf.org/doc/draft-davidben-tls-merkle-tree-certs/. "
    "Requires #tls-trust-anchor-ids.";

inline constexpr char kVerifyQWACsName[] = "Verify QWACs";
inline constexpr char kVerifyQWACsDescription[] =
    "Enables verification of qualified certificates for website authentication "
    "as described in ETSI TS 119 411-5 V2.1.1 (2025-02).";

inline constexpr char kV8VmFutureName[] = "Future V8 VM features";
inline constexpr char kV8VmFutureDescription[] =
    "This enables upcoming and experimental V8 VM features. "
    "This flag does not enable experimental JavaScript features.";

inline constexpr char kGlobalVaapiLockName[] =
    "Global lock on the VA-API wrapper.";
inline constexpr char kGlobalVaapiLockDescription[] =
    "Enable or disable the global VA-API lock for platforms and paths that "
    "support controlling this.";

inline constexpr char kWalletApiPrivatePassesEnabledName[] =
    "Wallet API Private Passes";
inline constexpr char kWalletApiPrivatePassesEnabledDescription[] =
    "Enables the Wallet API for private passes.";

inline constexpr char kWalletServiceUseSandboxName[] =
    "Use Google Payments sandbox servers";
inline constexpr char kWalletServiceUseSandboxDescription[] =
    "For developers: use the sandbox service for Google Payments API calls.";

inline constexpr char kWallpaperSearchSettingsVisibilityName[] =
    "Wallpaper Search Settings Visibility";
inline constexpr char kWallpaperSearchSettingsVisibilityDescription[] =
    "Shows wallpaper search settings in settings UI.";

inline constexpr char kWebAppInstallElementName[] = "Web App Install Element";
inline constexpr char kWebAppInstallElementDescription[] =
    "Enables the <install> HTML element which allows web apps to be "
    "installed declaratively.";

inline constexpr char kWebAppInstallationApiName[] = "Web App Installation API";
inline constexpr char kWebAppInstallationApiDescription[] =
    "Enables the Web App Installation API which allows web apps to be "
    "installed programmatically using navigator.install().";

inline constexpr char kWebAppInstallDialogName[] = "Web App install dialog";
inline constexpr char kWebAppInstallDialogDescription[] =
    "Enables the new web app install dialog.";

inline constexpr char kWebAppInstallDialogWinPinName[] =
    "Enable PWA install dialog pinning option";
inline constexpr char kWebAppInstallDialogWinPinDescription[] =
    "Enables the option to pin PWA to taskbar in the PWA install "
    "dialog.";

inline constexpr char kWebAppMigratePreinstalledChatName[] =
    "Migrate preinstalled Chat app";
inline constexpr char kWebAppMigratePreinstalledChatDescription[] =
    "When enabled, the preinstalled Chat web app will be migrated from its old "
    "URL to a new URL. This migration only applies to users who have not "
    "manually installed the Chat app.";

inline constexpr char kWebAuthnAmbientSigninName[] =
    "Enable Ambient sign-in for WebAuthn get requests";
inline constexpr char kWebAuthnAmbientSigninDescription[] =
    "Enables ambient sign-in for passkeys and passwords from a "
    "navigator.credentials.get() request. This is similar to conditional UI "
    "with the main difference being it does not use autofill UI.";

inline constexpr char kWebAuthnCrossDeviceFallbackUrlName[] =
    "WebAuthn Cross-device fallback URL extension";
inline constexpr char kWebAuthnCrossDeviceFallbackUrlDescription[] =
    "Enables support for the WebAuthn crossDeviceFallbackUrl extension, "
    "which allows Relying Parties to provide a fallback URL for hybrid "
    "authenticator get requests.";

inline constexpr char kWebBluetoothName[] = "Web Bluetooth";
inline constexpr char kWebBluetoothDescription[] =
    "Enables the Web Bluetooth API on platforms without official support";

inline constexpr char kWebBluetoothNewPermissionsBackendName[] =
    "Use the new permissions backend for Web Bluetooth";
inline constexpr char kWebBluetoothNewPermissionsBackendDescription[] =
    "Enables the new permissions backend for Web Bluetooth. This will enable "
    "persistent storage of device permissions and Web Bluetooth features such "
    "as BluetoothDevice.watchAdvertisements() and Bluetooth.getDevices()";

inline constexpr char kWebCryptoPQCName[] = "WebCrypto PQC";
inline constexpr char kWebCryptoPQCDescription[] =
    "Enables Post-Quantum Cryptography (PQC) algorithms (ML-DSA and ML-KEM) "
    "and ChaCha20-Poly1305 in WebCrypto.";

inline constexpr char kWebiumName[] = "Webium";
inline constexpr char kWebiumDescription[] = "Webium Prototype Browser.";

inline constexpr char kWebUIToolbarName[] = "WebUI Toolbar";
inline constexpr char kWebUIToolbarDescription[] =
    "Use WebUI to render the toolbar.";

inline constexpr char kWebglDeveloperExtensionsName[] =
    "WebGL Developer Extensions";
inline constexpr char kWebglDeveloperExtensionsDescription[] =
    "Enabling this option allows web applications to access WebGL extensions "
    "intended only for use during development time.";

inline constexpr char kWebglDraftExtensionsName[] = "WebGL Draft Extensions";
inline constexpr char kWebglDraftExtensionsDescription[] =
    "Enabling this option allows web applications to access the WebGL "
    "extensions that are still in draft status.";

inline constexpr char kWebGpuDeveloperFeaturesName[] =
    "WebGPU Developer Features";
inline constexpr char kWebGpuDeveloperFeaturesDescription[] =
    "Enables web applications to access WebGPU features intended only for use "
    "during development.";

inline constexpr char kWebPaymentsExperimentalFeaturesName[] =
    "Experimental Web Payments API features";
inline constexpr char kWebPaymentsExperimentalFeaturesDescription[] =
    "Enable experimental Web Payments API features";

inline constexpr char kAppStoreBillingDebugName[] =
    "Web Payments App Store Billing Debug Mode";
inline constexpr char kAppStoreBillingDebugDescription[] =
    "App-store purchases (e.g., Google Play Store) within a TWA can be "
    "requested using the Payment Request API. This flag removes the "
    "restriction that the TWA has to be installed from the app-store.";

inline constexpr char kSecurePaymentConfirmationCredentialDiscoveryModeName[] =
    "Secure Payment Confirmation Credential Discovery Mode";
inline constexpr char
    kSecurePaymentConfirmationCredentialDiscoveryModeDescription[] =
        "Controls the strategy for discovering Secure Payment Confirmation "
        "credentials (e.g. database-only, hybrid, or OS store only).";

inline constexpr char kWebrtcHideLocalIpsWithMdnsName[] =
    "Anonymize local IPs exposed by WebRTC.";
inline constexpr char kWebrtcHideLocalIpsWithMdnsDecription[] =
    "Conceal local IP addresses with mDNS hostnames.";

inline constexpr char kWebRtcAllowInputVolumeAdjustmentName[] =
    "Allow WebRTC to adjust the input volume.";
inline constexpr char kWebRtcAllowInputVolumeAdjustmentDescription[] =
    "Allow the Audio Processing Module in WebRTC to adjust the input volume "
    "during a real-time call. Disable if microphone muting or clipping issues "
    "are observed when the browser is running and used for a real-time call. "
    "This flag is experimental and may be removed at any time.";

inline constexpr char kWebRtcApmDownmixCaptureAudioMethodName[] =
    "WebRTC downmix capture audio method.";
inline constexpr char kWebRtcApmDownmixCaptureAudioMethodDescription[] =
    "Override the method that the Audio Processing Module in WebRTC uses to "
    "downmix the captured audio to mono (when needed) during a real-time call. "
    "This flag is experimental and may be removed at any time.";

inline constexpr char kWebRtcPqcForDtlsName[] = "WebRTC PQC for DTLS";
inline constexpr char kWebRtcPqcForDtlsDescription[] =
    "Support in WebRTC to enable PQC for DTLS";

inline constexpr char kWebrtcUseMinMaxVEADimensionsName[] =
    "WebRTC Min/Max Video Encode Accelerator dimensions";
inline constexpr char kWebrtcUseMinMaxVEADimensionsDescription[] =
    "When enabled, WebRTC will only use the Video Encode Accelerator for "
    "video resolutions inside those published as supported.";

inline constexpr char kWebTransportDeveloperModeName[] =
    "WebTransport Developer Mode";
inline constexpr char kWebTransportDeveloperModeDescription[] =
    "When enabled, removes the requirement that all certificates used for "
    "WebTransport over HTTP/3 are issued by a known certificate root.";

inline constexpr char kWebUsbDeviceDetectionName[] =
    "Automatic detection of WebUSB-compatible devices";
inline constexpr char kWebUsbDeviceDetectionDescription[] =
    "When enabled, the user will be notified when a device which advertises "
    "support for WebUSB is connected. Disable if problems with USB devices are "
    "observed when the browser is running.";

inline constexpr char kWebXrForceRuntimeName[] = "Force WebXr Runtime";
inline constexpr char kWebXrForceRuntimeDescription[] =
    "Force the browser to use a particular runtime, even if it would not "
    "usually be enabled or would otherwise not be selected based on the "
    "attached hardware.";

inline constexpr char kWebXrRuntimeChoiceNone[] = "No Runtime";
inline constexpr char kWebXrRuntimeChoiceOpenXR[] = "OpenXR";
inline constexpr char kWebXrRuntimeChoiceOrientationSensors[] =
    "Orientation Sensors";

inline constexpr char kWebXrHandAnonymizationStrategyName[] =
    "WebXr Hand Anonymization Strategy";
inline constexpr char kWebXrHandAnonymizationStrategyDescription[] =
    "Force the browser to use a particular strategy for anonymizing hand data, "
    "the default order has a hierarchy of strategies to try and if all of them "
    "fail, then no data will be returned, while this choice does allow the "
    "(not recommended) alternative of bypassing these algorithms all together.";

inline constexpr char kWebXrHandAnonymizationChoiceNone[] =
    "None (Not Recommended)";
inline constexpr char kWebXrHandAnonymizationChoiceRuntime[] =
    "Runtime Provided";
inline constexpr char kWebXrHandAnonymizationChoiceFallback[] =
    "Chrome Fallback";

inline constexpr char kWebXrIncubationsName[] = "WebXR Incubations";
inline constexpr char kWebXrIncubationsDescription[] =
    "Enables experimental features for WebXR.";

inline constexpr char kWindowOpenAlwaysOnTopName[] = "Always-on-top popups";
inline constexpr char kWindowOpenAlwaysOnTopDescription[] =
    "Allows websites with the window-management permission to open "
    "always-on-top popup windows via window.open().";

inline constexpr char kZeroCopyName[] = "Zero-copy rasterizer";
inline constexpr char kZeroCopyDescription[] =
    "Raster threads write directly to GPU memory associated with tiles.";

inline constexpr char kEnableVulkanName[] = "Vulkan";
inline constexpr char kEnableVulkanDescription[] =
    "Use vulkan as the graphics backend.";

inline constexpr char kWebGpuInteropName[] = "Force enable WebGPU interop";
inline constexpr char kWebGpuInteropDescription[] =
    "Force enable the WebGPU on vulkan via GL compositing interop.";

inline constexpr char kDefaultAngleVulkanName[] = "Default ANGLE Vulkan";
inline constexpr char kDefaultAngleVulkanDescription[] =
    "Use the Vulkan backend for ANGLE by default.";

inline constexpr char kVulkanFromAngleName[] = "Vulkan from ANGLE";
inline constexpr char kVulkanFromAngleDescription[] =
    "Initialize Vulkan from inside ANGLE and share the instance with Chrome.";

inline constexpr char kUsePassthroughCommandDecoderName[] =
    "Use passthrough command decoder";
inline constexpr char kUsePassthroughCommandDecoderDescription[] =
    "Use chrome passthrough command decoder instead of validating command "
    "decoder.";

inline constexpr char kUserValueDefaultBrowserStringsName[] =
    "Default Browser settings page - updated strings";
inline constexpr char kUserValueDefaultBrowserStringsDescription[] =
    "Improves the flow and the wording on the Default Browser settings page.";

inline constexpr char kPredictableReportedQuotaName[] =
    "Predictable Reported Quota";
inline constexpr char kPredictableReportedQuotaDescription[] =
    "Enables reporting of a predictable quota from the StorageManager's "
    "estimate API. This flag is intended only for validating if this change "
    "caused an unforeseen bug.";

inline constexpr char kRunVideoCaptureServiceInBrowserProcessName[] =
    "Run video capture service in browser";
inline constexpr char kRunVideoCaptureServiceInBrowserProcessDescription[] =
    "Run the video capture service in the browser process.";

inline constexpr char kPromptAPIName[] = "Prompt API";
inline constexpr char kPromptAPIDescription[] =
    "Enables the Prompt API, allowing you to send natural language "
    "instructions to a built-in large language model. This API is primarily "
    "intended for natural language processing tasks such as summarizing, "
    "classifying, or rephrasing text. It is NOT suitable for use cases that "
    "require factual accuracy (e.g. answering knowledge questions)."
    "Please refer to the built-in AI documentation [1] for details.";
inline constexpr const char* kAIAPIsLinks[1] = {
    "https://developer.chrome.com/docs/ai/built-in"};

inline constexpr char kDecisionsAPIName[] = "Decisions API";
inline constexpr char kDecisionsAPIDescription[] =
    "Enables the Built-in AI Decisions API (DecisionModel) for on-device "
    "verification, classification, and decisions regarding unstructured data, "
    "over a developer-defined schema. See the explainer [1] for details. "
    "All Backends uses the EmbeddingGemma V1 backend by default if "
    "devTrialBackend is not specified. Enabling Gemma 4 on CPU (or All "
    "Backends) forces CPU for all Gemma 4 usage across built-in AI APIs to "
    "work around GPU scoring issues.";
inline constexpr const char* kDecisionsAPILinks[1] = {
    "https://github.com/explainers-by-googlers/decisions-api"};

inline constexpr char kGemma4ForBuiltInAIName[] = "Gemma 4 for Built-in AI";
inline constexpr char kGemma4ForBuiltInAIDescription[] =
    "Enables all built-in AI APIs (Prompt, Summarizer, Writer, etc.) to use "
    "the Gemma 4 model.";

inline constexpr char kOnDeviceModelSpeculativeDecodingName[] =
    "Multi-Token Prediction (MTP) for Built-in AI";
inline constexpr char kOnDeviceModelSpeculativeDecodingDescription[] =
    "Enables Multi-Token Prediction (MTP) speculative decoding for Built-in "
    "AI APIs. Also enables requisite Gemma 4 and Sampling Mode features. "
    "Summarizer API sessions automatically use greedy sampling with MTP by "
    "default. Prompt API sessions use MTP when greedy sampling options are "
    "specified (such as samplingMode: \"most-predictable\"); default and "
    "non-greedy sessions run with standard decoding.";

inline constexpr char kPromptAPIMultimodalInputName[] =
    "Prompt API Multimodal Input";
inline constexpr char kPromptAPIMultimodalInputDescription[] =
    "Extends the Prompt API with image and audio input types. Allows you to "
    "supplement natural language instructions for a built-in large language "
    "model with image and audio inputs. This API enhancement is primarily "
    "intended for natural language processing tasks associated with visual and "
    "auditory data, such as generating rough descriptions of pictures and "
    "sounds. It is NOT suitable for use cases that require factual accuracy "
    "(e.g. answering knowledge questions). "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kPromptAPIToolUseName[] = "Prompt API Tool Use";
inline constexpr char kPromptAPIToolUseDescription[] =
    "Extends the Prompt API with tool use (function calling) capabilities.";

inline constexpr char kPromptAPISamplingModeName[] = "Prompt API Sampling Mode";
inline constexpr char kPromptAPISamplingModeDescription[] =
    "Enables setting the sampling mode option during session creation. "
    "When enabled, the model's creativity and predictability can be "
    "configured using high-level sampling modes.";

inline constexpr char kWriterAPIName[] = "Writer API";
inline constexpr char kWriterAPIDescription[] =
    "Enables the Writer API, allowing you to write a piece of text with a "
    "built-in large language model. "
    "The API may be subject to changes including the supported options. "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kRewriterAPIName[] = "Rewriter API";
inline constexpr char kRewriterAPIDescription[] =
    "Enables the Rewriter API, allowing you to rewrite a piece of text with a "
    "built-in large language model. "
    "The API may be subject to changes including the supported options. "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kProofreaderAPIName[] = "Proofreader API";
inline constexpr char kProofreaderAPIDescription[] =
    "Enables the Proofreader API, allowing you to proofread a piece of text "
    "with a built-in small expert model. "
    "The API may be subject to changes including the supported options. "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kSemanticEmbedderAPIName[] = "Semantic Embedder API";
inline constexpr char kSemanticEmbedderAPIDescription[] =
    "Enables the Semantic Embedder API, allowing you to generate semantic "
    "embeddings with a built-in model in Chrome. "
    "The API may be subject to changes including the supported options. "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kSummarizerAPIWithPerformancePreferenceName[] =
    "Summarizer API Performance Preference";
inline constexpr char kSummarizerAPIWithPerformancePreferenceDescription[] =
    "Adds the \"preference\" create option to the Summarizer API. Allows the "
    "developer to choose between \"capability\", \"speed\", and \"auto\" to "
    "hint at the desired balance between performance and capability. "
    "We're rapidly iterating on the training set for the smaller expert "
    "model used with the 'speed' preference, and welcome your feedback";
inline constexpr const char* kSummarizerAPIWithPerformancePreferenceLinks[2] = {
    "https://chromestatus.com/feature/6309243756085248",
    "https://issues.chromium.org/issues/new?component=1617227"};

inline constexpr char kSummarizerAPIName[] = "Summarizer API";
inline constexpr char kSummarizerAPIDescription[] =
    "Enables the Summarizer API, allowing you to summarize a piece of text "
    "with a built-in large language model. "
    "Please refer to the built-in AI documentation [1] for details.";

inline constexpr char kOnDeviceWebSpeechSmallExpertModelName[] =
    "Web Speech API On-Device Small Expert Model";
inline constexpr char kOnDeviceWebSpeechSmallExpertModelDescription[] =
    "Enables the small expert model for on-device Web Speech API "
    "recognition.";

// Android ---------------------------------------------------------------------
// FLAG_DESCRIPTIONS_ANDROID_START

inline constexpr char kForceWebGpuAdapterName[] = "Use WebGPU adapter";
inline constexpr char kForceWebGpuAdapterDescription[] =
    "Force WebGPU to use a specific graphics adapter backend.";
inline constexpr char kForceWebGpuAdapterOpenGLES[] = "OpenGL ES";

inline constexpr char kTextDetectionAPIName[] = "Text Detection API";
inline constexpr char kTextDetectionAPIDescription[] =
    "Enables the Web Text Detection API (Shape Detection API for text).";

inline constexpr char kUniversalOptOutName[] = "Universal Opt Out";
inline constexpr char kUniversalOptOutDescription[] =
    "Enables Universal Opt-Out feature.";

inline constexpr char kUniversalOptOutSettingsName[] =
    "Universal Opt Out Settings";
inline constexpr char kUniversalOptOutSettingsDescription[] =
    "Enables the Universal Opt Out settings for eligible users.";

inline constexpr char kNotebookHomeURLName[] = "Notebook Home URL";
inline constexpr char kNotebookHomeURLDescription[] =
    "Overrides the default URL for the Notebook Home WebUI.";

inline constexpr char kNotebooksName[] = "Notebooks";
inline constexpr char kNotebooksDescription[] = "Enables Notebooks.";

inline constexpr char kSpatialEntitesDepthHitTestName[] =
    "Spatial Entities Depth-based Hit Test";
inline constexpr char kSpatialEntitesDepthHitTestDescription[] =
    "Controls whether the spatial entities framework is allowed to use "
    "depth-based hit tests or only plane-based ones.";

inline constexpr char kTrustedVaultSharedRecoveryFactorsAndEncryptionName[] =
    "Trusted Vault shared recovery factors and encryption";
inline constexpr char
    kTrustedVaultSharedRecoveryFactorsAndEncryptionDescription[] =
        "Enables sharing recovery factors across security domains and "
        "encrypting local storage using OSCrypt for Trusted Vault.";

inline constexpr char kAimUsePecApiName[] = "Use PEC API";
inline constexpr char kAimUsePecApiDescription[] =
    "When enabled, all composebox context menus will use the `SearchboxConfig` "
    "from the PEC API.";

inline constexpr char kAimEligibilityComponentExtensionName[] =
    "AIM Eligibility Component Extension";
inline constexpr char kAimEligibilityComponentExtensionDescription[] =
    "When enabled, registers the AIM Eligibility debugger as a component "
    "extension.";

// FLAG_DESCRIPTIONS_ANDROID_END
// Non-Android -----------------------------------------------------------------

inline constexpr char kAllowAllSitesToInitiateMirroringName[] =
    "Allow all sites to initiate mirroring";
inline constexpr char kAllowAllSitesToInitiateMirroringDescription[] =
    "When enabled, allows all websites to request to initiate tab mirroring "
    "via Presentation API. Requires #cast-media-route-provider to also be "
    "enabled";

inline constexpr char kAXTreeFixingName[] = "AXTree Fixing";
inline constexpr char kAXTreeFixingDescription[] =
    "When enabled, allows Chrome to dynamically fix the AXTree of sites. This "
    "is experimental and may cause breaking changes to users of assistive "
    "technology.";

inline constexpr char kBrowserInitiatedAutomaticPictureInPictureName[] =
    "Browser initiated automatic picture in picture";
inline constexpr char kBrowserInitiatedAutomaticPictureInPictureDescription[] =
    "When enabled, allows the browser to automatically enter picture in "
    "picture when a series of conditions are met.";

inline constexpr char kDeviceSignalsBackfillDisclaimerName[] =
    "Startup Enterprise Signals Disclaimer";
inline constexpr char kDeviceSignalsBackfillDisclaimerDescription[] =
    "Enables the Startup Enterprise Signals Disclaimer. The disclaimer will be "
    "shown to managed users missing the device signals permission on chrome "
    "startup.";

inline constexpr char kPictureInPictureMuteControlName[] =
    "Picture-in-Picture Mute Control";
inline constexpr char kPictureInPictureMuteControlDescription[] =
    "Adds a mute/unmute button to the Video Picture-in-Picture window.";

inline constexpr char kCastMirroringNativeNV12Name[] =
    "Cast Mirroring Native NV12";
inline constexpr char kCastMirroringNativeNV12Description[] =
    "If enabled, NV12 video frames will be sent to the encoder without being "
    "converted to I420 during Cast Mirroring.";

inline constexpr char
    kClientSideDetectionServerModelForScamDetectionDesktopName[] =
        "Client Side Detection Server Model for Scam Detection on Desktop";
inline constexpr char
    kClientSideDetectionServerModelForScamDetectionDesktopDescription[] =
        "Enables server model for scam detection on Desktop.";

inline constexpr char kComposeboxDriveContextMenuOptionName[] =
    "Composebox Drive Context Menu Option";
inline constexpr char kComposeboxDriveContextMenuOptionDescription[] =
    "Enables Google Drive option in the Composebox context menu.";
inline constexpr char kComposeboxDriveContextMenuOptionDisclaimerName[] =
    "Composebox Drive Context Menu Option Disclaimer";
inline constexpr char kComposeboxDriveContextMenuOptionDisclaimerDescription[] =
    "Enables the disclaimer flow for the Google Drive option in the Composebox "
    "context menu.";
inline constexpr char kComposeboxDriveContextMenuOptionSigninPromoName[] =
    "Composebox Drive Context Menu Option Sign-in Promo";
inline constexpr char
    kComposeboxDriveContextMenuOptionSigninPromoDescription[] =
        "Enables the sign-in promo for the Google Drive option in the "
        "Composebox context menu.";

inline constexpr char kEnableHeadlessLiveCaptionName[] =
    "Headless Live Captions";
inline constexpr char kEnableHeadlessLiveCaptionDescription[] =
    "Enable features related to headless captions exploration. These are "
    "very likely unstable.";

inline constexpr char kEnableInputProtectionName[] = "Input protection";
inline constexpr char kEnableInputProtectionDescription[] =
    "Enables input protection by blocking interaction with views that are "
    "currently or were recently obscured by always-on-top windows, and "
    "prevents unintended events on security-sensitive UI that appears and "
    "activates unexpectedly.";

inline constexpr char kFeedbackDisabledDialogName[] =
    "Feedback Disabled Dialog";
inline constexpr char kFeedbackDisabledDialogDescription[] =
    "Enables the dialog shown when user feedback is disabled.";

inline constexpr char kForceDriveDisclaimerAcceptedName[] =
    "Force Drive Disclaimer Accepted";
inline constexpr char kForceDriveDisclaimerAcceptedDescription[] =
    "Whether to force the Google Drive disclaimer to be accepted. This flag is "
    "only used for testing purposes.";

inline constexpr char kHeadlessCaptionEarlyStartName[] =
    "Headless Caption Early Start";
inline constexpr char kHeadlessCaptionEarlyStartDescription[] =
    "Start headless captioning as soon as a profile is loaded.";

inline constexpr char kEnableShouldShowPromotionName[] =
    "Enable Chrome Web Store Promotion Banner";
inline constexpr char kEnableShouldShowPromotionDescription[] =
    "Enables showing the promotion banner on chromewebstore page.";

inline constexpr char kReadAnythingOmniboxChipName[] =
    "Reading Mode Omnibox Chip";
inline constexpr char kReadAnythingOmniboxChipDescription[] =
    "Enables the omnibox chip entry point for Reading mode";

inline constexpr char kReadAnythingLineFocusName[] = "Reading Mode Line Focus";
inline constexpr char kReadAnythingLineFocusDescription[] =
    "Enables the experimental line focus mode for Reading mode.";

inline constexpr char kImmersiveReadAnythingPrintName[] =
    "Immersive Reading Mode Print";
inline constexpr char kImmersiveReadAnythingPrintDescription[] =
    "Enables printing the distilled content from immersive Reading mode.";

inline constexpr char kReadAnythingReadAloudPhraseHighlightingName[] =
    "Reading Mode Read Aloud Phrase Highlighting";
inline constexpr char kReadAnythingReadAloudPhraseHighlightingDescription[] =
    "Enables the experimental Reading Mode feature that highlights by phrases "
    "when reading aloud, when the phrase option is selected from the highlight "
    "menu.";

inline constexpr char kReadAnythingDistillerRefactorName[] =
    "Reading Mode Distiller Stability Updates";
inline constexpr char kReadAnythingDistillerRefactorDescription[] =
    "Optimizes how web page content is prepared and processed for Reading "
    "Mode.";

inline constexpr char kReadAnythingWithReadabilityName[] =
    "Reading Mode Experimental Webpage Distilation";
inline constexpr char kReadAnythingWithReadabilityDescription[] =
    "Enables the experimental text webpage distillation using readability.js "
    "method for reading mode.";
inline constexpr char kReadAnythingImprovedUiName[] =
    "Reading Mode Improved UI";
inline constexpr char kReadAnythingImprovedUiDescription[] =
    "Enables menu grouping for Reading Mode and the listen to this page entry "
    "point.";
inline constexpr char kReadAnythingReadAloudExperimentalPlaybackUiName[] =
    "Reading Mode Read Aloud Experimental Playback UI";
inline constexpr char
    kReadAnythingReadAloudExperimentalPlaybackUiDescription[] =
        "Enables the experimental playback UI for Reading Mode Read Aloud.";

inline constexpr char kReadAnythingSendFeedbackName[] =
    "Reading Mode Send Feedback";
inline constexpr char kReadAnythingSendFeedbackDescription[] =
    "Enables the send feedback entry point in the Reading Mode settings menu.";

inline constexpr char kReauthSkipIdentifierPageName[] =
    "Skip identifier page on reauth";
inline constexpr char kReauthSkipIdentifierPageDescription[] =
    "Re-authentication (mainly in the sign-in pending state) opens Gaia's "
    "AccountChooser with ptc=1 instead of AddSession, skipping the email "
    "page.";

inline constexpr char kReauthSkipIdentifierPageFollowupName[] =
    "Skip identifier page on reauth follow-up";
inline constexpr char kReauthSkipIdentifierPageFollowupDescription[] =
    "Enables follow-up changes to skipping the identifier page on reauth, "
    "including supervised user re-authentication in the pending state and "
    "refined fallback metrics.";

inline constexpr char kRealboxVirtualFocusNavigationName[] =
    "Enable Realbox Virtual Focus Navigation";
inline constexpr char kRealboxVirtualFocusNavigationDescription[] =
    "Enables a new virtual focus model for keyboard navigation within the "
    "New Tab Page Realbox. This allows tabbing through all interactive "
    "elements within suggestions (actions, buttons) without losing focus "
    "from the searchbox, unifying behavior with the Omnibox and improving "
    "accessibility.";


inline constexpr char kOmniboxEverywhereScreenshotNewWindowName[] =
    "Omnibox Everywhere Screenshot New Window";
inline constexpr char kOmniboxEverywhereScreenshotNewWindowDescription[] =
    "When enabled, submitting a query with a screenshot attached in Omnibox "
    "Everywhere fulfills the query in a new browser window aligned "
    "with the Omnibox Everywhere widget.";

inline constexpr char kOmniboxEverywhereVirtualFocusNavigationName[] =
    "Enable Omnibox Everywhere Virtual Focus Navigation";
inline constexpr char kOmniboxEverywhereVirtualFocusNavigationDescription[] =
    "Enables virtual focus keyboard navigation within Omnibox Everywhere.";

inline constexpr char kWebuiBrowserVirtualFocusNavigationName[] =
    "Enable WebUI Browser Virtual Focus Navigation";
inline constexpr char kWebuiBrowserVirtualFocusNavigationDescription[] =
    "Enables virtual focus keyboard navigation within the WebUI Browser "
    "searchbox.";

inline constexpr char kMarkAllCredentialsAsLeakedName[] =
    "Mark all credential as leaked";
inline constexpr char kMarkAllCredentialsAsLeakedDescription[] =
    "Will pop up the leaked check dialog on every password form submission. "
    "This should be used "
    "in combination with #improved-password-change-service to better test the "
    "improved password change service";

inline constexpr char kMultistepFilterName[] = "Multistep Filter";
inline constexpr char kMultistepFilterDescription[] =
    "Allows users to quickly apply search filters when "
    "browsing supported websites. Suggestions are based on recent "
    "browsing activity and filters are saved locally on the device.";

inline constexpr char kMuteNotificationSnoozeActionName[] =
    "Snooze action for mute notifications";
inline constexpr char kMuteNotificationSnoozeActionDescription[] =
    "Adds a Snooze action to mute notifications shown while sharing a screen.";

inline constexpr char kNtpAlphaBackgroundCollectionsName[] =
    "NTP Alpha Background Collections";
inline constexpr char kNtpAlphaBackgroundCollectionsDescription[] =
    "Shows alpha NTP background collections in Customize Chrome.";

inline constexpr char kNtpBackgroundImageErrorDetectionName[] =
    "NTP Background Image Error Detection";
inline constexpr char kNtpBackgroundImageErrorDetectionDescription[] =
    "Checks NTP background image links for HTTP status errors.";

inline constexpr char kNtpCalendarModuleName[] = "NTP Calendar Module";
inline constexpr char kNtpCalendarModuleDescription[] =
    "Shows the Google Calendar module on the New Tab Page.";

inline constexpr char kNtpComponentExtensionName[] = "NTP Component Extension";
inline constexpr char kNtpComponentExtensionDescription[] =
    "When enabled, registers the New Tab Page as a component extension.";

inline constexpr char kNtpComposeboxName[] = "NTP Composebox";
inline constexpr char kNtpComposeboxDescription[] =
    "Shows the Composebox on the New Tab Page Searchbox upon clicking the "
    "entrypoint.";

inline constexpr char kNtpCustomizeChromeAutoOpenName[] =
    "NTP Customize Chrome Auto Promo";
inline constexpr char kNtpCustomizeChromeAutoOpenDescription[] =
    "Shows the Customize Chrome on the New Tab Page automatically.";

inline constexpr char kNtpRealboxNextName[] = "NTP Realbox Next";
inline constexpr char kNtpRealboxNextDescription[] =
    "Enables the Realbox 'Next' experience.";

inline constexpr char kNtpRealboxCyclingPlaceholdersName[] =
    "NTP Realbox Cycling Placeholders";
inline constexpr char kNtpRealboxCyclingPlaceholdersDescription[] =
    "Enables cycling placeholders within the Realbox 'Next' experience.";

inline constexpr char kNtpDriveModuleName[] = "NTP Drive Module";
inline constexpr char kNtpDriveModuleDescription[] =
    "Shows the Google Drive module on the New Tab Page";

inline constexpr char kNtpDriveModuleSegmentationName[] =
    "NTP Drive Module Segmentation";
inline constexpr char kNtpDriveModuleSegmentationDescription[] =
    "Uses segmentation data to decide whether to show the Drive module on the "
    "New Tab Page.";

inline constexpr char kNtpDummyModulesName[] = "NTP Dummy Modules";
inline constexpr char kNtpDummyModulesDescription[] =
    "Adds dummy modules to New Tab Page when 'NTP Modules Redesigned' is "
    "enabled.";

inline constexpr char kNtpFeatureOptimizationModuleRemovalName[] =
    "NTP Feature Optimization Module Removal";
inline constexpr char kNtpFeatureOptimizationModuleRemovalDescription[] =
    "Enables auto-removal of stale modules from the NTP.";

inline constexpr char kNtpFeatureOptimizationShortcutsRemovalName[] =
    "NTP Feature Optimization Shortcuts Removal";
inline constexpr char kNtpFeatureOptimizationShortcutsRemovalDescription[] =
    "Enables auto-removal of stale shortcuts from the NTP.";

inline constexpr char kNtpAnimatedDoodlesName[] = "NTP Animated Doodles";
inline constexpr char kNtpAnimatedDoodlesDescription[] =
    "Enables animated Doodles on the NTP.";

inline constexpr char kNtpDoodleMuralsName[] = "NTP Doodle Murals";
inline constexpr char kNtpDoodleMuralsDescription[] =
    "Enables Doodle Murals on the NTP.";

inline constexpr char kNtpThreadsRailName[] = "NTP Threads Rail";
inline constexpr char kNtpThreadsRailDescription[] =
    "Enables the Threads Rail on the NTP. The Hybrid variation renders the "
    "folded state natively in Chrome while the expanded state is rendered by "
    "Google Search.";

inline constexpr char kNtpSimplificationBookmarkBarName[] =
    "NTP Simplification Bookmark Bar";
inline constexpr char kNtpSimplificationBookmarkBarDescription[] =
    "Auto-removes the bookmark bar on the New Tab Page and updates bookmark "
    "bar visbility settings.";

inline constexpr char kNtpFooterName[] = "NTP Footer";
inline constexpr char kNtpFooterDescription[] =
    "Adds footer to New Tab Page that encapsulates customize buttons and "
    "background/theme attributions.";

inline constexpr char kNtpMicrosoftAuthenticationModuleName[] =
    "NTP Microsoft Authentication Module";
inline constexpr char kNtpMicrosoftAuthenticationModuleDescription[] =
    "Shows the Microsoft Authentication Module on the New Tab Page.";

inline constexpr char kNtpModuleSignInRequirementName[] =
    "NTP Modules Sign-in Requirement";
inline constexpr char kNtpModuleSignInRequirementDescription[] =
    "Makes NTP Sign-in Requirement per module, removing the requirement for "
    "Microsoft Modules";

inline constexpr char kNtpNextFeaturesName[] = "NTP Next Features";
inline constexpr char kNtpNextFeaturesDescription[] =
    "Enables features (e.g., AI action chips) in NTP Next";

inline constexpr char kNtpNextCanvasChipName[] = "NTP Next Canvas Chip";
inline constexpr char kNtpNextCanvasChipDescription[] =
    "Enables the Canvas tool chip for NTP action chips";

inline constexpr char kNtpOneGoogleBarAsyncBarPartsName[] =
    "NTP OneGoogleBar Async Bar Parts";
inline constexpr char kNtpOneGoogleBarAsyncBarPartsDescription[] =
    "Enables the OneGoogleBar async bar parts API on the New Tab Page.";

inline constexpr char kNtpOutlookCalendarModuleName[] =
    "NTP Outlook Calendar Module";
inline constexpr char kNtpOutlookCalendarModuleDescription[] =
    "Shows the Outlook Calendar module on the New Tab Page.";

inline constexpr char kNtpRealboxCr23ThemingName[] =
    "Chrome Refresh Themed Realbox";
inline constexpr char kNtpRealboxCr23ThemingDescription[] =
    "CR23 theming will be applied in Realbox when enabled.";

inline constexpr char kNtpScaledActionChipsName[] = "NTP Scaled Action Chips";
inline constexpr char kNtpScaledActionChipsDescription[] =
    "Enables scaled merchandising action chips (e.g., dynamic tool suggestions "
    "and resource pickers) on the New Tab Page.";

inline constexpr char kNtpScaledActionChipsSmallName[] =
    "NTP Scaled Action Chips Small";
inline constexpr char kNtpScaledActionChipsSmallDescription[] =
    "Enables small scaled merchandising action chips on the New Tab Page.";

inline constexpr char kNtpSharepointModuleName[] = "NTP Sharepoint Module";
inline constexpr char kNtpSharepointModuleDescription[] =
    "Shows the Sharepoint module on the New Tab Page.";

inline constexpr char kNtpStarterChipName[] = "NTP Starter Chip";
inline constexpr char kNtpStarterChipDescription[] =
    "Enables the starter chip on the New Tab Page.";

inline constexpr char kNtpTabGroupsModuleName[] = "NTP Tab Groups Module";
inline constexpr char kNtpTabGroupsModuleDescription[] =
    "Shows the Tab Groups module on the New Tab Page.";

inline constexpr char kNtpTabGroupsModuleZeroStateName[] =
    "NTP Tab Groups Zero State Card";
inline constexpr char kNtpTabGroupsModuleZeroStateDescription[] =
    "Enables the zero-state card for the Tab Groups Module on the New Tab "
    "Page.";

inline constexpr char kNtpWallpaperSearchButtonName[] =
    "NTP Wallpaper Search Button";
inline constexpr char kNtpWallpaperSearchButtonDescription[] =
    "Enables entry point on New Tab Page for Customize Chrome Side Panel "
    "Wallpaper Search.";

inline constexpr char kNtpWallpaperSearchButtonAnimationName[] =
    "NTP Wallpaper Search Button Animation";
inline constexpr char kNtpWallpaperSearchButtonAnimationDescription[] =
    "Enables animation for New Tab Page's Wallpaper Search button. Requires "
    "#ntp-wallpaper-search-button to be enabled too.";

inline constexpr char kHappinessTrackingSurveysForDesktopDemoName[] =
    "Happiness Tracking Surveys Demo";
inline constexpr char kHappinessTrackingSurveysForDesktopDemoDescription[] =
    "Enable showing Happiness Tracking Surveys Demo to users on Desktop";

inline constexpr char kMainNodeAnnotationsName[] = "Main Node Annotations";
inline constexpr char kMainNodeAnnotationsDescription[] =
    "Uses Screen2x main content extractor to annotate the accessibility tree "
    "with the main landmark on the node identified as main.";

inline constexpr char kTranslateOpenSettingsName[] = "Translate Open Settings";
inline constexpr char kTranslateOpenSettingsDescription[] =
    "Add an option to the translate bubble menu to open language settings.";

inline constexpr char kWebAuthenticationPermitEnterpriseAttestationName[] =
    "Web Authentication Enterprise Attestation";
inline constexpr char
    kWebAuthenticationPermitEnterpriseAttestationDescription[] =
        "Permit a set of origins to request a uniquely identifying enterprise "
        "attestation statement from a security key when creating a Web "
        "Authentication credential.";

inline constexpr char kNtpShortcutsRedesignName[] = "NTP Shortcuts Redesign";
inline constexpr char kNtpShortcutsRedesignDescription[] =
    "Enables the redesigned shortcuts on the New Tab Page.";

inline constexpr char kNtpShortcutsReorderName[] = "NTP Shortcuts Reorder";
inline constexpr char kNtpShortcutsReorderDescription[] =
    "Enables shortcut reordering entrypoint on the New Tab Page.";

// Windows ---------------------------------------------------------------------

inline constexpr char kArabicIndicDigitInputName[] = "Arabic-Indic Digit Input";
inline constexpr char kArabicIndicDigitInputDescription[] =
    "Enable/Disable Arabic-Indic digit input while holding right alt or "
    "ctrl+alt on Arabic 101 keyboard layout.";

inline constexpr char kEnableMediaFoundationVideoCaptureName[] =
    "MediaFoundation Video Capture";
inline constexpr char kEnableMediaFoundationVideoCaptureDescription[] =
    "Enable/Disable the usage of MediaFoundation for video capture. Fall back "
    "to DirectShow if disabled.";

inline constexpr char kHardwareSecureDecryptionName[] =
    "Hardware Secure Decryption";
inline constexpr char kHardwareSecureDecryptionDescription[] =
    "Enable/Disable the use of hardware secure Content Decryption Module (CDM) "
    "for protected content playback.";

inline constexpr char kHidGetFeatureReportFixName[] =
    "Adjust feature reports received with WebHID";
inline constexpr char kHidGetFeatureReportFixDescription[] =
    "Enable/Disable a fix for a bug that caused feature reports to be offset "
    "by one byte when received from devices that do not use numbered reports.";

inline constexpr char kHardwareSecureDecryptionFallbackName[] =
    "Hardware Secure Decryption Fallback";
inline constexpr char kHardwareSecureDecryptionFallbackDescription[] =
    "Allows automatically disabling hardware secure Content Decryption Module "
    "(CDM) after failures or crashes. Subsequent playback may use software "
    "secure CDMs. If this feature is disabled, the fallback will never happen "
    "and users could be stuck with playback failures.";

inline constexpr char kMediaFoundationCameraUsageMonitoringName[] =
    "Media Foundation Camera Usage Monitoring";
inline constexpr char kMediaFoundationCameraUsageMonitoringDescription[] =
    "Enables the use of Media Foundation for camera usage monitoring. "
    "This allows detecting if a camera is being used by another application.";

inline constexpr char kUseAngleDescriptionWindows[] =
    "Choose the graphics backend for ANGLE. D3D11 is used on most Windows "
    "computers by default.";

inline constexpr char kUseAngleD3D11[] = "D3D11";
inline constexpr char kUseAngleD3D11Warp[] = "D3D11 WARP";

inline constexpr char kUseWaitableSwapChainName[] = "Use waitable swap chains";
inline constexpr char kUseWaitableSwapChainDescription[] =
    "Use waitable swap chains to reduce presentation latency (effective only "
    "Windows 8.1 or later). If enabled, specify the maximum number of frames "
    "that can be queued, ranging from 1-3. 1 has the lowest delay but is most "
    "likely to drop frames, while 3 has the highest delay but is least likely "
    "to drop frames.";

inline constexpr char kUseWinrtMidiApiName[] = "Use Windows Runtime MIDI API";
inline constexpr char kUseWinrtMidiApiDescription[] =
    "Use Windows Runtime MIDI API for WebMIDI (effective only on Windows 10 or "
    "later).";

inline constexpr char kWebRtcWgcRequireBorderName[] =
    "Border around WGC captures";
inline constexpr char kWebRtcWgcRequireBorderDescription[] =
    "When using WGC to capture a window or a screen, draw a border around the "
    "captured surface.";

inline constexpr char kWindowsSystemTracingName[] = "System tracing";
inline constexpr char kWindowsSystemTracingDescription[] =
    "When enabled, the system tracing service is started along with Chrome's "
    "tracing service (if the system tracing service is registered).";

inline constexpr char kPrintWithPostScriptType42FontsName[] =
    "Print with PostScript Type 42 fonts";
inline constexpr char kPrintWithPostScriptType42FontsDescription[] =
    "When using PostScript level 3 printing, render text with Type 42 fonts if "
    "possible.";

inline constexpr char kPrintWithReducedRasterizationName[] =
    "Print with reduced rasterization";
inline constexpr char kPrintWithReducedRasterizationDescription[] =
    "When using GDI printing, avoid rasterization if possible.";

// Mac -------------------------------------------------------------------------

inline constexpr char kMacAccessibilityAPIMigrationName[] =
    "Mac A11y API Migration";
inline constexpr char kMacAccessibilityAPIMigrationDescription[] =
    "Enables the migration to the new Cocoa accessibility API.";

inline constexpr char kMacCatapLoopbackAudioForCastName[] =
    "Mac Core Audio Tap System Loopback Capture for Cast";
inline constexpr char kMacCatapLoopbackAudioForCastDescription[] =
    "Enable system audio loopback capture for Cast using the macOS CoreAudio "
    "tap API on macOS 14.2+.";

inline constexpr char kMacPWAsNotificationAttributionName[] =
    "Mac PWA notification attribution";
inline constexpr char kMacPWAsNotificationAttributionDescription[] =
    "Route notifications for PWAs on Mac through the app shim, attributing "
    "notifications to the correct apps.";

inline constexpr char kMacSandboxLoggingName[] = "Mac Sandbox Logging";
inline constexpr char kMacSandboxLoggingDescription[] =
    "Enables sandbox violation logging on macOS. Useful for determining "
    "whether an isuse is the result of an unexpected sandbox denial and for "
    "debugging issues that stem from sandbox denials.";

inline constexpr char kUnexportableKeyDeletionName[] =
    "Enable Unexportable Key Deletion";
inline constexpr char kUnexportableKeyDeletionDescription[] =
    "Enables the garbage collection and deletion of obsolete cryptographic "
    "keys used for Device Bound Session Credentials.";

inline constexpr char kUseSCContentSharingPickerName[] =
    "Use ScreenCaptureKit picker for stream selection";
inline constexpr char kUseSCContentSharingPickerDescription[] =
    "This feature opens a native picker in macOS 15+ to allow the selection "
    "of a window or screen that will be captured.";

inline constexpr char kBlockRootWindowAccessibleNameChangeEventName[] =
    "Block Root Window Accessible Name Change Event";
inline constexpr char kBlockRootWindowAccessibleNameChangeEventDescription[] =
    "This feature prevents the firing of accessible name change events on the "
    "Root Window of MacOS applications. By blocking these events, it ensures "
    "that changes to the accessible name of Root Window do not trigger "
    "notifications to assistive technologies. This can be useful in scenarios "
    "where frequent or unnecessary name change events could lead to "
    "performance issues or unwanted behavior in assistive applications.";

inline constexpr char kSuppressAppKitFrameAdjustmentsDuringMoveLoopName[] =
    "Suppress AppKit Frame Adjustments During Move Loop";
inline constexpr char
    kSuppressAppKitFrameAdjustmentsDuringMoveLoopDescription[] =
        "Suppresses AppKit system frame adjustment and move events on macOS "
        "while a window move loop is active (e.g. during tab dragging across "
        "display boundaries).";

// Windows and Mac -------------------------------------------------------------

inline constexpr char kLocationProviderManagerName[] =
    "Enable location provider manager for Geolocation API";
inline constexpr char kLocationProviderManagerDescription[] =
    "Enables usage of the location provider manager to select between "
    "the operating system's location API or the network-based provider "
    "as the data source for Geolocation API.";

// Windows, Mac and Android  --------------------------------------------------

inline constexpr char kUseAngleName[] = "Choose ANGLE graphics backend";

inline constexpr char kUseAngleDefault[] = "Default";

// ChromeOS -------------------------------------------------------------------

inline constexpr char kEnterpriseTabTitleReportingName[] =
    "Enterprise Tab Title Reporting";
inline constexpr char kEnterpriseTabTitleReportingDescription[] =
    "Enables the reporting of destination tab titles in Chrome Enterprise URL "
    "navigation logs.";

inline constexpr char kIdbSqliteBackingStoreName[] = "IDB SQLite Backing Store";
inline constexpr char kIdbSqliteBackingStoreDescription[] =
    "Uses a SQLite-powered backing store for IndexedDB. No data is migrated "
    "from existing backing stores, including LevelDB stores or SQLite stores "
    "with older schemas; use at your own peril.";

inline constexpr char kStructuredDnsErrorsName[] = "Structured DNS Errors";
inline constexpr char kStructuredDnsErrorsDescription[] =
    "When enabled, signals support for Structured DNS Errors when sending DNS "
    "requests, renders Extended DNS Error codes on the net error page when "
    "applicable, and interprets filtering details provided via "
    "draft-nottingham-public-resolver-errors-02";

inline constexpr char kUserMediaElementName[] = "UserMediaElement";
inline constexpr char kUserMediaElementDescription[] =
    "Enables the <usermedia> element, allowing sites to embed a "
    "camera/microphone permission request directly in the page.";

inline constexpr char kUserMediaElementLegacyName[] = "UserMediaElementLegacy";
inline constexpr char kUserMediaElementLegacyDescription[] =
    "Enables legacy compatibility paths for <usermedia> element.";

// Prefer keeping this section sorted to adding new definitions down here.

// Linux -----------------------------------------------------------------------

inline constexpr char kMiddleClickAutoscrollName[] = "Middle-click autoscroll";
inline constexpr char kMiddleClickAutoscrollDescription[] =
    "Enables autoscrolling when pressing the middle mouse button on a "
    "scrollable page.";

inline constexpr char kPulseaudioLoopbackForCastName[] =
    "Linux System Audio Loopback for Cast (pulseaudio)";
inline constexpr char kPulseaudioLoopbackForCastDescription[] =
    "Enable system audio mirroring when casting a screen on Linux with "
    "pulseaudio.";

inline constexpr char kPulseaudioLoopbackForScreenShareName[] =
    "Linux System Audio Loopback for Screen Sharing (pulseaudio)";
inline constexpr char kPulseaudioLoopbackForScreenShareDescription[] =
    "Enable system audio sharing when screen sharing on Linux with pulseaudio.";

inline constexpr char kWaylandSessionManagementName[] =
    "Wayland session management";
inline constexpr char kWaylandSessionManagementDescription[] =
    "Enable Wayland's xx/xdg-session-management-v1 experimental support.";

// Random platform combinations -----------------------------------------------

inline constexpr char kZeroCopyVideoCaptureName[] =
    "Enable Zero-Copy Video Capture";
inline constexpr char kZeroCopyVideoCaptureDescription[] =
    "Camera produces a gpu friendly buffer on capture and, if there is, "
    "hardware accelerated video encoder consumes the buffer";

inline constexpr char kLocalNetworkAccessChecksName[] =
    "Local Network Access Checks";
inline constexpr char kLocalNetworkAccessChecksDescription[] =
    "Enables Local Network Access checks. "
    "See: https://chromestatus.com/feature/5152728072060928";

inline constexpr char kLocalNetworkAccessChecksWebRTCName[] =
    "Local Network Access Checks for WebRTC";
inline constexpr char kLocalNetworkAccessChecksWebRTCDescription[] =
    "Enable Local Network Access checks for WebRTC. Requires the "
    "#local-network-access-check flag to also be enabled "
    "See: https://chromestatus.com/feature/5065884686876672";

inline constexpr char kLocalNetworkAccessChecksWebSocketsName[] =
    "Local Network Access Checks for WebSockets";
inline constexpr char kLocalNetworkAccessChecksWebSocketsDescription[] =
    "Enable Local Network Access checks for WebSockets. Requires the "
    "#local-network-access-check flag to also be enabled "
    "See: https://chromestatus.com/feature/5197681148428288";

inline constexpr char kLocalNetworkAccessChecksWebTransportName[] =
    "Local Network Access Checks for WebTransport";
inline constexpr char kLocalNetworkAccessChecksWebTransportDescription[] =
    "Enable Local Network Access checks for WebTransport. Requires the "
    "#local-network-access-check flag to also be enabled "
    "See: https://chromestatus.com/feature/5126430912544768";

inline constexpr char kGroupPromoPrototypeName[] = "Group Promo Prototype";
inline constexpr char kGroupPromoPrototypeDescription[] =
    "Enables prototype for group promo.";

inline constexpr char kEnableNetworkServiceSandboxName[] =
    "Enable the network service sandbox.";
inline constexpr char kEnableNetworkServiceSandboxDescription[] =
    "Enables a sandbox around the network service to help mitigate exploits in "
    "its process. This may cause crashes if Kerberos is used.";

inline constexpr char kWebBluetoothConfirmPairingSupportName[] =
    "Web Bluetooth confirm pairing support";
inline constexpr char kWebBluetoothConfirmPairingSupportDescription[] =
    "Enable confirm-only and confirm-pin pairing mode support for Web "
    "Bluetooth";

inline constexpr char kCupsIppPrintingBackendName[] =
    "CUPS IPP Printing Backend";
inline constexpr char kCupsIppPrintingBackendDescription[] =
    "Use the CUPS IPP printing backend instead of the original CUPS backend "
    "that calls the PPD API.";

inline constexpr char kChromeWideEchoCancellationName[] =
    "Chrome-wide echo cancellation";
inline constexpr char kChromeWideEchoCancellationDescription[] =
    "Run WebRTC capture audio processing in the audio process instead of the "
    "renderer processes, thereby cancelling echoes from more audio sources.";

inline constexpr char kDcheckIsFatalName[] = "DCHECKs are fatal";
inline constexpr char kDcheckIsFatalDescription[] =
    "By default Chrome will evaluate in this build, but only log failures, "
    "rather than crashing. If enabled, DCHECKs will crash the calling process.";

inline constexpr char kRouteMatchingName[] = "Route matching";
inline constexpr char kRouteMatchingDescription[] =
    "Allow definition of routes as e.g. URLPattern. Special CSS rules can be "
    "used to match active routes. See "
    "https://github.com/WICG/declarative-partial-updates/blob/main/"
    "route-matching-explainer.md";

inline constexpr char kEnableOopPrintDriversName[] =
    "Enables Out-of-Process Printer Drivers";
inline constexpr char kEnableOopPrintDriversDescription[] =
    "Enables printing interactions with the operating system to be performed "
    "out-of-process.";

inline constexpr char kPdfAccessibilityHeuristicEnhancementsName[] =
    "PDF Accessibility Heuristic Enhancements";
inline constexpr char kPdfAccessibilityHeuristicEnhancementsDescription[] =
    "Enables heuristic enhancements for PDF accessibility.";

inline constexpr char kPdfBrotliDecodeName[] = "PDF Brotli Decode";
inline constexpr char kPdfBrotliDecodeDescription[] =
    "Enables support for the experimental PDF 2.0 /BrotliDecode filter.";

inline constexpr char kPdfInk2Name[] = "PDF Ink Signatures";
inline constexpr char kPdfInk2Description[] =
    "Enables the ability to annotate PDFs using a new ink library.";

inline constexpr char kPdfSaveToDriveName[] = "Save PDF to Drive";
inline constexpr char kPdfSaveToDriveDescription[] =
    "Enables the ability to save PDFs to Google Drive.";

inline constexpr char kPdfOopifName[] = "OOPIF for PDF Viewer";
inline constexpr char kPdfOopifDescription[] =
    "Use an OOPIF for the PDF Viewer, instead of a GuestView.";

inline constexpr char kPdfPortfolioName[] = "PDF portfolio";
inline constexpr char kPdfPortfolioDescription[] =
    "Enable PDF portfolio feature.";

inline constexpr char kPdfUseSkiaRendererName[] = "Use Skia Renderer";
inline constexpr char kPdfUseSkiaRendererDescription[] =
    "Use Skia as the PDF renderer. This flag will have no effect if the "
    "renderer choice is controlled by an enterprise policy.";

inline constexpr char kWebXrLayersName[] = "WebXR Layers";
inline constexpr char kWebXrLayersDescription[] =
    "Enables use of WebXR Layers.";
inline constexpr char kWebXrWebGpuBindingName[] = "WebXR/WebGPU Binding";
inline constexpr char kWebXrWebGpuBindingDescription[] =
    "Enables rendering with WebGPU for WebXR sessions. WebXR Projection "
    "Layers must be also be enabled to use this feature.";
inline constexpr char kWebXrInternalsName[] = "WebXR Internals Debugging Page";
inline constexpr char kWebXrInternalsDescription[] =
    "Enables the webxr-internals developer page which can be used to help "
    "debug issues with the WebXR Device API.";

inline constexpr char kWebXrPlaneDetectionName[] = "WebXR Plane Detection";
inline constexpr char kWebXrPlaneDetectionDescription[] =
    "Enables access to the WebXR plane-detection feature.";

inline constexpr char kWebXrMeshDetectionName[] = "WebXR Mesh Detection";
inline constexpr char kWebXrMeshDetectionDescription[] =
    "Enables access to the WebXR mesh-detection feature.";

inline constexpr char kAutofillCreditCardUploadName[] =
    "Enable offering upload of Autofilled credit cards";
inline constexpr char kAutofillCreditCardUploadDescription[] =
    "Enables a new option to upload credit cards to Google Payments for sync "
    "to all Chrome devices.";

inline constexpr char kElasticOverscrollName[] = "Elastic Overscroll";
inline constexpr char kElasticOverscrollDescription[] =
    "Enables Elastic Overscrolling on touchscreens and precision touchpads.";

inline constexpr char kUIDebugToolsName[] = "Debugging tools for UI";
inline constexpr char kUIDebugToolsDescription[] =
    "Enables additional keyboard shortcuts to help debugging.";

inline constexpr char kWebrtcPipeWireCameraName[] = "PipeWire Camera support";
inline constexpr char kWebrtcPipeWireCameraDescription[] =
    "When enabled the PipeWire multimedia server will be used for cameras.";

inline constexpr char kComposeSelectionNudgeName[] = "Compose Selection Nudge";
inline constexpr char kComposeSelectionNudgeDescription[] =
    "Enables nudge on selection for Compose";

inline constexpr char kAiOverlayDisableNavigationContextName[] =
    "AI Overlay Disable Navigation Context";
inline constexpr char kAiOverlayDisableNavigationContextDescription[] =
    "Disables sharing navigation history.";






inline constexpr char kPageColorsSettingsName[] = "Page Colors settings";
inline constexpr char kPageColorsSettingsDescription[] =
    "When enabled, shows a Page Colors control in Chrome's accessibility "
    "settings, which lets users pick a high contrast theme for web content or "
    "turn forced colors off, independently of the operating system theme.";

inline constexpr char kProfilesReorderingName[] = "Profiles Reordering";
inline constexpr char kProfilesReorderingDescription[] =
    "Enables profiles reordering in the Profile Picker main view by drag and "
    "dropping the Profile Tiles. The order is saved when changed and "
    "persisted.";

inline constexpr char kEnableChromeRefreshTokenBindingName[] =
    "Chrome Refresh Token Binding";
inline constexpr char kEnableChromeRefreshTokenBindingDescription[] =
    "Enables binding of Chrome refresh tokens to cryptographic keys.";

inline constexpr char kEnableChromeRefreshTokenBindingUpgradeName[] =
    "Chrome Refresh Token Binding Upgrade";
inline constexpr char kEnableChromeRefreshTokenBindingUpgradeDescription[] =
    "Enables opportunistic binding of existing unbound Chrome refresh tokens "
    "and session cookies to cryptographic keys. Requires Chrome Refresh Token "
    "Binding to be enabled.";

inline constexpr char kEnableCookieBindingCookieUpgradeName[] =
    "Cookie Binding Cookie Upgrade";
inline constexpr char kEnableCookieBindingCookieUpgradeDescription[] =
    "Enables upgrading unbound session cookies to be hardware-bound on "
    "supported device platforms.";

inline constexpr char kDiceLinkedAccountsName[] = "Linked Accounts";
inline constexpr char kDiceLinkedAccountsDescription[] =
    "Enables Linked Accounts and DICE version 2 protocol header.";

inline constexpr char kEnableOAuthMultiloginStandardCookiesBindingName[] =
    "Enable OAuthMultilogin Standard Cookies Binding";
inline constexpr char
    kEnableOAuthMultiloginStandardCookiesBindingDescription[] =
        "Enables binding of cookies returned from OAuthMultilogin to "
        "cryptographic keys using standard DBSC.";

inline constexpr char kEnableOAuthMultiloginYoutubeCookiesBindingName[] =
    "Enable Youtube OAuthMultilogin Cookies Binding";
inline constexpr char kEnableOAuthMultiloginYoutubeCookiesBindingDescription[] =
    "When enabled, Chrome will send a specific URL parameter to Gaia "
    "to trigger the server-side experiment for binding the OAuthMultilogin "
    "cookies for Youtube to cryptographic keys.";

inline constexpr char kEnableBoundSessionCredentialsName[] =
    "Device Bound Session Credentials";
inline constexpr char kEnableBoundSessionCredentialsDescription[] =
    "Enables Google session credentials binding to cryptographic keys.";

inline constexpr char
    kEnableBoundSessionCredentialsSoftwareKeysForManualTestingName[] =
        "Device Bound Session Credentials with software keys";
inline constexpr char
    kEnableBoundSessionCredentialsSoftwareKeysForManualTestingDescription[] =
        "Enables mock software-backed cryptographic keys for Google session "
        "credentials binding and Chrome refresh tokens binding (not secure). "
        "This is intended to be used for manual testing only.";

inline constexpr char kEnableStandardBoundSessionCredentialsName[] =
    "Device Bound Session Credentials (Standard)";
inline constexpr char kEnableStandardBoundSessionCredentialsDescription[] =
    "Enables the official version of Device Bound Session Credentials. For "
    "more information see https://github.com/WICG/dbsc.";
inline constexpr char kEnableDeviceBoundSessionsDsePrewarmerName[] =
    "Enable Device Bound Session Credentials Prewarmer for Default Search "
    "Engine";
inline constexpr char kEnableDeviceBoundSessionsDsePrewarmerDescription[] =
    "Enables preemptive refreshes for sessions registered in the same site "
    "as the default search engine when the user is not actively navigating.";
inline constexpr char
    kEnableStandardBoundSessionCredentialsFederatedSessionsName[] =
        "Device Bound Session Credentials (Standard) - Federated Registrations";
inline constexpr char
    kEnableStandardBoundSessionCredentialsFederatedSessionsDescription[] =
        "Enables federated session registration for the official version of "
        "Device Bound Session Credentials.";
inline constexpr char kEnableStandardBoundSessionsGoogleName[] =
    "Device Bound Session Credentials (Standard) on Google";
inline constexpr char kEnableStandardBoundSessionsGoogleDescription[] =
    "Enables Device Bound Session Credentials (Standard) to register sessions "
    "on Google sites";
inline constexpr char kEnableStandardBoundSessionCredentialsAudienceName[] =
    "Device Bound Session Credentials (Standard) - Audience Claim";
inline constexpr char
    kEnableStandardBoundSessionCredentialsAudienceDescription[] =
        "Enables including the 'aud' (audience) claim in Device Bound "
        "Session Credentials registration and refresh JWT payloads.";

inline constexpr char kVariationsSeedCorpusName[] = "Variations seed corpus";
inline constexpr char kVariationsSeedCorpusDescription[] =
    "The value of the 'corpus' parameter in the variations seed request. "
    "If unspecified, the 'corpus' parameter is omitted from the request.";

inline constexpr char kHandleMdmErrorsForDasherAccountsName[] =
    "Mdm error handling for dasher accounts";
inline constexpr char kHandleMdmErrorsForDasherAccountsDescription[] =
    "Enables the mdm error handling feature for dasher accounts";

inline constexpr char kEnablePingSSOName[] =
    "Enable Ping SSO for enterprise browsers.";
inline constexpr char kEnablePingSSODescription[] =
    "Enables native support for Ping Identity SSO on managed MacOS devices.";

inline constexpr char kAntivirusTelemetryForDownloadsName[] =
    "Antivirus telemetry for downloads";
inline constexpr char kAntivirusTelemetryForDownloadsDescription[] =
    "Enables antivirus product info to be included in Safe Browsing download "
    "pings.";

// ============================================================================
// Don't just add flags to the end, put them in the alphabetical order.
// ============================================================================

}  // namespace flag_descriptions

#endif  // CHROME_BROWSER_FLAG_DESCRIPTIONS_H_
