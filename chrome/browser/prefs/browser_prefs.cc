// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/browser_prefs.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "build/android_buildflags.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/accessibility/accessibility_labels_service.h"
#include "chrome/browser/accessibility/invert_bubble_prefs.h"
#include "chrome/browser/accessibility/page_colors_controller.h"
#include "chrome/browser/accessibility/prefers_default_scrollbar_styles_prefs.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/component_updater/component_updater_prefs.h"
#include "chrome/browser/context_hub/prefs.h"
#include "chrome/browser/contextual_cueing/prefs.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/engagement/important_sites_util.h"
#include "chrome/browser/enterprise/util/managed_browser_utils.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/glic/gemini_enterprise/geic_pref_names.h"
#include "chrome/browser/glic/glic_pref_names.h"
#include "chrome/browser/glic/suggestions/contextual_cueing_prefs.h"
#include "chrome/browser/gpu/gpu_mode_manager.h"
#include "chrome/browser/hid/hid_policy_allowed_devices.h"
#include "chrome/browser/intranet_redirect_detector.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/login_detection/login_detection_prefs.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/media/media_storage_id_salt.h"
#include "chrome/browser/media/prefs/capture_device_ranking.h"
#include "chrome/browser/media/unified_autoplay_config.h"
#include "chrome/browser/media/webrtc/capture_policy_utils.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/permission_bubble_media_access_handler.h"
#include "chrome/browser/memory/enterprise_memory_limit_pref_observer.h"
#include "chrome/browser/metrics/chrome_metrics_service_client.h"
#include "chrome/browser/metrics/tab_stats/tab_stats_tracker.h"
#include "chrome/browser/net/net_error_tab_helper.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/new_tab_page/prefs/ntp_pref_names.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/notifier_state_tracker.h"
#include "chrome/browser/notifications/platform_notification_service_impl.h"
#include "chrome/browser/permissions/quiet_notification_permission_ui_state.h"
#include "chrome/browser/platform_experience/prefs.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/preloading/prefetch/prefetch_service/prefetch_origin_decider.h"
#include "chrome/browser/preloading/prefetch/search_prefetch/search_prefetch_service.h"
#include "chrome/browser/preloading/preloading_prefs.h"
#include "chrome/browser/preloading/search_preload/search_preload_service.h"
#include "chrome/browser/printing/print_preview_sticky_settings.h"
#include "chrome/browser/profiles/chrome_version_service.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_impl.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/push_messaging/push_messaging_app_identifier.h"
#include "chrome/browser/push_messaging/push_messaging_service_impl.h"
#include "chrome/browser/push_messaging/push_messaging_unsubscribed_entry.h"
#include "chrome/browser/rlz/chrome_rlz_tracker_delegate.h"
#include "chrome/browser/search/background/ntp_custom_background_service.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/serial/serial_policy_allowed_ports.h"
#include "chrome/browser/sharing_hub/sharing_hub_features.h"
#include "chrome/browser/signin/chrome_signin_client.h"
#include "chrome/browser/signin/signin_promo_util.h"
#include "chrome/browser/ssl/ssl_config_service_manager.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/tips/core/tips_prefs.h"
#include "chrome/browser/tracing/chrome_tracing_delegate.h"
#include "chrome/browser/ttc/core/prefs.h"
#include "chrome/browser/ui/browser_ui_prefs.h"
#include "chrome/browser/ui/network_profile_bubble.h"
#include "chrome/browser/ui/performance_controls/performance_controls_metrics.h"
#include "chrome/browser/ui/prefs/prefs_tab_helper.h"
#include "chrome/browser/ui/safety_hub/safety_hub_prefs.h"
#include "chrome/browser/ui/side_panel/side_panel_prefs.h"
#include "chrome/browser/ui/tabs/organizer/organizer_prefs.h"
#include "chrome/browser/ui/tabs/tab_strip_prefs.h"
#include "chrome/browser/ui/toolbar/chrome_labs/chrome_labs_prefs.h"
#include "chrome/browser/ui/toolbar/chrome_location_bar_model_delegate.h"
#include "chrome/browser/ui/toolbar/toolbar_pref_names.h"
#include "chrome/browser/ui/webui/accessibility/accessibility_ui.h"
#include "chrome/browser/ui/webui/bookmarks/bookmark_prefs.h"
#include "chrome/browser/ui/webui/flags/flags_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/browser/ui/webui/policy/policy_ui.h"
#include "chrome/browser/ui/webui/print_preview/policy_settings.h"
#include "chrome/browser/updates/announcement_notification/announcement_notification_service.h"
#include "chrome/browser/webauthn/webauthn_pref_names.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/secure_origin_allowlist.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/blocked_content/safe_browsing_triggered_popup_blocker.h"
#include "components/breadcrumbs/core/breadcrumbs_status.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/certificate_transparency/pref_names.h"
#include "components/collaboration/public/pref_names.h"
#include "components/commerce/core/prefs.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/contextual_search/contextual_search_service.h"
#include "components/contextual_tasks/public/prefs.h"
#include "components/custom_handlers/protocol_handler_registry.h"
#include "components/device_signals/core/browser/pref_names.h"  // nogncheck due to crbug.com/40147906
#include "components/dom_distiller/core/distilled_page_prefs.h"
#include "components/domain_reliability/domain_reliability_prefs.h"
#include "components/embedder_support/origin_trials/origin_trial_prefs.h"
#include "components/enterprise/browser/groups/groups_prefs.h"
#include "components/enterprise/browser/identifiers/identifiers_prefs.h"
#include "components/enterprise/browser/promotion/promotion_prefs.h"
#include "components/enterprise/browser/reporting/prefs.h"
#include "components/enterprise/buildflags/buildflags.h"
#include "components/enterprise/connectors/core/connectors_prefs.h"
#include "components/enterprise/isolated_mode/prefs.h"
#include "components/enterprise/net/core/prefs.h"
#include "components/enterprise/network_header_injection/core/network_header_injection_prefs.h"
#include "components/feature_engagement/public/pref_names.h"
#include "components/history_clusters/core/history_clusters_prefs.h"
#include "components/image_fetcher/core/cache/image_cache.h"
#include "components/language/content/browser/geo_language_provider.h"
#include "components/language/content/browser/ulp_language_code_locator/ulp_language_code_locator.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/lens/buildflags.h"
#include "components/lens/lens_overlay_permission_utils.h"
#include "components/lookalikes/core/lookalike_url_util.h"
#include "components/media_device_salt/media_device_id_salt.h"
#include "components/metrics/demographics/user_demographics.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_reporting_choice_service.h"
#include "components/network_time/network_time_tracker.h"
#include "components/ntp_tiles/custom_links_manager_impl.h"
#include "components/ntp_tiles/enterprise/enterprise_shortcuts_manager_impl.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/ntp_tiles/tile_type.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/omnibox/browser/aim_eligibility_service.h"
#include "components/omnibox/browser/document_provider.h"
#include "components/omnibox/browser/omnibox_prefs.h"
#include "components/omnibox/browser/zero_suggest_provider.h"
#include "components/on_device_translation/buildflags/buildflags.h"
#include "components/optimization_guide/core/delivery/prefs.h"
#include "components/optimization_guide/core/feature_registry/feature_registration.h"
#include "components/optimization_guide/core/hints/prefs.h"
#include "components/optimization_guide/core/model_execution/on_device_prefs.h"
#include "components/optimization_guide/core/model_quality/model_quality_prefs.h"
#include "components/page_info/core/merchant_trust_service.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/payments/core/payment_prefs.h"
#include "components/performance_manager/public/user_tuning/prefs.h"
#include "components/permissions/permission_hats_trigger_helper.h"
#include "components/permissions/pref_names.h"
#include "components/personal_context/core/personal_context_prefs.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/browser/url_list/url_blocklist_manager.h"
#include "components/policy/core/browser/url_list/url_list_policy_pref_names.h"
#include "components/policy/core/common/local_test_policy_provider.h"
#include "components/policy/core/common/management/management_service.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/core/common/policy_statistics_collector.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/regional_capabilities/regional_capabilities_prefs.h"
#include "components/safe_browsing/buildflags.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/safety_check/safety_check_prefs.h"
#include "components/saved_tab_groups/public/pref_names.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/security_interstitials/content/insecure_form_blocking_page.h"
#include "components/security_interstitials/content/stateful_ssl_host_state_delegate.h"
#include "components/segmentation_platform/embedder/default_model/device_switcher_result_dispatcher.h"
#include "components/segmentation_platform/public/segmentation_platform_service.h"
#include "components/sessions/core/session_id_generator.h"
#include "components/sharing_message/sharing_sync_preference.h"
#include "components/signin/core/browser/active_primary_accounts_metrics_recorder.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/base/signin_prefs.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/site_engagement/content/site_engagement_service.h"
#include "components/site_token_provider/site_token_provider_prefs.h"
#include "components/skills/public/skills_prefs.h"
#include "components/subresource_filter/content/browser/ruleset_service.h"
#include "components/subresource_filter/core/common/constants.h"
#include "components/subscription_eligibility/subscription_eligibility_prefs.h"
#include "components/supervised_user/core/browser/supervised_user_preferences.h"
#include "components/sync/base/account_pref_utils.h"
#include "components/sync/base/pref_names.h"
#include "components/sync/service/device_statistics_scheduler.h"
#include "components/sync/service/glue/sync_transport_data_prefs.h"
#include "components/sync/service/sync_prefs.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_preferences/cross_device_pref_tracker/prefs/cross_device_pref_registry.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_sessions/session_sync_prefs.h"
#include "components/tracing/common/pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "components/universal_optout/prefs.h"
#include "components/update_client/update_client.h"
#include "components/variations/service/variations_service.h"
#include "components/visited_url_ranking/internal/url_grouping/group_suggestions_service_impl.h"
#include "components/wallet/core/common/wallet_prefs.h"
#include "components/webui/chrome_urls/pref_names.h"
#include "components/webui/flags/pref_service_flags_storage.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/buildflags.h"
#include "extensions/buildflags/buildflags.h"
#include "net/http/http_server_properties_manager.h"
#include "pdf/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "rlz/buildflags/buildflags.h"
#include "ui/accessibility/accessibility_prefs.h"
#include "ui/webui/buildflags.h"

#if BUILDFLAG(ENABLE_WEBUI_NTP)
#include "chrome/browser/new_tab_page/modules/v2/most_relevant_tab_resumption/most_relevant_tab_resumption_page_handler.h"
#include "chrome/browser/ui/webui/cr_components/most_visited/most_visited_pref_observer.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_handler.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/new_tab_footer/new_tab_footer_ui.h"
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/extensions/background_mode_manager.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "chrome/browser/extensions/commands/command_service.h"
#include "chrome/browser/extensions/component_loader_prefs.h"
#include "chrome/browser/extensions/extension_url_overrides.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/low_trust_policy_install_block_manager.h"
#include "chrome/browser/extensions/preinstalled_extensions.h"
#include "chrome/browser/ui/extensions/extension_settings_overridden_dialog.h"
#include "chrome/browser/ui/extensions/settings_api_bubble_helpers.h"
#include "chrome/browser/ui/webui/extensions/extensions_ui_prefs.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/permissions_manager.h"
#include "extensions/browser/pref_names.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/accessibility/animation_policy_prefs.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_PDF)
#include "chrome/browser/pdf/pdf_pref_names.h"
#endif  // BUILDFLAG(ENABLE_PDF)

#include "chrome/browser/ui/webui/management/management_ui.h"


#include "chrome/browser/actor/ui/actor_ui_state_manager_prefs.h"
#include "chrome/browser/child_module/child_module_manager.h"
#include "chrome/browser/desktop_to_mobile_promos/promos_utils.h"  // nogncheck crbug.com/40147906
#include "chrome/browser/gcm/gcm_product_util.h"
#include "chrome/browser/indigo/indigo_prefs.h"
#include "chrome/browser/media/router/discovery/access_code/access_code_cast_feature.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/new_tab_page/modules/file_suggestion/microsoft_files_page_handler.h"
#include "chrome/browser/new_tab_page/modules/v2/calendar/outlook_calendar_page_handler.h"
#include "chrome/browser/screen_ai/pref_names.h"
#include "chrome/browser/search_engine_choice/search_engine_choice_dialog_service.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/speech/speech_recognition_small_expert_model_installer.h"
#include "chrome/browser/task_manager/task_manager_interface.h"
#include "chrome/browser/ui/commerce/commerce_ui_tab_helper.h"
#include "chrome/browser/ui/hats/hats_service_desktop.h"
#include "chrome/browser/ui/omnibox/omnibox_everywhere/omnibox_everywhere_prefs.h"
#include "chrome/browser/ui/read_anything/read_anything_prefs.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "chrome/browser/ui/tabs/saved_tab_groups/saved_tab_group_pref_names.h"
#include "chrome/browser/ui/tabs/tab_strip_prefs.h"
#include "chrome/browser/ui/webui/certificate_manager/certificate_manager_handler.h"
#include "chrome/browser/ui/webui/history/foreign_session_handler.h"
#include "chrome/browser/ui/webui/settings/settings_ui.h"
#include "chrome/browser/ui/webui/tab_search/tab_search_prefs.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "chrome/browser/user_education/browser_user_education_storage_service.h"
#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"
#include "components/headless/policy/headless_mode_prefs.h"  // nogncheck crbug.com/40147906
#include "components/live_caption/live_caption_controller.h"
#include "components/live_caption/live_translate_controller.h"

#include "chrome/browser/ui/ai_overlay_dialog/ai_overlay_dialog_controller.h"

#if BUILDFLAG(ENABLE_DEVTOOLS_FRONTEND)
#include "chrome/browser/devtools/devtools_window.h"
#endif  // BUILDFLAG(ENABLE_DEVTOOLS_FRONTEND)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "chrome/browser/ui/webui/whats_new/whats_new_ui.h"
#endif


#if BUILDFLAG(IS_MAC)
#include "chrome/browser/media/webrtc/system_media_capture_permissions_stats_mac.h"
#include "chrome/browser/ui/cocoa/apps/quit_with_apps_controller_mac.h"
#include "chrome/browser/ui/cocoa/confirm_quit.h"
#include "chrome/browser/ui/views/frame/glass_frame_service.h"
#include "chrome/browser/web_applications/os_integration/mac/app_shim_registry.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/font_prewarmer_tab_helper.h"
#include "chrome/browser/media/cdm_pref_service_helper.h"
#include "chrome/browser/media/media_foundation_service_monitor.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "chrome/browser/webnn/webnn_prefs.h"
#include "components/os_crypt/async/browser/dpapi_key_provider.h"
#include "components/os_crypt/async/browser/os_crypt_win.h"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#include "chrome/browser/enterprise/platform_auth/platform_auth_policy_observer.h"
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "chrome/browser/browser_switcher/browser_switcher_prefs.h"
#include "chrome/browser/enterprise/signin/enterprise_signin_prefs.h"
#include "chrome/browser/lifetime/scheduled_restart_manager.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/ui/startup/first_run_service.h"
#endif

#if BUILDFLAG(ENABLE_DOWNGRADE_PROCESSING)
#include "chrome/browser/downgrade/downgrade_prefs.h"  // nogncheck
#endif

#include "chrome/browser/device_identity/device_oauth2_token_store_desktop.h"
#include "chrome/browser/ui/startup/default_browser_prompt/default_browser_prompt.h"

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/browser_view_prefs.h"
#include "chrome/browser/ui/side_search/side_search_prefs.h"
#endif

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_data_service.h"
#include "chrome/browser/sessions/session_service_log.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "ui/color/system_theme.h"
#endif

#if BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
#include "components/on_device_translation/public/pref_names.h"
#endif  // BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)

#if BUILDFLAG(ENTERPRISE_DATA_CONTROLS)
#include "components/enterprise/data_controls/core/browser/prefs.h"
#endif

#if BUILDFLAG(SAFE_BROWSING_AVAILABLE)
#include "components/safe_browsing/content/common/file_type_policies/file_type_policies_prefs.h"
#endif

#if BUILDFLAG(CHROME_FOR_TESTING)
#include "chrome/browser/chrome_for_testing/prefs.h"
#endif

namespace {

// Please keep the list of deprecated prefs in chronological order. i.e. Add to
// the bottom of the list, not here at the top.


// Deprecated 09/2025.
constexpr char kGaiaCookieLastListAccountsData[] =
    "gaia_cookie.last_list_accounts_data";

// Deprecated 09/2025.
constexpr char kLensOverlayEduActionChipShownCount[] =
    "lens.edu_action_chip.shown_count";

// Deprecated 10/2025.
constexpr char kSessionRestoreTurnOffFromRestartInfoBarTimesShown[] =
    "browser.session_restore_turn_off_from_restart_infobar_times_shown";

constexpr char kSessionRestoreTurnOffFromSessionInfoBarTimesShown[] =
    "browser.session_restore_turn_off_from_session_infobar_times_shown";

constexpr char kSessionRestorePrefChanged[] = "session.restore_pref_changed";

constexpr char kLegacySyncSessionsGUID[] = "sync.session_sync_guid";

const char kRefreshHeuristicBreakageException[] =
    "fingerprinting_protection_filter.refresh_heuristic_breakage_exception_"
    "sites";

const char kFpfRulesetContent[] =
    "fingerprinting_protection_filter.ruleset_version.content";

const char kFpfRulesetFormat[] =
    "fingerprinting_protection_filter.ruleset_version.format";

const char kFpfRulesetChecksum[] =
    "fingerprinting_protection_filter.ruleset_version.checksum";

// Deprecated 12/2025.
const char kPrivacyBudgetGeneration[] = "privacy_budget.generation";
const char kPrivacyBudgetSeenSurfaces[] = "privacy_budget.seen";
const char kPrivacyBudgetSelectedOffsets[] = "privacy_budget.selected";
const char kPrivacyBudgetSelectedBlock[] = "privacy_budget.block_offset";
const char kPrivacyBudgetMetaExperimentActivationSalt[] =
    "privacy_budget.meta_experiment_activation_salt";

// Preference key for Enterprise policy UserAgentReduction which is distinct
// from blink::features::kReduceUserAgentMinorVersion.
constexpr char kReduceUserAgentMinorVersion[] = "user_agent_reduction";

// Deprecated 12/2025.
constexpr char kAutofillStatesDataDir[] = "autofill.states_data_dir";
constexpr char kMerchantTrustUiLastInteractionTime[] =
    "merchant_trust.ui.last_interaction_time";
constexpr char kMerchantTrustPageInfoLastOpenTime[] =
    "merchant_trust.page_info.last_open_time";

// Deprecated 12/2025.
constexpr char kCloudPrintProxyEnabled[] = "cloud_print.enabled";
constexpr char kCloudPrintEmail[] = "cloud_print.email";

// Deprecated 12/2025.
constexpr char kTrackingProtectionEligibleSince[] =
    "tracking_protection.tracking_protection_eligible_since";
constexpr char kTrackingProtectionOnboardedSince[] =
    "tracking_protection.tracking_protection_onboarded_since";
constexpr char kTrackingProtectionNoticeLastShown[] =
    "tracking_protection.tracking_protection_notice_last_shown";
constexpr char kTrackingProtectionOnboardingAckedSince[] =
    "tracking_protection.tracking_protection_onboarding_acked_since";
constexpr char kTrackingProtectionOnboardingAcked[] =
    "tracking_protection.tracking_protection_onboarding_acked";
constexpr char kTrackingProtectionOnboardingAckAction[] =
    "tracking_protection.tracking_protection_onboarding_ack_action";
constexpr char kTrackingProtectionSilentEligibleSince[] =
    "tracking_protection.tracking_protection_silent_eligible_since";
constexpr char kTrackingProtectionSilentOnboardedSince[] =
    "tracking_protection.tracking_protection_silent_onboarded_since";
constexpr char kAllowAll3pcToggleEnabled[] =
    "tracking_protection.allow_all_3pc_toggle_enabled";
constexpr char kTrackingProtectionLevel[] =
    "tracking_protection.tracking_protection_level";
constexpr char kIpProtectionEnabled[] =
    "tracking_protection.ip_protection_enabled";
constexpr char kIpProtectionInitializedByDogfood[] =
    "tracking_protection.ip_protection_initialized_by_dogfood";
constexpr char kUserBypass3pcExceptionsMigrated[] =
    "tracking_protection.user_bypass_3pc_exceptions_migrated";
constexpr char kTrackingProtectionSilentOnboardingStatus[] =
    "tracking_protection.tracking_protection_silent_onboarding_status";
constexpr char kFingerprintingProtectionEnabled[] =
    "tracking_protection.fingerprinting_protection_enabled";
constexpr char kTrackingProtectionOnboardingStatus[] =
    "tracking_protection.tracking_protection_onboarding_status";
constexpr char kTPCDExperimentClientState[] = "tpcd_experiment.client_state";
constexpr char kTPCDExperimentClientStateVersion[] =
    "tpcd_experiment.client_state_version";
constexpr char kTPCDExperimentProfileState[] = "tpcd_experiment.profile_state";



// Deprecated 01/2026.
constexpr char kCookieClearOnExitMigrationNoticeComplete[] =
    "signin.cookie_clear_on_exit_migration_notice_complete";

// Deprecated 02/2026.
// Note that these were replaced by local state prefs of the same names and
// functions: those should not be removed. Only the profile prefs registered
// here are deprecated.
constexpr char kGlicGuestUrlPresetAutopush[] = "glic.guest_url_preset_autopush";
constexpr char kGlicGuestUrlPresetPreprod[] = "glic.guest_url_preset_preprod";
constexpr char kGlicGuestUrlPresetProd[] = "glic.guest_url_preset_prod";

// Deprecated 02/2026.
constexpr char kProfilesDeletedOld[] = "profiles.profiles_deleted";

// Deprecated 02/2026.
inline constexpr char kExplicitBrowserSigninWithoutFeatureEnabled[] =
    "signin.explicit_browser_signin";

// Deprecated 02/2026.
constexpr char kDiceMigrationDialogShownCount[] =
    "signin.dice_migration.dialog_shown_count";
constexpr char kDiceMigrationDialogLastShownTime[] =
    "signin.dice_migration.dialog_last_shown_time";
constexpr char kDiceMigrationBackup[] = "signin.dice_migration.backup";
constexpr char kDiceMigrationRestoredFromBackup[] =
    "signin.dice_migration.restored_from_backup";

// Deprecated 02/2026.
inline constexpr char kTabSearchOpened[] = "tab_search.opened";

// Deprecated 02/2026.
constexpr char kTabOrganizationFeature[] = "tab_organization.feature";

// Deprecated 03/2026.
constexpr char kTabDeclutterUsageCount[] = "tab_declutter.usage_count";

// Deprecated 03/2026.
inline constexpr char kTabSearchTabIndex[] = "tab_search.tab_index";

// Deprecated 03/2026.
constexpr char kGlicMultiInstanceEnabledBySubscriptionTier[] =
    "glic.multi_instance_enabled_by_tier";

// Deprecated 03/2026
constexpr char kSigninFromBookmarksBubbleSyntheticTrialGroupNamePref[] =
    "UnoDesktopBookmarksEnabledInAccountFromBubbleGroup";
constexpr char kBookmarksBubblePromoShownSyntheticTrialGroupNamePref[] =
    "UnoDesktopBookmarksBubblePromoShownGroup";


// Deprecated 03/2026.
constexpr char kTabOrganizationNudgeBackoffCount[] =
    "tab_organization.nudge_backoff_count";
constexpr char kTabOrganizationShowFRE[] = "tab_organization.show_fre_2";
constexpr char kTabOrganizationModelStrategy[] =
    "tab_organization.model_strategy";

// Deprecated 03/2026.
constexpr char kNtpContextMenuClickCount[] = "ntp.context_menu_click_count";

// Deprecated 03/2026.
constexpr char kNtpPromoPrefLastSnoozed[] =
    "in_product_help.ntp_promos.last_snoozed";

// Deprecated 03/2026.
constexpr char kSafeBrowsingModuleShownCount[] =
    "safebrowsing.ntp.module_shown_count";
constexpr char kSafeBrowsingModuleLastCooldownStartAt[] =
    "safebrowsing.ntp.last_cooldown_start_timestamp";
constexpr char kSafeBrowsingModuleOpened[] =
    "safebrowsing.ntp.user_opened_module";

// Deprecated 04/2026.
constexpr char kTpcdMetadataCohorts[] = "tpcd.metadata.cohorts";

// Deprecated 04/2026.
constexpr char kHasSeenWebFeed[] = "webfeed.has_seen_feed";
constexpr char kLastBadgeAnimationTime[] = "webfeed.last_badge_animation_time";

// Deprecated 04/2026.
inline constexpr char kPreallocatedAddressesVersion[] =
    "plus_addresses.preallocation.version";
inline constexpr char kPreallocatedAddresses[] =
    "plus_addresses.preallocation.addresses";
inline constexpr char kPreallocatedAddressesNext[] =
    "plus_addresses.preallocation.next";
inline constexpr char kFirstPlusAddressCreationTime[] =
    "plus_addresses.creation.first.time";
inline constexpr char kLastPlusAddressFillingTime[] =
    "plus_addresses.last.filling.time";

// Deprecated 05/2026.
constexpr char kWebFeedContentOrder[] = "webfeed.content_order";
constexpr char kWebFeedsRequestSchedule[] = "webfeed.request_schedule";
constexpr char kEnableWebFeedFollowIntroDebug[] =
    "webfeed_follow_intro_debug.enable";
constexpr char kLastSeenFeedType[] = "feedv2.last_seen_feed_type";

// Deprecated 05/2026.
constexpr char kShouldShowRemoteAnnotatorFirstRunInfo[] =
    "accessibility_annotator.should_show_remote_annotator_first_run_info";

// Deprecated 08/2026.
constexpr char kUkmLoggingUserSecret[] =
    "accessibility_annotator.ukm_logging_user_secret";
constexpr char kUkmLoggingUserSecretCreationTime[] =
    "accessibility_annotator.ukm_logging_user_secret_creation_time";
constexpr char kObsoleteNtpPromoBlocklist[] = "ntp.promo_blocklist";

// Deprecated 05/2026.
constexpr char kHttpCacheFinchExperimentGroups[] =
    "profile_network_context_service.http_cache_finch_experiment_groups";

// Deprecated 05/2026.
constexpr char kContextualCueingEnterprisePolicyAllowedDeprecated[] =
    "optimization_guide.model_execution.contextual_cueing_enterprise_policy_"
    "allowed";

// Deprecated 06/2026.
inline constexpr char kDeleteTimePeriodBasic[] =
    "browser.clear_data.time_period_basic";
inline constexpr char kDeleteBrowsingHistoryBasic[] =
    "browser.clear_data.browsing_history_basic";
inline constexpr char kDeleteCacheBasic[] = "browser.clear_data.cache_basic";
inline constexpr char kDeleteCookiesBasic[] =
    "browser.clear_data.cookies_basic";
inline constexpr char kLastClearBrowsingDataTab[] =
    "browser.last_clear_browsing_data_tab";

// Deprecated 06/2026.
constexpr char kGlicSelectionWidgetDismissCount[] =
    "glic.selection_widget_dismiss_count";

// Deprecated 06/2026.
inline constexpr char kRealboxContextMenuAnimationState[] =
    "realbox.context_menu_animation_state";

// Deprecated 06/2026.
inline constexpr char kTabSearchMigrationComplete[] =
    "toolbar.tab_search_migration_complete";
inline constexpr char kTabSearchPinnedToTabstripMigrationComplete[] =
    "tab_search.pinned_to_tabstrip_migration_complete";
inline constexpr char kTabSearchPinnedToTabstripMigrationComplete2[] =
    "tab_search.pinned_to_tabstrip_migration_complete_2";

// Deprecated 06/2026.
constexpr char kPersonalContextInAutofillNoticeShouldBeShown[] =
    "autofill.personal_context.notice_should_be_shown";

// Deprecated 06/2026.
inline constexpr char kDefaultBrowserInfobarLastDeclined[] =
    "browser.default_browser_infobar_last_declined";


// Deprecated 07/2026.
inline constexpr char kObsoleteMetricsReportingLevel[] =
    "user_experience_metrics.reporting_level";
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
inline constexpr char kProxyOverrideRulesAffiliation[] =
    "proxy_override_rules_affiliation";
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
// Deprecated 07/2026.
inline constexpr char kMV2DeprecationWarningAcknowledgedGlobally[] =
    "mv2_deprecation_warning_ack_globally";
inline constexpr char kMV2DeprecationDisabledAcknowledgedGlobally[] =
    "mv2_deprecation_disabled_ack_globally";
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

// Deprecated 07/2026.
inline constexpr char kObsoleteManagementPlatformLastLogTime[] =
    "management.platform.last_log_time";
inline constexpr char kObsoleteManagementProfileLastLogTime[] =
    "management.profile.last_log_time";

// Deprecated 07/2026.
constexpr char kMetricsReportingMigrationDone[] =
    "user_experience_metrics.consent_migration_done";
constexpr char kMetricsConsentRestructureFeatureState[] =
    "user_experience_metrics.consent_restructure_feature_state";

// Deprecated 08/2026.
constexpr char kInitialSendAnimationShown[] =
    "send_tab_to_self.initial_animation_shown";
constexpr char kPrivacySandboxNotices[] = "privacy_sandbox.notices";
constexpr char kPrivacySandboxM1ConsentDecisionMade[] =
    "privacy_sandbox.m1.consent_decision_made";
constexpr char kPrivacySandboxM1EEANoticeAcknowledged[] =
    "privacy_sandbox.m1.eea_notice_acknowledged";
constexpr char kPrivacySandboxM1RowNoticeAcknowledged[] =
    "privacy_sandbox.m1.row_notice_acknowledged";
constexpr char kPrivacySandboxM1RestrictedNoticeAcknowledged[] =
    "privacy_sandbox.m1.restricted_notice_acknowledged";
constexpr char kPrivacySandboxM1PromptSuppressed[] =
    "privacy_sandbox.m1.prompt_suppressed";
constexpr char kPrivacySandboxNoticeDisplayed[] =
    "privacy_sandbox.notice_displayed";
constexpr char kPrivacySandboxConsentDecisionMade[] =
    "privacy_sandbox.consent_decision_made";
constexpr char kPrivacySandboxNoConfirmationSandboxDisabled[] =
    "privacy_sandbox.no_confirmation_sandbox_disabled";
constexpr char kPrivacySandboxNoConfirmationSandboxRestricted[] =
    "privacy_sandbox.no_confirmation_sandbox_restricted";
constexpr char kPrivacySandboxNoConfirmationSandboxManaged[] =
    "privacy_sandbox.no_confirmation_sandbox_managed";
constexpr char kPrivacySandboxNoConfirmationThirdPartyCookiesBlocked[] =
    "privacy_sandbox.no_confirmation_3PC_blocked";
constexpr char kPrivacySandboxNoConfirmationManuallyControlled[] =
    "privacy_sandbox.no_confirmation_manually_controlled";
constexpr char kPrivacySandboxDisabledInsufficientConfirmation[] =
    "privacy_sandbox.disabled_insufficient_confirmation";
constexpr char kPrivacySandboxTopicsConsentGiven[] =
    "privacy_sandbox.topics_consent.consent_given";
constexpr char kPrivacySandboxTopicsConsentLastUpdateTime[] =
    "privacy_sandbox.topics_consent.last_update_time";
constexpr char kPrivacySandboxTopicsConsentLastUpdateReason[] =
    "privacy_sandbox.topics_consent.last_update_reason";
constexpr char kPrivacySandboxTopicsConsentTextAtLastUpdate[] =
    "privacy_sandbox.topics_consent.text_at_last_update";
constexpr char kPrivacySandboxAllowNoticeFor3PCBlockedTrial[] =
    "privacy_sandbox.allow_notice_for_3PC_blocked_trial";
constexpr char kObsoleteAutofillWalletImportEnabled[] =
    "autofill.wallet_import_enabled";
constexpr char kObsoleteAutofillWalletImportEnabledMigrated[] =
    "sync.autofill_wallet_import_enabled_migrated";
constexpr char kPrivacySandboxTopicsDataAccessibleSince[] =
    "privacy_sandbox.topics_data_accessible_since";
constexpr char kPrivacySandboxBlockedTopics[] =
    "privacy_sandbox.blocked_topics";
constexpr char kPrivacySandboxFledgeJoinBlocked[] =
    "privacy_sandbox.fledge_join_blocked";
constexpr char kShowRollbackUiModeB[] =
    "tracking_protection.show_rollback_ui_mode_b";
constexpr char kTrackingProtection3pcdEnabled[] =
    "tracking_protection.tracking_protection_3pcd_enabled";
constexpr char kBlockAll3pcToggleEnabled[] =
    "tracking_protection.block_all_3pc_toggle_enabled";
inline constexpr char kExternalAppRedirectTimestamps[] =
    "safe_browsing.external_app_redirect_timestamps";

// Deprecated 08/2026.
constexpr char kEverythingMenuPinnedToTabstripMigrationComplete[] =
    "everything_menu.pinned_to_tabstrip_migration_complete";

// Deprecated 08/2026.
constexpr char kSigninInterceptionIDPCookiesUrl[] =
    "signin.interception.idp_cookies.url";



// Deprecated 09/2026.
inline constexpr char kInvalidationPerSenderRegisteredForInvalidation[] =
    "invalidation.per_sender_registered_for_invalidation";
inline constexpr char kInvalidationPerSenderActiveRegistrationTokens[] =
    "invalidation.per_sender_active_registration_tokens";
inline constexpr char kBookmarkEditorExpandedNodes[] =
    "bookmark_editor.expanded_nodes";


// Deprecated 09/2026.
#if BUILDFLAG(ENABLE_WEBUI_NTP)
constexpr char kNtpDriveLastDismissedTime[] =
    "NewTabPage.Drive.LastDimissedTime";
constexpr char kNtpGoogleCalendarLastDismissedTime[] =
    "NewTabPage.GoogleCalendar.LastDimissedTime";
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)
constexpr char kNtpMicrosoftAuthLastDismissedTime[] =
    "NewTabPage.MicrosoftAuthentication.LastDimissedTime";
constexpr char kNtpTabGroupsLastDismissedTime[] =
    "NewTabPage.TabGroups.LastDimissedTime";
// Deprecated on non-ChromePS platforms on 09/2026. Still used on ChromeOS.
constexpr char kAudioApiStableDeviceIds[] =
    "extensions.audio.stable_device_ids";

// Deprecated 09/2026.
constexpr char kAddressSignInPromoShownCountPerProfileForLimitsExperiment[] =
    "signin.AddressSignInPromoShownCountForLimitsExperiment";
constexpr char kBookmarkSignInPromoShownCountPerProfileForLimitsExperiment[] =
    "signin.BookmarkSignInPromoShownCountForLimitsExperiment";
constexpr char kPasswordSignInPromoShownCountPerProfileForLimitsExperiment[] =
    "signin.PasswordSignInPromoShownCountForLimitsExperiment";
constexpr char kAddressSignInPromoDismissCountPerProfileForLimitsExperiment[] =
    "signin.AddressSignInPromoDismissCountForLimitsExperiment";
constexpr char kBookmarkSignInPromoDismissCountPerProfileForLimitsExperiment[] =
    "signin.BookmarkSignInPromoDismissCountForLimitsExperiment";
constexpr char kPasswordSignInPromoDismissCountPerProfileForLimitsExperiment[] =
    "signin.PasswordSignInPromoDismissCountForLimitsExperiment";



// Deprecated 10/2026.
inline constexpr char kDismissedHistoryEmbeddingsScopePromo[] =
    "omnibox.dismissed_history_embeddings_scope_promo";
inline constexpr char kDismissedHistoryEmbeddingsSettingsPromo[] =
    "omnibox.dismissed_history_embeddings_settings_promo";
inline constexpr char kShownCountHistoryEmbeddingsScopePromo[] =
    "omnibox.shown_count_history_embeddings_scope_promo";
inline constexpr char kShownCountHistoryEmbeddingsSettingsPromo[] =
    "omnibox.shown_count_history_embeddings_settings_promo";
inline constexpr char kHistorySearchSettingState[] =
    "optimization_guide.history_search_setting_state";

// Deprecated 10/2026.
constexpr char kSupervisedUserSafeSites[] = "profile.managed.safe_sites";
constexpr char kSupervisedUserManualHosts[] = "profile.managed.manual_hosts";
constexpr char kSupervisedUserManualUrls[] = "profile.managed.manual_urls";

// Deprecated 10/2026.
inline constexpr char kTabSearchUsed[] = "tab_search.used";


// Deprecated 10/2026.
inline constexpr char kSyncEncryptionBootstrapToken[] =
    "sync.encryption_bootstrap_token";
inline constexpr char kSyncEncryptionBootstrapTokenPerAccountMigrationDone[] =
    "sync.encryption_bootstrap_token_per_account_migration_done";

// Deprecated 10/2026.
inline constexpr char kTailoredSecuritySyncFlowLastRunTime[] =
    "safebrowsing.aesb_sync_flow_start_timestamp";
inline constexpr char kTailoredSecuritySyncFlowLastUserInteractionState[] =
    "safebrowsing.aesb_sync_flow_last_user_interaction_state";
inline constexpr char kTailoredSecuritySyncFlowObservedOutcomeUnsetTimestamp[] =
    "safebrowsing.aesb_sync_flow_observed_outcome_unset_timestamp";
inline constexpr char kSyncingThemePrefsMigratedToNonSyncing[] =
    "syncing_theme_prefs_migrated_to_non_syncing";
inline constexpr char kShouldReadIncomingSyncingThemePrefs[] =
    "should_read_incoming_syncing_theme_prefs";
inline constexpr char kDeprecatedBrowserColorSchemeDoNotUse[] =
    "browser.theme.color_scheme";
inline constexpr char kDeprecatedUserColorDoNotUse[] =
    "browser.theme.user_color";
inline constexpr char kDeprecatedBrowserColorVariantDoNotUse[] =
    "browser.theme.color_variant";
inline constexpr char kDeprecatedGrayscaleThemeEnabledDoNotUse[] =
    "browser.theme.is_grayscale";
inline constexpr char kDeprecatedNtpCustomBackgroundDictDoNotUse[] =
    "ntp.custom_background_dict";

// Deprecated 10/2026.
inline constexpr char kTabsFromOtherDevicesAutoPinnedMigration[] =
    "toolbar.tabs_from_other_devices_auto_pinned_migration";

// Deprecated 11/2026.
inline constexpr char kBundledSettingsCheckedMigrateUserToEnhancedBundle[] =
    "safebrowsing.bundled_settings.checked_migrate_user_to_enhanced_bundle";

// Deprecated 11/2026.
inline constexpr char kGlicPromotionSourceCohort[] =
    "glic.promotion_source_cohort";


// Register local state used only for migration (clearing or moving to a new
// key).
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Deprecated 11/2025.
  registry->RegisterStringPref(kFpfRulesetContent, std::string());
  registry->RegisterIntegerPref(kFpfRulesetFormat, 0);
  registry->RegisterUint64Pref(kFpfRulesetChecksum, 0);

  // Deprecated 12/2025.
  registry->RegisterIntegerPref(kPrivacyBudgetGeneration, 0);
  registry->RegisterStringPref(kPrivacyBudgetSeenSurfaces, std::string());
  registry->RegisterStringPref(kPrivacyBudgetSelectedOffsets, std::string());

  // Deprecated 12/2025.
  registry->RegisterStringPref(kAutofillStatesDataDir, std::string());

  // Deprecated 12/2025.
  registry->RegisterBooleanPref(kFingerprintingProtectionEnabled, false);
  registry->RegisterBooleanPref(kIpProtectionEnabled, false);
  registry->RegisterBooleanPref(kAllowAll3pcToggleEnabled, false);
  registry->RegisterBooleanPref(kUserBypass3pcExceptionsMigrated, false);
  registry->RegisterIntegerPref(kTrackingProtectionLevel, 0);
  registry->RegisterTimePref(kTrackingProtectionSilentOnboardedSince,
                             base::Time());
  registry->RegisterTimePref(kTrackingProtectionSilentEligibleSince,
                             base::Time());
  registry->RegisterTimePref(kTrackingProtectionEligibleSince, base::Time());
  registry->RegisterTimePref(kTrackingProtectionOnboardedSince, base::Time());
  registry->RegisterTimePref(kTrackingProtectionNoticeLastShown, base::Time());
  registry->RegisterTimePref(kTrackingProtectionOnboardingAckedSince,
                             base::Time());
  registry->RegisterBooleanPref(kTrackingProtectionOnboardingAcked, false);
  registry->RegisterIntegerPref(kTrackingProtectionOnboardingAckAction, 0);
  registry->RegisterBooleanPref(kIpProtectionInitializedByDogfood, false);
  registry->RegisterIntegerPref(kTrackingProtectionSilentOnboardingStatus, 0);
  registry->RegisterIntegerPref(kTrackingProtectionOnboardingStatus, 0);
  registry->RegisterIntegerPref(kTPCDExperimentClientState, 0);
  registry->RegisterIntegerPref(kTPCDExperimentClientStateVersion, 0);
  registry->RegisterIntegerPref(kTPCDExperimentProfileState, 0);


  // Deprecated 02/2026.
  registry->RegisterListPref(kProfilesDeletedOld);

  // Deprecated 03/2026.
  registry->RegisterBooleanPref(kGlicMultiInstanceEnabledBySubscriptionTier,
                                false);
  registry->RegisterIntegerPref(kPrivacyBudgetSelectedBlock, -1);
  registry->RegisterDoublePref(kPrivacyBudgetMetaExperimentActivationSalt, 0);

  // Deprecated 04/2026.
  registry->RegisterDictionaryPref(kTpcdMetadataCohorts);

  // Deprecated 04/2026.
  registry->RegisterBooleanPref(kHasSeenWebFeed, false);
  registry->RegisterTimePref(kLastBadgeAnimationTime, base::Time());

  // Deprecated 05/2026.
  registry->RegisterStringPref(kHttpCacheFinchExperimentGroups, "");


  // Deprecated 07/2026.
  registry->RegisterIntegerPref(kObsoleteMetricsReportingLevel, 0);
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(kProxyOverrideRulesAffiliation, true);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(kMetricsReportingMigrationDone, false);
  registry->RegisterBooleanPref(kMetricsConsentRestructureFeatureState, false);

  // Deprecated 07/2026.
  registry->RegisterTimePref(kObsoleteManagementPlatformLastLogTime,
                             base::Time());

  // Deprecated 09/2026.
  registry->RegisterDictionaryPref(
      kInvalidationPerSenderRegisteredForInvalidation);
  registry->RegisterDictionaryPref(
      kInvalidationPerSenderActiveRegistrationTokens);


}

// Register prefs used only for migration (clearing or moving to a new key).
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Deprecated 08/2025.
  // TODO(crbug.com/557083938): Cleanup migrated pref.
  registry->RegisterBooleanPref(ntp_prefs::kNtpUseMostVisitedTiles, false);


  // Deprecated 09/2025.
  registry->RegisterStringPref(kGaiaCookieLastListAccountsData, std::string());

  // Deprecated 09/2025.
  registry->RegisterIntegerPref(kLensOverlayEduActionChipShownCount, 0);

  // Deprecated 10/2025.
  registry->RegisterIntegerPref(
      kSessionRestoreTurnOffFromRestartInfoBarTimesShown, 0);

  // Deprecated 10/2025.
  registry->RegisterIntegerPref(
      kSessionRestoreTurnOffFromSessionInfoBarTimesShown, 0);

  // Deprecated 10/2025.
  registry->RegisterBooleanPref(kSessionRestorePrefChanged, false);

  // Deprecated 10/2025.
  registry->RegisterIntegerPref(ntp_prefs::kNtpShortcutsType, 0);

  // Deprecated 10/2025.
  registry->RegisterStringPref(kLegacySyncSessionsGUID, std::string());

  // Deprecated 11/2025.
  registry->RegisterDictionaryPref(kRefreshHeuristicBreakageException);

  // Deprecated 12/2025.
  registry->RegisterBooleanPref(kReduceUserAgentMinorVersion, false);
  registry->RegisterTimePref(kMerchantTrustUiLastInteractionTime, base::Time());
  registry->RegisterTimePref(kMerchantTrustPageInfoLastOpenTime, base::Time());

  // Deprecated 12/2025.
  registry->RegisterBooleanPref(kCloudPrintProxyEnabled, true);
  registry->RegisterStringPref(kCloudPrintEmail, std::string());


  // Deprecated 01/2026.
  registry->RegisterBooleanPref(kCookieClearOnExitMigrationNoticeComplete,
                                false);

  // Deprecated 02/2026.
  registry->RegisterStringPref(kGlicGuestUrlPresetAutopush, std::string());
  registry->RegisterStringPref(kGlicGuestUrlPresetPreprod, std::string());
  registry->RegisterStringPref(kGlicGuestUrlPresetProd, std::string());

  // Deprecated 02/2026.
  registry->RegisterBooleanPref(kExplicitBrowserSigninWithoutFeatureEnabled,
                                false);

  // Deprecated 02/2026.
  registry->RegisterIntegerPref(kDiceMigrationDialogShownCount, 0);
  registry->RegisterTimePref(kDiceMigrationDialogLastShownTime, base::Time());
  registry->RegisterDictionaryPref(kDiceMigrationBackup);
  registry->RegisterBooleanPref(kDiceMigrationRestoredFromBackup, false);

  // Deprecated 02/2026.
  registry->RegisterBooleanPref(kTabSearchOpened, false);

  // Deprecated 02/2026.
  registry->RegisterIntegerPref(kTabOrganizationFeature, 0);

  // Deprecated 03/2026.
  registry->RegisterIntegerPref(kTabDeclutterUsageCount, 0);

  // Deprecated 03/2026.
  registry->RegisterIntegerPref(kTabSearchTabIndex, 1);

  // Deprecated 03/2026.
  registry->RegisterStringPref(
      kSigninFromBookmarksBubbleSyntheticTrialGroupNamePref, std::string());
  registry->RegisterStringPref(
      kBookmarksBubblePromoShownSyntheticTrialGroupNamePref, std::string());


  // Deprecated 03/2026.
  registry->RegisterIntegerPref(kTabOrganizationNudgeBackoffCount, 0);
  registry->RegisterBooleanPref(kTabOrganizationShowFRE, true);
  registry->RegisterIntegerPref(kTabOrganizationModelStrategy, 0);

  // Deprecated 03/2026.
  registry->RegisterIntegerPref(kNtpContextMenuClickCount, 0);

  // Deprecated 03/2026.
  registry->RegisterTimePref(kNtpPromoPrefLastSnoozed, base::Time());

  // Deprecated 03/2026.
  registry->RegisterIntegerPref(kSafeBrowsingModuleShownCount, 0);
  registry->RegisterInt64Pref(kSafeBrowsingModuleLastCooldownStartAt, 0);
  registry->RegisterBooleanPref(kSafeBrowsingModuleOpened, false);

  // Deprecated 04/2026.
  registry->RegisterIntegerPref(kPreallocatedAddressesVersion, 1);
  registry->RegisterListPref(kPreallocatedAddresses);
  registry->RegisterIntegerPref(kPreallocatedAddressesNext, 0);
  registry->RegisterTimePref(kFirstPlusAddressCreationTime, base::Time());
  registry->RegisterTimePref(kLastPlusAddressFillingTime, base::Time());

  // Deprecated 05/2026.
  registry->RegisterIntegerPref(kWebFeedContentOrder, 0);
  registry->RegisterDictionaryPref(kWebFeedsRequestSchedule);
  registry->RegisterBooleanPref(kEnableWebFeedFollowIntroDebug, false);
  registry->RegisterIntegerPref(kLastSeenFeedType, 0);

  // Deprecated 05/2026.
  registry->RegisterBooleanPref(kShouldShowRemoteAnnotatorFirstRunInfo, true);

  // Deprecated 05/2026.
  registry->RegisterIntegerPref(
      kContextualCueingEnterprisePolicyAllowedDeprecated, 0);

  // Deprecated 06/2026.
  registry->RegisterIntegerPref(kDeleteTimePeriodBasic, 0);
  registry->RegisterBooleanPref(kDeleteBrowsingHistoryBasic, true);
  registry->RegisterBooleanPref(kDeleteCacheBasic, true);
  registry->RegisterBooleanPref(kDeleteCookiesBasic, true);
  registry->RegisterIntegerPref(kLastClearBrowsingDataTab, 0);
  registry->RegisterIntegerPref(kGlicSelectionWidgetDismissCount, 0);

  // Deprecated 06/2026.
  registry->RegisterDictionaryPref(kRealboxContextMenuAnimationState);

  // Deprecated 06/2026.
  registry->RegisterBooleanPref(kTabSearchMigrationComplete, true);
  registry->RegisterBooleanPref(kTabSearchPinnedToTabstripMigrationComplete,
                                true);
  registry->RegisterBooleanPref(kTabSearchPinnedToTabstripMigrationComplete2,
                                true);

  // Deprecated 06/2026.
  registry->RegisterBooleanPref(kPersonalContextInAutofillNoticeShouldBeShown,
                                true);

  // Deprecated 06/2026.
  registry->RegisterInt64Pref(kDefaultBrowserInfobarLastDeclined, 0);


#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  // Deprecated 07/2026.
  registry->RegisterBooleanPref(kProxyOverrideRulesAffiliation, true);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  // Deprecated 07/2026.
  registry->RegisterBooleanPref(kMV2DeprecationWarningAcknowledgedGlobally,
                                false);
  registry->RegisterBooleanPref(kMV2DeprecationDisabledAcknowledgedGlobally,
                                false);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

  // Deprecated 07/2026.
  registry->RegisterTimePref(kObsoleteManagementProfileLastLogTime,
                             base::Time());

  // Deprecated 07/2026.
  registry->RegisterBooleanPref(prefs::kProjectsPanelEntrypointEnabled, true);
  registry->RegisterBooleanPref(prefs::kProjectsPanelPinnedToTabstrip, true);

  // Deprecated 08/2026.
  registry->RegisterBooleanPref(kInitialSendAnimationShown, false);
  registry->RegisterDictionaryPref(kPrivacySandboxNotices);
  registry->RegisterTimePref(kPrivacySandboxTopicsDataAccessibleSince,
                             base::Time());
  registry->RegisterListPref(kPrivacySandboxBlockedTopics);
  registry->RegisterDictionaryPref(kPrivacySandboxFledgeJoinBlocked);
  registry->RegisterBooleanPref(kPrivacySandboxM1ConsentDecisionMade, false);
  registry->RegisterBooleanPref(kPrivacySandboxM1EEANoticeAcknowledged, false);
  registry->RegisterBooleanPref(kPrivacySandboxM1RowNoticeAcknowledged, false);
  registry->RegisterBooleanPref(kPrivacySandboxM1RestrictedNoticeAcknowledged,
                                false);
  registry->RegisterIntegerPref(kPrivacySandboxM1PromptSuppressed, 0);
  registry->RegisterBooleanPref(kPrivacySandboxNoticeDisplayed, false);
  registry->RegisterBooleanPref(kPrivacySandboxConsentDecisionMade, false);
  registry->RegisterBooleanPref(kPrivacySandboxNoConfirmationSandboxDisabled,
                                false);
  registry->RegisterBooleanPref(kPrivacySandboxNoConfirmationSandboxRestricted,
                                false);
  registry->RegisterBooleanPref(kPrivacySandboxNoConfirmationSandboxManaged,
                                false);
  registry->RegisterBooleanPref(
      kPrivacySandboxNoConfirmationThirdPartyCookiesBlocked, false);
  registry->RegisterBooleanPref(kPrivacySandboxNoConfirmationManuallyControlled,
                                false);
  registry->RegisterBooleanPref(kPrivacySandboxDisabledInsufficientConfirmation,
                                false);
  registry->RegisterBooleanPref(kPrivacySandboxTopicsConsentGiven, false);
  registry->RegisterTimePref(kPrivacySandboxTopicsConsentLastUpdateTime,
                             base::Time());
  registry->RegisterIntegerPref(kPrivacySandboxTopicsConsentLastUpdateReason,
                                0);
  registry->RegisterStringPref(kPrivacySandboxTopicsConsentTextAtLastUpdate,
                               "");
  registry->RegisterBooleanPref(kPrivacySandboxAllowNoticeFor3PCBlockedTrial,
                                false);


  // Deprecated 08/2026.
  registry->RegisterBooleanPref(kObsoleteAutofillWalletImportEnabled, true);
  registry->RegisterBooleanPref(kObsoleteAutofillWalletImportEnabledMigrated,
                                false);

  // Deprecated 08/2026.
  registry->RegisterBooleanPref(kShowRollbackUiModeB, false);
  registry->RegisterBooleanPref(kBlockAll3pcToggleEnabled, false);
  registry->RegisterBooleanPref(kTrackingProtection3pcdEnabled, false);

  // Deprecated 08/2026.
  registry->RegisterStringPref(kUkmLoggingUserSecret, std::string());
  registry->RegisterTimePref(kUkmLoggingUserSecretCreationTime, base::Time());
  registry->RegisterDictionaryPref(kExternalAppRedirectTimestamps);

  // Deprecated 08/2026.
  registry->RegisterBooleanPref(
      kEverythingMenuPinnedToTabstripMigrationComplete, false);

  // Deprecated 08/2026.
  registry->RegisterDictionaryPref(kObsoleteNtpPromoBlocklist);

  // Deprecated 08/2026.
  registry->RegisterStringPref(kSigninInterceptionIDPCookiesUrl, std::string());


  // Deprecated 09/2026.
  registry->RegisterDictionaryPref(
      kInvalidationPerSenderRegisteredForInvalidation);
  registry->RegisterDictionaryPref(
      kInvalidationPerSenderActiveRegistrationTokens);
  registry->RegisterListPref(kBookmarkEditorExpandedNodes);


  // Deprecated 09/2026.
#if BUILDFLAG(ENABLE_WEBUI_NTP)
  registry->RegisterTimePref(kNtpDriveLastDismissedTime, base::Time());
  registry->RegisterTimePref(kNtpGoogleCalendarLastDismissedTime, base::Time());
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)
  registry->RegisterTimePref(kNtpMicrosoftAuthLastDismissedTime, base::Time());
  registry->RegisterTimePref(kNtpTabGroupsLastDismissedTime, base::Time());
  // Deprecated 09/2026.
  // TODO(b/566104798): Clean up deleted pref code after 1 year.
  registry->RegisterListPref(kAudioApiStableDeviceIds);

  // Deprecated 09/2026.
  registry->RegisterIntegerPref(
      kAddressSignInPromoShownCountPerProfileForLimitsExperiment, 0);
  registry->RegisterIntegerPref(
      kBookmarkSignInPromoShownCountPerProfileForLimitsExperiment, 0);
  registry->RegisterIntegerPref(
      kPasswordSignInPromoShownCountPerProfileForLimitsExperiment, 0);
  registry->RegisterIntegerPref(
      kAddressSignInPromoDismissCountPerProfileForLimitsExperiment, 0);
  registry->RegisterIntegerPref(
      kBookmarkSignInPromoDismissCountPerProfileForLimitsExperiment, 0);
  registry->RegisterIntegerPref(
      kPasswordSignInPromoDismissCountPerProfileForLimitsExperiment, 0);


  // Deprecated 10/2026.
  registry->RegisterBooleanPref(kDismissedHistoryEmbeddingsScopePromo, false);
  registry->RegisterBooleanPref(kDismissedHistoryEmbeddingsSettingsPromo,
                                false);
  registry->RegisterIntegerPref(kShownCountHistoryEmbeddingsScopePromo, 0);
  registry->RegisterIntegerPref(kShownCountHistoryEmbeddingsSettingsPromo, 0);

  // Deprecated 10/2026.
  registry->RegisterBooleanPref(kSupervisedUserSafeSites, false);
  registry->RegisterDictionaryPref(kSupervisedUserManualHosts);
  registry->RegisterDictionaryPref(kSupervisedUserManualUrls);

  // Deprecated 10/2026.
  registry->RegisterBooleanPref(kTabSearchUsed, false);

  // Deprecated 10/2026.
  registry->RegisterStringPref(kSyncEncryptionBootstrapToken, std::string());
  registry->RegisterBooleanPref(
      kSyncEncryptionBootstrapTokenPerAccountMigrationDone, false);

  // Deprecated 10/2026.
  registry->RegisterTimePref(kTailoredSecuritySyncFlowLastRunTime,
                             base::Time());
  registry->RegisterIntegerPref(
      kTailoredSecuritySyncFlowLastUserInteractionState, 0);
  registry->RegisterTimePref(
      kTailoredSecuritySyncFlowObservedOutcomeUnsetTimestamp, base::Time());
  registry->RegisterBooleanPref(kSyncingThemePrefsMigratedToNonSyncing, false);
  registry->RegisterBooleanPref(kShouldReadIncomingSyncingThemePrefs, true);
  registry->RegisterIntegerPref(kDeprecatedBrowserColorSchemeDoNotUse, 0);
  registry->RegisterIntegerPref(kDeprecatedUserColorDoNotUse, 0);
  registry->RegisterIntegerPref(kDeprecatedBrowserColorVariantDoNotUse, 0);
  registry->RegisterBooleanPref(kDeprecatedGrayscaleThemeEnabledDoNotUse,
                                false);
  registry->RegisterDictionaryPref(kDeprecatedNtpCustomBackgroundDictDoNotUse);
  // Deprecated 10/2026.
  registry->RegisterBooleanPref(kTabsFromOtherDevicesAutoPinnedMigration,
                                false);

  // Deprecated 10/2026.
  registry->RegisterIntegerPref(kHistorySearchSettingState, 0);


  // Deprecated 11/2026.
  registry->RegisterBooleanPref(
      kBundledSettingsCheckedMigrateUserToEnhancedBundle, false);

  // Deprecated 11/2026.
  registry->RegisterStringPref(kGlicPromotionSourceCohort, std::string());
}

}  // namespace

std::string GetCountry() {
  if (!g_browser_process || !g_browser_process->variations_service()) {
    // This should only happen in tests. Ideally this would be guarded by
    // CHECK_IS_TEST, but that is not set on Android, so no specific guard.
    return std::string();
  }
  return std::string(
      g_browser_process->variations_service()->GetStoredPermanentCountry());
}

void RegisterLocalState(PrefRegistrySimple* registry) {
  // Call outs to individual subsystems that register Local State (browser-wide)
  // prefs en masse. See RegisterProfilePrefs for per-profile prefs. Please
  // keep this list alphabetized.
  autofill::prefs::RegisterLocalStatePrefs(registry);
  breadcrumbs::RegisterPrefs(registry);
  browser_shutdown::RegisterPrefs(registry);
  BrowserProcessImpl::RegisterPrefs(registry);
  ChromeContentBrowserClient::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(CHROME_FOR_TESTING)
  chrome_for_testing::RegisterPrefs(registry);
#endif
  chrome_labs_prefs::RegisterLocalStatePrefs(registry);
  chrome_urls::RegisterPrefs(registry);
  ChromeMetricsServiceClient::RegisterPrefs(registry);
  enterprise_connectors::RegisterLocalStatePrefs(registry);
  enterprise_groups::RegisterLocalStatePrefs(registry);
  enterprise_util::RegisterLocalStatePrefs(registry);
  component_updater::RegisterPrefs(registry);
  domain_reliability::RegisterPrefs(registry);
  embedder_support::OriginTrialPrefs::RegisterPrefs(registry);
  enterprise_reporting::RegisterLocalStatePrefs(registry);
  ExternalProtocolHandler::RegisterPrefs(registry);
  flags_ui::PrefServiceFlagsStorage::RegisterPrefs(registry);
  GpuModeManager::RegisterPrefs(registry);
  signin::IdentityManager::RegisterLocalStatePrefs(registry);
  IntranetRedirectDetector::RegisterPrefs(registry);
  language::GeoLanguageProvider::RegisterLocalStatePrefs(registry);
  language::UlpLanguageCodeLocator::RegisterLocalStatePrefs(registry);
  memory::EnterpriseMemoryLimitPrefObserver::RegisterPrefs(registry);
  metrics::RegisterDemographicsLocalStatePrefs(registry);
  metrics::TabStatsTracker::RegisterPrefs(registry);
  network_time::NetworkTimeTracker::RegisterPrefs(registry);
  omnibox::RegisterLocalStatePrefs(registry);
  omnibox_everywhere::prefs::RegisterLocalStatePrefs(registry);
  optimization_guide::prefs::RegisterLocalStatePrefs(registry);
  optimization_guide::model_execution::prefs::RegisterLocalStatePrefs(registry);
  optimization_guide::RegisterModelQualityLocalStatePrefs(registry);
  password_manager::PasswordManager::RegisterLocalPrefs(registry);
  policy::BrowserPolicyConnector::RegisterPrefs(registry);
  policy::LocalTestPolicyProvider::RegisterLocalStatePrefs(registry);
  policy::ManagementService::RegisterLocalStatePrefs(registry);
  policy::PolicyStatisticsCollector::RegisterPrefs(registry);
  PrefProxyConfigTrackerImpl::RegisterPrefs(registry);
  ProfileAttributesEntry::RegisterLocalStatePrefs(registry);
  ProfileAttributesStorage::RegisterPrefs(registry);
  ProfileNetworkContextService::RegisterLocalStatePrefs(registry);
  profiles::RegisterPrefs(registry);
  feature_engagement::RegisterLocalStatePrefs(registry);
  RegisterScreenshotPrefs(registry);
  safe_browsing::RegisterLocalStatePrefs(registry);
  search_engines::SearchEngineChoiceService::RegisterLocalStatePrefs(registry);
  secure_origin_allowlist::RegisterPrefs(registry);
  segmentation_platform::SegmentationPlatformService::RegisterLocalStatePrefs(
      registry);
  SerialPolicyAllowedPorts::RegisterPrefs(registry);
  HidPolicyAllowedDevices::RegisterLocalStatePrefs(registry);
  sessions::SessionIdGenerator::RegisterPrefs(registry);
  signin::ActivePrimaryAccountsMetricsRecorder::RegisterLocalStatePrefs(
      registry);
  SSLConfigServiceManager::RegisterPrefs(registry);
  subresource_filter::IndexedRulesetVersion::RegisterPrefs(
      registry, subresource_filter::kSafeBrowsingRulesetConfig.filter_tag);
  SystemNetworkContextManager::RegisterPrefs(registry);
  tracing::RegisterPrefs(registry);
  update_client::RegisterPrefs(registry);
  variations::VariationsService::RegisterPrefs(registry);

  // Individual preferences. If you have multiple preferences that should
  // clearly be grouped together, please group them together into a helper
  // function called above. Please keep this list alphabetized.

  registry->RegisterTimePref(prefs::kAudioInputStreamLastTimeCreated,
                             base::Time(), PrefRegistry::LOSSY_PREF);
  registry->RegisterBooleanPref(
      policy::policy_prefs::kIntensiveWakeUpThrottlingEnabled, false);
  registry->RegisterBooleanPref(prefs::kPromotionsEnabled, true);
#if BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  registry->RegisterBooleanPref(prefs::kFeatureNotificationsEnabled, true);
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

  // Below this point is for platform-specific and compile-time conditional
  // calls. Please follow the helper-function-first-then-direct-calls pattern
  // established above, and keep things alphabetized.

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  BackgroundModeManager::RegisterPrefs(registry);
#endif

  child_module::ChildModuleManager::RegisterLocalStatePrefs(registry);
  gcm::RegisterPrefs(registry);
  headless::RegisterPrefs(registry);
  media_router::RegisterLocalStatePrefs(registry);
  performance_manager::user_tuning::prefs::RegisterLocalStatePrefs(registry);
  PerformanceInterventionMetricsReporter::RegisterLocalStatePrefs(registry);
  RegisterBrowserPrefs(registry);
  speech::SodaInstaller::RegisterLocalStatePrefs(registry);
  speech::SpeechRecognitionSmallExpertModelInstaller::RegisterLocalStatePrefs(
      registry);
  StartupBrowserCreator::RegisterLocalStatePrefs(registry);
  task_manager::TaskManagerInterface::RegisterPrefs(registry);
  UpgradeDetector::RegisterPrefs(registry);
  registry->RegisterIntegerPref(prefs::kLastWhatsNewVersion, 0);

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  extensions::ExtensionPrefs::RegisterLocalStatePrefs(registry);
  extensions::component_loader_prefs::RegisterPrefs(registry);
#endif

#if BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
  on_device_translation::RegisterLocalStatePrefs(registry);
#endif  // BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  scheduled_restart::ScheduledRestartManager::RegisterLocalStatePrefs(registry);
  WhatsNewUI::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  FirstRunService::RegisterLocalStatePrefs(registry);
#endif


#if BUILDFLAG(IS_MAC)
  confirm_quit::RegisterLocalState(registry);
  QuitWithAppsController::RegisterPrefs(registry);
  system_media_permissions::RegisterSystemMediaPermissionStatesPrefs(registry);
  AppShimRegistry::Get()->RegisterLocalPrefs(registry);
  GlassFrameService::RegisterLocalStatePrefs(registry);

  registry->RegisterBooleanPref(prefs::kUpdateOnZeroWindowEnabled, true);
#endif

#if BUILDFLAG(IS_WIN)
  os_crypt_async::RegisterLocalPrefs(registry);
  registry->RegisterBooleanPref(prefs::kRendererAppContainerEnabled, true);
  registry->RegisterBooleanPref(prefs::kBlockBrowserLegacyExtensionPoints,
                                true);
  registry->RegisterIntegerPref(prefs::kDynamicCodeSettings, /*Default=*/0);
  registry->RegisterBooleanPref(prefs::kApplicationBoundEncryptionEnabled,
                                true);
  registry->RegisterBooleanPref(prefs::kPrintingLPACSandboxEnabled, true);
  registry->RegisterBooleanPref(
      policy::policy_prefs::kNativeWindowOcclusionEnabled, true);
  registry->RegisterBooleanPref(prefs::kRestrictCoreSharingOnRenderer, false);
  MediaFoundationServiceMonitor::RegisterPrefs(registry);
  os_crypt_async::AppBoundEncryptionProviderWin::RegisterLocalPrefs(registry);
  webnn::RegisterLocalPrefs(registry);
  registry->RegisterStringPref(prefs::kPreviousIsolationState, std::string());
  registry->RegisterBooleanPref(prefs::kForegroundLaunchOnLogin, false);
  registry->RegisterBooleanPref(prefs::kStartupLaunchInfobarAccepted, false);
  registry->RegisterTimePref(prefs::kStartupLaunchInfobarLastDeclinedTime,
                             base::Time());
  registry->RegisterIntegerPref(prefs::kStartupLaunchInfobarDeclinedCount, 0);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_DOWNGRADE_PROCESSING)
  downgrade::RegisterPrefs(registry);
#endif

  RegisterDefaultBrowserPromptPrefs(registry);
  DeviceOAuth2TokenStoreDesktop::RegisterPrefs(registry);

  screen_ai::RegisterLocalStatePrefs(registry);

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  PlatformAuthPolicyObserver::RegisterPrefs(registry);
#endif

  // Platform-specific and compile-time conditional individual preferences.
  // If you have multiple preferences that should clearly be grouped together,
  // please group them together into a helper function called above. Please
  // keep this list alphabetized.

#if BUILDFLAG(ENABLE_OOP_PRINTING)
  registry->RegisterBooleanPref(prefs::kOopPrintDriversAllowedByPolicy, true);
#endif

#if BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  // TODO(b/328668317): Default pref should be set to true once this is
  // launched.
  registry->RegisterBooleanPref(prefs::kOsUpdateHandlerEnabled, false);
  platform_experience::prefs::RegisterPrefs(*registry);
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

#if BUILDFLAG(ENABLE_PDF)
  registry->RegisterBooleanPref(prefs::kPdfViewerOutOfProcessIframeEnabled,
                                true);
#endif  // BUILDFLAG(ENABLE_PDF)

#if BUILDFLAG(ENABLE_PDF_SAVE_TO_DRIVE)
  registry->RegisterStringPref(
      prefs::kRestrictPdfSaveToGoogleDriveAccountsToPattern, "");
#endif  // BUILDFLAG(ENABLE_PDF_SAVE_TO_DRIVE)

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kChromeForTestingAllowed, true);
#endif

  registry->RegisterBooleanPref(prefs::kQRCodeGeneratorEnabled, true);

  registry->RegisterIntegerPref(prefs::kChromeDataRegionSetting, 0);

  glic::prefs::RegisterLocalStatePrefs(registry);

  registry->RegisterIntegerPref(prefs::kToastAlertLevel, 0);

  registry->RegisterStringPref(prefs::kNonMilestoneUpdateToastVersion, "");
  registry->RegisterBooleanPref(prefs::kSilentPrintingEnabled, false);

  registry->RegisterListPref(
      prefs::kManagedLocalNetworkAccessIpAddressSpaceOverrides);
  registry->RegisterBooleanPref(
      policy::policy_prefs::kLocalNetworkAccessPermissionsPolicyDefaultEnabled,
      false);

  // This is intentionally last.
  RegisterLocalStatePrefsForMigration(registry);
}

// Register prefs applicable to all profiles.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                          const std::string& locale) {
  TRACE_EVENT0("browser", "chrome::RegisterProfilePrefs");
  // User prefs. Please keep this list alphabetized.
  AccessibilityLabelsService::RegisterProfilePrefs(registry);
  registry->RegisterBooleanPref(prefs::kRendererAccessibilityEnabled, true);
  AccessibilityUIMessageHandler::RegisterProfilePrefs(registry);
  ttc::AiOverlayDialogController::RegisterProfilePrefs(registry);
  AimEligibilityService::RegisterProfilePrefs(registry);
  AnnouncementNotificationService::RegisterProfilePrefs(registry);
  autofill::prefs::RegisterProfilePrefs(registry);
  browsing_data::prefs::RegisterBrowserUserPrefs(registry);
  capture_policy::RegisterProfilePrefs(registry);
  certificate_transparency::prefs::RegisterPrefs(registry);
  ChromeContentBrowserClient::RegisterProfilePrefs(registry);
  chrome_labs_prefs::RegisterProfilePrefs(registry);
  ChromeLocationBarModelDelegate::RegisterProfilePrefs(registry);
  content_settings::CookieSettings::RegisterProfilePrefs(registry);
  ChromeVersionService::RegisterProfilePrefs(registry);
  chrome_browser_net::NetErrorTabHelper::RegisterProfilePrefs(registry);
  chrome_prefs::RegisterProfilePrefs(registry);
  collaboration::prefs::RegisterProfilePrefs(registry);
  commerce::RegisterProfilePrefs(registry);
  context_hub::prefs::RegisterProfilePrefs(registry);
  contextual_cueing::prefs::RegisterProfilePrefs(registry);
  contextual_search::ContextualSearchService::RegisterProfilePrefs(registry);
  contextual_tasks::RegisterProfilePrefs(registry);
  registry->RegisterIntegerPref(prefs::kContextualTasksNextPanelOpenCount, 0);
  registry->RegisterBooleanPref(prefs::kCtrlTabMru, false);
  cross_device::RegisterProfilePrefs(registry);
  device_signals::RegisterProfilePrefs(registry);
  enterprise::RegisterIdentifiersProfilePrefs(registry);
  enterprise_connectors::RegisterProfilePrefs(registry);
  enterprise_isolated_mode::RegisterProfilePrefs(registry);
#if BUILDFLAG(ENTERPRISE_PROXY)
  enterprise_net::RegisterProfilePrefs(registry);
#endif
  enterprise_promotion::RegisterProfilePrefs(registry);
  enterprise_reporting::RegisterProfilePrefs(registry);
  dom_distiller::DistilledPagePrefs::RegisterProfilePrefs(registry);
  DownloadPrefs::RegisterProfilePrefs(registry);
  geic::prefs::RegisterProfilePrefs(registry);
  glic::prefs::RegisterProfilePrefs(registry);
  glic::contextual_cueing::prefs::RegisterProfilePrefs(registry);
  permissions::PermissionHatsTriggerHelper::RegisterProfilePrefs(registry);
  history_clusters::prefs::RegisterProfilePrefs(registry);
  HostContentSettingsMap::RegisterProfilePrefs(registry);
  image_fetcher::ImageCache::RegisterProfilePrefs(registry);
  site_engagement::ImportantSitesUtil::RegisterProfilePrefs(registry);
  IncognitoModePrefs::RegisterProfilePrefs(registry);
  language::LanguagePrefs::RegisterProfilePrefs(registry);
  login_detection::prefs::RegisterProfilePrefs(registry);
  lookalikes::RegisterProfilePrefs(registry);
  media_prefs::RegisterUserPrefs(registry);
  MediaCaptureDevicesDispatcher::RegisterProfilePrefs(registry);
  media_device_salt::MediaDeviceIDSalt::RegisterProfilePrefs(registry);
  MediaEngagementService::RegisterProfilePrefs(registry);
  MediaStorageIdSalt::RegisterProfilePrefs(registry);
  metrics::RegisterDemographicsProfilePrefs(registry);
  metrics::MetricsReportingChoiceService::RegisterProfilePrefs(registry);
  NotificationDisplayServiceImpl::RegisterProfilePrefs(registry);
  NotifierStateTracker::RegisterProfilePrefs(registry);
  ntp_tiles::CustomLinksManagerImpl::RegisterProfilePrefs(registry);
  ntp_tiles::MostVisitedSites::RegisterProfilePrefs(registry);
#if BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
  on_device_translation::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
  optimization_guide::prefs::RegisterProfilePrefs(registry);
  optimization_guide::RegisterGenAiFeatures(registry);
  PageColorsController::RegisterProfilePrefs(registry);
  password_manager::PasswordManager::RegisterProfilePrefs(registry);
  payments::RegisterProfilePrefs(registry);
  performance_manager::user_tuning::prefs::RegisterProfilePrefs(registry);
  permissions::RegisterProfilePrefs(registry);
  PermissionBubbleMediaAccessHandler::RegisterProfilePrefs(registry);
  PlatformNotificationServiceImpl::RegisterProfilePrefs(registry);
  policy::URLBlocklistManager::RegisterProfilePrefs(registry);
  PolicyUI::RegisterProfilePrefs(registry);
  PrefProxyConfigTrackerImpl::RegisterProfilePrefs(registry);
  prefetch::RegisterPredictionOptionsProfilePrefs(registry);
  PrefetchOriginDecider::RegisterPrefs(registry);
  PrefsTabHelper::RegisterProfilePrefs(registry, locale);
  personal_context::prefs::RegisterProfilePrefs(registry);
  privacy_sandbox::RegisterProfilePrefs(registry);
  Profile::RegisterProfilePrefs(registry);
  ProfileImpl::RegisterProfilePrefs(registry);
  ProfileNetworkContextService::RegisterProfilePrefs(registry);
  custom_handlers::ProtocolHandlerRegistry::RegisterProfilePrefs(registry);
  PushMessagingAppIdentifier::RegisterProfilePrefs(registry);
  PushMessagingUnsubscribedEntry::RegisterProfilePrefs(registry);
  QuietNotificationPermissionUiState::RegisterProfilePrefs(registry);
  regional_capabilities::prefs::RegisterProfilePrefs(registry);
  RegisterBrowserUserPrefs(registry);
  registry->RegisterBooleanPref(prefs::kRestrictYouTubeCookiesDeletion, false);
  RegisterGeminiSettingsPrefs(registry);
  registry->RegisterIntegerPref(prefs::kVoiceTypingSettings, 0);
  registry->RegisterBooleanPref(prefs::kPrefDictationOnboardingCompleted,
                                false);
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
  registry->RegisterStringPref(prefs::kVoiceTypingHotkey, "Ctrl+Space");
#else
  registry->RegisterStringPref(prefs::kVoiceTypingHotkey, "Alt+Space");
#endif

  indigo::prefs::RegisterProfilePrefs(registry);
  RegisterPrefersDefaultScrollbarStylesPrefs(registry);
  RegisterSafetyHubProfilePrefs(registry);
#if BUILDFLAG(SAFE_BROWSING_AVAILABLE)
  safe_browsing::file_type::RegisterProfilePrefs(registry);
#endif
  safe_browsing::RegisterProfilePrefs(registry);
  safety_check::prefs::RegisterProfilePrefs(registry);
  SearchPrefetchService::RegisterProfilePrefs(registry);
  blocked_content::SafeBrowsingTriggeredPopupBlocker::RegisterProfilePrefs(
      registry);
  security_interstitials::InsecureFormBlockingPage::RegisterProfilePrefs(
      registry);
  segmentation_platform::SegmentationPlatformService::RegisterProfilePrefs(
      registry);
  segmentation_platform::DeviceSwitcherResultDispatcher::RegisterProfilePrefs(
      registry);
  SearchPreloadService::RegisterProfilePrefs(registry);
  SessionStartupPref::RegisterProfilePrefs(registry);
  SharingSyncPreference::RegisterProfilePrefs(registry);
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  signin::AvatarButtonPromoManager::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
  SigninPrefs::RegisterProfilePrefs(registry);
  site_engagement::SiteEngagementService::RegisterProfilePrefs(registry);
  site_token_provider::prefs::RegisterProfilePrefs(registry);
  skills::prefs::RegisterProfilePrefs(registry);
  subscription_eligibility::prefs::RegisterProfilePrefs(registry);
  supervised_user::RegisterProfilePrefs(registry);
  sync_sessions::SessionSyncPrefs::RegisterProfilePrefs(registry);
  syncer::DeviceInfoPrefs::RegisterProfilePrefs(registry);
  syncer::DeviceStatisticsScheduler::RegisterProfilePrefs(registry);
  syncer::SyncPrefs::RegisterProfilePrefs(registry);
  syncer::SyncTransportDataPrefs::RegisterProfilePrefs(registry);
  TemplateURLPrepopulateData::RegisterProfilePrefs(registry);
  tab_groups::prefs::RegisterProfilePrefs(registry);
  translate::TranslatePrefs::RegisterProfilePrefs(registry);
  ttc::RegisterProfilePrefs(registry);
  universal_optout::prefs::RegisterProfilePrefs(registry);
  visited_url_ranking::GroupSuggestionsServiceImpl::RegisterProfilePrefs(
      registry);
  wallet::prefs::RegisterProfilePrefs(registry);
  omnibox::RegisterProfilePrefs(registry);
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  omnibox_everywhere::prefs::RegisterProfilePrefs(registry);
#endif
  ZeroSuggestProvider::RegisterProfilePrefs(registry);
  NtpCustomBackgroundService::RegisterProfilePrefs(registry);

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  RegisterSessionServiceLogProfilePrefs(registry);
  SessionDataService::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  extensions::ActivityLog::RegisterProfilePrefs(registry);
  extensions::PermissionsManager::RegisterProfilePrefs(registry);
  extensions::ExtensionPrefs::RegisterProfilePrefs(registry);
  extensions::LowTrustPolicyInstallBlockManager::RegisterProfilePrefs(registry);
  extensions::RuntimeAPI::RegisterPrefs(registry);
  extensions::CommandService::RegisterProfilePrefs(registry);
  extensions::util::RegisterProfilePrefs(registry);
  extensions_ui_prefs::RegisterProfilePrefs(registry);
  ExtensionUrlOverrides::RegisterProfilePrefs(registry);
  update_client::RegisterProfilePrefs(registry);

  // TODO(devlin): This would be more inline with the other calls here if it
  // were nested in either a class or separate namespace with a simple
  // Register[Profile]Prefs() name.
  extensions::RegisterSettingsOverriddenUiPrefs(registry);
  ExtensionSettingsOverriddenDialog::RegisterProfilePrefs(registry);

#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

#if BUILDFLAG(ENABLE_EXTENSIONS)
  RegisterAnimationPolicyPrefs(registry);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_PDF)
  registry->RegisterListPref(prefs::kPdfLocalFileAccessAllowedForDomains,
                             base::ListValue());
  registry->RegisterBooleanPref(prefs::kPdfUseSkiaRendererEnabled, true);
#endif  // BUILDFLAG(ENABLE_PDF)

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  printing::PolicySettings::RegisterProfilePrefs(registry);
  printing::PrintPreviewStickySettings::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_RLZ)
  ChromeRLZTrackerDelegate::RegisterProfilePrefs(registry);
#endif

  UnifiedAutoplayConfig::RegisterProfilePrefs(registry);
#if BUILDFLAG(ENABLE_WEBUI_NTP)
// TODO(b/502297163): Implement for Android.
  NewTabFooterUI::RegisterProfilePrefs(registry);
  NewTabPageHandler::RegisterProfilePrefs(registry);
  ThemeColorPickerHandler::RegisterProfilePrefs(registry);
  NewTabPageUI::RegisterProfilePrefs(registry);
  MostVisitedPrefObserver::RegisterProfilePrefs(registry);
  MostRelevantTabResumptionPageHandler::RegisterProfilePrefs(registry);
#else
  // Registered here because it is still accessed on Android (which doesn't use
  // WebUI NTP).
  registry->RegisterBooleanPref(ntp_prefs::kNtpShortcutsVisible, true);
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)

  bookmarks_webui::RegisterProfilePrefs(registry);
  browser_sync::ForeignSessionHandler::RegisterProfilePrefs(registry);
  BrowserUserEducationStorageService::RegisterProfilePrefs(registry);
  captions::LiveCaptionController::RegisterProfilePrefs(registry);
  captions::LiveTranslateController::RegisterProfilePrefs(registry);
  ChromeAuthenticatorRequestDelegate::RegisterProfilePrefs(registry);
  commerce::CommerceUiTabHelper::RegisterProfilePrefs(registry);
  first_run::RegisterProfilePrefs(registry);
  gcm::RegisterProfilePrefs(registry);
  HatsServiceDesktop::RegisterProfilePrefs(registry);
  media_router::RegisterAccessCodeProfilePrefs(registry);
  media_router::RegisterProfilePrefs(registry);
  MicrosoftFilesPageHandler::RegisterProfilePrefs(registry);
  OutlookCalendarPageHandler::RegisterProfilePrefs(registry);
  PinnedTabCodec::RegisterProfilePrefs(registry);
  promos_utils::RegisterProfilePrefs(registry);
  RegisterReadAnythingProfilePrefs(registry);
  settings::SettingsUI::RegisterProfilePrefs(registry);
  signin::RegisterProfilePrefs(registry);
  StartupBrowserCreator::RegisterProfilePrefs(registry);
  tab_groups::saved_tab_groups::prefs::RegisterProfilePrefs(registry);
  tab_search_prefs::RegisterProfilePrefs(registry);
  toolbar::RegisterProfilePrefs(registry);

#if BUILDFLAG(ENABLE_LENS_OVERLAY_BACKEND)
  lens::prefs::RegisterProfilePrefs(registry);
#endif

  ThemeService::RegisterProfilePrefs(registry);
  ManagementUI::RegisterProfilePrefs(registry);

#if BUILDFLAG(ENABLE_DEVTOOLS_FRONTEND)
  DevToolsWindow::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENABLE_DEVTOOLS_FRONTEND)



#if BUILDFLAG(IS_WIN)
  CdmPrefServiceHelper::RegisterProfilePrefs(registry);
  FontPrewarmerTabHelper::RegisterProfilePrefs(registry);
  NetworkProfileBubble::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  ntp_tiles::EnterpriseShortcutsManagerImpl::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  browser_switcher::BrowserSwitcherPrefs::RegisterProfilePrefs(registry);
  enterprise_signin::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  preinstalled_extensions::RegisterProfilePrefs(registry);
#endif

  sharing_hub::RegisterProfilePrefs(registry);

#if defined(TOOLKIT_VIEWS)
  accessibility_prefs::RegisterInvertBubbleUserPrefs(registry);
  side_search_prefs::RegisterProfilePrefs(registry);
  RegisterBrowserViewProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_LENS_DESKTOP)
  registry->RegisterBooleanPref(
      prefs::kLensRegionSearchEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#endif
#if BUILDFLAG(ENABLE_LENS_DESKTOP) || BUILDFLAG(ENABLE_WEBUI_NTP)
  registry->RegisterBooleanPref(prefs::kLensDesktopNTPSearchEnabled, true);
#endif

  registry->RegisterBooleanPref(prefs::kDisableScreenshots, false);
  registry->RegisterDictionaryPref(prefs::kContextMenuAnimationState);
  registry->RegisterListPref(
      webauthn::pref_names::kRemoteDesktopAllowedOrigins);

  enterprise_custom_headers::RegisterProfilePrefs(registry);

  side_panel_prefs::RegisterProfilePrefs(registry);

  registry->RegisterBooleanPref(
      webauthn::pref_names::kRemoteProxiedRequestsAllowed, false);

  registry->RegisterIntegerPref(
      webauthn::pref_names::kEnclaveFailedPINAttemptsCount, 0);

  tabs::RegisterProfilePrefs(registry);

  CertificateManagerPageHandler::RegisterProfilePrefs(registry);

  actor::ui::RegisterProfilePrefs(registry);

  organizer::RegisterProfilePrefs(registry);

  registry->RegisterBooleanPref(webauthn::pref_names::kAllowWithBrokenCerts,
                                false);

  registry->RegisterBooleanPref(prefs::kPrivacyGuideViewed, false);

  RegisterProfilePrefsForMigration(registry);

  registry->RegisterIntegerPref(prefs::kMemorySaverChipExpandedCount, 0);
  registry->RegisterTimePref(prefs::kLastMemorySaverChipExpandedTimestamp,
                             base::Time());
  registry->RegisterBooleanPref(
      prefs::kAccessibilityMainNodeAnnotationsEnabled, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterBooleanPref(
      prefs::kManagedLocalNetworkAccessRestrictionsTemporaryOptOut, false);


#if BUILDFLAG(ENTERPRISE_DATA_CONTROLS)
  data_controls::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENTERPRISE_DATA_CONTROLS)

#if BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kNativeHostsExecutablesLaunchDirectly,
                                false);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_COMPOSE)
  registry->RegisterBooleanPref(prefs::kPrefHasCompletedComposeFRE, false);
  registry->RegisterBooleanPref(prefs::kEnableProactiveNudge, true);
  registry->RegisterDictionaryPref(prefs::kProactiveNudgeDisabledSitesWithTime);
#endif

  registry->RegisterIntegerPref(prefs::kChromeDataRegionSetting, 0);

  registry->RegisterIntegerPref(prefs::kLensOverlayStartCount, 0);

  registry->RegisterBooleanPref(prefs::kViewSourceLineWrappingEnabled, false);

  // TODO(crbug.com/442891187): Move these to appropriate manager files when
  // the policies logic is implemented.
  registry->RegisterListPref(policy::policy_prefs::kIncognitoModeUrlBlocklist);
  registry->RegisterListPref(policy::policy_prefs::kIncognitoModeUrlAllowlist);

  registry->RegisterBooleanPref(
      ntp_tiles::prefs::kTabResumptionHomeModuleEnabled, true);

  registry->RegisterBooleanPref(ntp_tiles::prefs::kMagicStackHomeModuleEnabled,
                                true);

  registry->RegisterBooleanPref(ntp_tiles::prefs::kTipsHomeModuleEnabled, true);


  registry->RegisterBooleanPref(prefs::kStaticStorageQuotaEnabled, false);
}

void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterUserProfilePrefs(registry, g_browser_process->GetApplicationLocale());
}

void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                              const std::string& locale) {
  RegisterProfilePrefs(registry, locale);

}

void RegisterScreenshotPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kDisableScreenshots, false);
}

void RegisterGeminiSettingsPrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(optimization_guide::prefs::kGeminiSettings, 0);
}


// This method should be periodically pruned of year+ old migrations.
// See chrome/browser/prefs/README.md for details.
void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.

  // BEGIN_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
  // Please don't delete the preceding line. It is used by PRESUBMIT.py.

  // Added 10/2025
  local_state->ClearPref(prefs::kDefaultBrowserFirstShownTime);

  // Added 11/2025
  local_state->ClearPref(kFpfRulesetContent);
  local_state->ClearPref(kFpfRulesetFormat);
  local_state->ClearPref(kFpfRulesetChecksum);

  // Added 12/2025
  local_state->ClearPref(kPrivacyBudgetGeneration);
  local_state->ClearPref(kPrivacyBudgetSeenSurfaces);
  local_state->ClearPref(kPrivacyBudgetSelectedOffsets);
  local_state->ClearPref(kPrivacyBudgetSelectedBlock);
  local_state->ClearPref(kPrivacyBudgetMetaExperimentActivationSalt);

  // Added 12/2025
  local_state->ClearPref(kAutofillStatesDataDir);

  // Added 12/2025
  local_state->ClearPref(kFingerprintingProtectionEnabled);
  local_state->ClearPref(kIpProtectionEnabled);
  local_state->ClearPref(kAllowAll3pcToggleEnabled);
  local_state->ClearPref(kUserBypass3pcExceptionsMigrated);
  local_state->ClearPref(kTrackingProtectionLevel);
  local_state->ClearPref(kTrackingProtectionSilentOnboardedSince);
  local_state->ClearPref(kTrackingProtectionSilentEligibleSince);
  local_state->ClearPref(kTrackingProtectionEligibleSince);
  local_state->ClearPref(kTrackingProtectionOnboardedSince);
  local_state->ClearPref(kTrackingProtectionNoticeLastShown);
  local_state->ClearPref(kTrackingProtectionOnboardingAckedSince);
  local_state->ClearPref(kTrackingProtectionOnboardingAcked);
  local_state->ClearPref(kTrackingProtectionOnboardingAckAction);
  local_state->ClearPref(kIpProtectionInitializedByDogfood);
  local_state->ClearPref(kTrackingProtectionSilentOnboardingStatus);
  local_state->ClearPref(kTrackingProtectionOnboardingStatus);
  local_state->ClearPref(kTPCDExperimentClientState);
  local_state->ClearPref(kTPCDExperimentClientStateVersion);
  local_state->ClearPref(kTPCDExperimentProfileState);


  // Added 02/2026.
  if (local_state->HasPrefPath(kProfilesDeletedOld)) {
    const base::ListValue& old_list = local_state->GetList(kProfilesDeletedOld);
    if (!old_list.empty()) {
      ScopedListPrefUpdate update(local_state, prefs::kProfilesDeleted);
      for (const auto& value : old_list) {
        std::optional<base::FilePath> path = base::ValueToFilePath(value);
        if (path) {
          base::FilePath basename = path->BaseName();
          // Avoid the edge case where the base name is the root, e.g `/` on
          // linux.
          if (!basename.IsAbsolute()) {
            update->Append(base::FilePathToValue(basename));
          }
        }
      }
    }
    local_state->ClearPref(kProfilesDeletedOld);
  }

  // Added 03/2026.
  local_state->ClearPref(kGlicMultiInstanceEnabledBySubscriptionTier);

  // Added 04/2026.
  local_state->ClearPref(kTpcdMetadataCohorts);

  // Added 04/2026.
  tabs::MigrateHoverCardMemoryPref(local_state);

  // Added 04/2026.
  local_state->ClearPref(kHasSeenWebFeed);
  local_state->ClearPref(kLastBadgeAnimationTime);

  // Added 05/2026
  local_state->ClearPref(kHttpCacheFinchExperimentGroups);


  // Added 07/2026.
  local_state->ClearPref(kObsoleteMetricsReportingLevel);
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  // Added 07/2026.
  local_state->ClearPref(kProxyOverrideRulesAffiliation);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  local_state->ClearPref(kMetricsReportingMigrationDone);
  local_state->ClearPref(kMetricsConsentRestructureFeatureState);

  // Added 07/2026.
  local_state->ClearPref(kObsoleteManagementPlatformLastLogTime);

  // Added 09/2026.
  local_state->ClearPref(kInvalidationPerSenderRegisteredForInvalidation);
  local_state->ClearPref(kInvalidationPerSenderActiveRegistrationTokens);



  // Please don't delete the following line. It is used by PRESUBMIT.py.
  // END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS

  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.
}

// This method should be periodically pruned of year+ old migrations.
// See chrome/browser/prefs/README.md for details.
void MigrateObsoleteProfilePrefs(PrefService* profile_prefs,
                                 const base::FilePath& profile_path) {
  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.

  // BEGIN_MIGRATE_OBSOLETE_PROFILE_PREFS
  // Please don't delete the preceding line. It is used by PRESUBMIT.py.

  // Do not remove: `SigninPrefs::MigrateObsoleteSigninPrefs()` should remain
  // active; obsolete per-account prefs are cleaned up internally within it.
  SigninPrefs(*profile_prefs).MigrateObsoleteSigninPrefs();

  // Added 10/2025.
  profile_prefs->ClearPref(kSessionRestoreTurnOffFromRestartInfoBarTimesShown);
  profile_prefs->ClearPref(kSessionRestoreTurnOffFromSessionInfoBarTimesShown);
  profile_prefs->ClearPref(kSessionRestorePrefChanged);

  // Check MigrateDeprecatedAutofillPrefs() to see if this is safe to remove.
  autofill::prefs::MigrateDeprecatedAutofillPrefs(profile_prefs);

#if BUILDFLAG(ENABLE_WEBUI_NTP)
  // Added 08/2025.
  // TODO(crbug.com/557083938): Cleanup migration.
  MostVisitedPrefObserver::MigrateDeprecatedUseMostVisitedTilesPref(
      profile_prefs);
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)


  // Added 09/2025.
  PageColorsController::MigrateObsoleteProfilePrefs(profile_prefs);
  profile_prefs->ClearPref(kGaiaCookieLastListAccountsData);

  // Added 09/2025.
  profile_prefs->ClearPref(kLensOverlayEduActionChipShownCount);

#if BUILDFLAG(ENABLE_WEBUI_NTP)
  // Added 10/2025
  MostVisitedPrefObserver::MigrateDeprecatedShortcutsTypePref(profile_prefs);
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)

  // Added 10/2025.
  profile_prefs->ClearPref(kLegacySyncSessionsGUID);

  // Added 11/2025.
  profile_prefs->ClearPref(kRefreshHeuristicBreakageException);

  // Added 12/2025.
  profile_prefs->ClearPref(kReduceUserAgentMinorVersion);
  profile_prefs->ClearPref(kMerchantTrustUiLastInteractionTime);
  profile_prefs->ClearPref(kMerchantTrustPageInfoLastOpenTime);

  // Added 12/2025.
  profile_prefs->ClearPref(kCloudPrintProxyEnabled);
  profile_prefs->ClearPref(kCloudPrintEmail);


  // Added 01/2026.
  profile_prefs->ClearPref(kCookieClearOnExitMigrationNoticeComplete);

  // Added 02/2026.
  profile_prefs->ClearPref(kGlicGuestUrlPresetAutopush);
  profile_prefs->ClearPref(kGlicGuestUrlPresetPreprod);
  profile_prefs->ClearPref(kGlicGuestUrlPresetProd);

  // Added 02/2026.
  profile_prefs->ClearPref(kExplicitBrowserSigninWithoutFeatureEnabled);

  // Added 02/2026.
  profile_prefs->ClearPref(kTabSearchOpened);

  // Added 03/2026.
  profile_prefs->ClearPref(kTabDeclutterUsageCount);

  // Added 03/2026.
  profile_prefs->ClearPref(kTabSearchTabIndex);

  // Added 03/2026
  profile_prefs->ClearPref(
      kSigninFromBookmarksBubbleSyntheticTrialGroupNamePref);
  profile_prefs->ClearPref(
      kBookmarksBubblePromoShownSyntheticTrialGroupNamePref);

  // Added 03/2026.
  profile_prefs->ClearPref(kSafeBrowsingModuleShownCount);
  profile_prefs->ClearPref(kSafeBrowsingModuleLastCooldownStartAt);
  profile_prefs->ClearPref(kSafeBrowsingModuleOpened);


  // Added 03/2026.
  profile_prefs->ClearPref(kTabOrganizationNudgeBackoffCount);
  profile_prefs->ClearPref(kTabOrganizationShowFRE);
  profile_prefs->ClearPref(kTabOrganizationModelStrategy);

  // Added 03/2026.
  profile_prefs->ClearPref(kNtpContextMenuClickCount);

  // Added 03/2026.
  profile_prefs->ClearPref(kNtpPromoPrefLastSnoozed);

  // Added 04/2026.
  profile_prefs->ClearPref(kPreallocatedAddressesVersion);
  profile_prefs->ClearPref(kPreallocatedAddresses);
  profile_prefs->ClearPref(kPreallocatedAddressesNext);
  profile_prefs->ClearPref(kFirstPlusAddressCreationTime);
  profile_prefs->ClearPref(kLastPlusAddressFillingTime);

  // Added 05/2026.
  profile_prefs->ClearPref(kWebFeedContentOrder);
  profile_prefs->ClearPref(kWebFeedsRequestSchedule);
  profile_prefs->ClearPref(kEnableWebFeedFollowIntroDebug);
  profile_prefs->ClearPref(kLastSeenFeedType);

  // Added 05/2026.
  profile_prefs->ClearPref(kShouldShowRemoteAnnotatorFirstRunInfo);

  // Deprecated 06/2026.
  profile_prefs->ClearPref(kDeleteTimePeriodBasic);
  profile_prefs->ClearPref(kDeleteBrowsingHistoryBasic);
  profile_prefs->ClearPref(kDeleteCacheBasic);
  profile_prefs->ClearPref(kDeleteCookiesBasic);
  profile_prefs->ClearPref(kLastClearBrowsingDataTab);

  // Added 06/2026.
  profile_prefs->ClearPref(kGlicSelectionWidgetDismissCount);

  // Added 06/2026.
  profile_prefs->ClearPref(kRealboxContextMenuAnimationState);

  // Added 06/2026
  profile_prefs->ClearPref(kTabSearchMigrationComplete);
  profile_prefs->ClearPref(kTabSearchPinnedToTabstripMigrationComplete);
  profile_prefs->ClearPref(kTabSearchPinnedToTabstripMigrationComplete2);

  // Added 06/2026.
  profile_prefs->ClearPref(kPersonalContextInAutofillNoticeShouldBeShown);

  // Added 06/2026
  profile_prefs->ClearPref(kDefaultBrowserInfobarLastDeclined);


  // Added 06/2026.
  syncer::ClearAccountKeyedPrefValue(
      profile_prefs, autofill::prefs::kAutofillAiOptInStatus, {});

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  // Added 07/2026.
  profile_prefs->ClearPref(kProxyOverrideRulesAffiliation);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  // Added 07/2026.
  profile_prefs->ClearPref(kMV2DeprecationWarningAcknowledgedGlobally);
  profile_prefs->ClearPref(kMV2DeprecationDisabledAcknowledgedGlobally);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

  // Added 07/2026.
  profile_prefs->ClearPref(kObsoleteManagementProfileLastLogTime);

  // Added 07/2026.
  profile_prefs->ClearPref(prefs::kProjectsPanelEntrypointEnabled);
  profile_prefs->ClearPref(prefs::kProjectsPanelPinnedToTabstrip);

  // Added 08/2026.
  profile_prefs->ClearPref(kInitialSendAnimationShown);
  profile_prefs->ClearPref(kPrivacySandboxNotices);
  profile_prefs->ClearPref(kPrivacySandboxM1ConsentDecisionMade);
  profile_prefs->ClearPref(kPrivacySandboxM1EEANoticeAcknowledged);
  profile_prefs->ClearPref(kPrivacySandboxM1RowNoticeAcknowledged);
  profile_prefs->ClearPref(kPrivacySandboxM1RestrictedNoticeAcknowledged);
  profile_prefs->ClearPref(kPrivacySandboxM1PromptSuppressed);
  profile_prefs->ClearPref(kPrivacySandboxNoticeDisplayed);
  profile_prefs->ClearPref(kPrivacySandboxConsentDecisionMade);
  profile_prefs->ClearPref(kPrivacySandboxNoConfirmationSandboxDisabled);
  profile_prefs->ClearPref(kPrivacySandboxNoConfirmationSandboxRestricted);
  profile_prefs->ClearPref(kPrivacySandboxNoConfirmationSandboxManaged);
  profile_prefs->ClearPref(
      kPrivacySandboxNoConfirmationThirdPartyCookiesBlocked);
  profile_prefs->ClearPref(kPrivacySandboxNoConfirmationManuallyControlled);
  profile_prefs->ClearPref(kPrivacySandboxDisabledInsufficientConfirmation);
  profile_prefs->ClearPref(kPrivacySandboxTopicsConsentGiven);
  profile_prefs->ClearPref(kPrivacySandboxTopicsConsentLastUpdateTime);
  profile_prefs->ClearPref(kPrivacySandboxTopicsConsentLastUpdateReason);
  profile_prefs->ClearPref(kPrivacySandboxTopicsConsentTextAtLastUpdate);
  profile_prefs->ClearPref(kPrivacySandboxAllowNoticeFor3PCBlockedTrial);


  // Added 08/2026.
  profile_prefs->ClearPref(kObsoleteAutofillWalletImportEnabled);
  profile_prefs->ClearPref(kObsoleteAutofillWalletImportEnabledMigrated);
  profile_prefs->ClearPref(kPrivacySandboxTopicsDataAccessibleSince);
  profile_prefs->ClearPref(kPrivacySandboxBlockedTopics);
  profile_prefs->ClearPref(kPrivacySandboxFledgeJoinBlocked);

  // Added 08/2026.
  profile_prefs->ClearPref(kShowRollbackUiModeB);
  profile_prefs->ClearPref(kBlockAll3pcToggleEnabled);
  profile_prefs->ClearPref(kTrackingProtection3pcdEnabled);
  profile_prefs->ClearPref(kExternalAppRedirectTimestamps);

  // Added 08/2026.
  profile_prefs->ClearPref(kUkmLoggingUserSecret);
  profile_prefs->ClearPref(kUkmLoggingUserSecretCreationTime);

  // Added 08/2026.
  profile_prefs->ClearPref(kEverythingMenuPinnedToTabstripMigrationComplete);

  // Added 08/2026.
  profile_prefs->ClearPref(kObsoleteNtpPromoBlocklist);

  // Added 08/2026.
  profile_prefs->ClearPref(kSigninInterceptionIDPCookiesUrl);


  // Added 09/2026.
  profile_prefs->ClearPref(kInvalidationPerSenderRegisteredForInvalidation);
  profile_prefs->ClearPref(kInvalidationPerSenderActiveRegistrationTokens);
  profile_prefs->ClearPref(kBookmarkEditorExpandedNodes);

#if BUILDFLAG(IS_WIN)
  // Added 09/2026.
  CdmPrefServiceHelper::MigrateObsoleteProfilePrefs(profile_prefs);
#endif  // BUILDFLAG(IS_WIN)


  // Added 09/2026.
#if BUILDFLAG(ENABLE_WEBUI_NTP)
  profile_prefs->ClearPref(kNtpDriveLastDismissedTime);
  profile_prefs->ClearPref(kNtpGoogleCalendarLastDismissedTime);
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)
  profile_prefs->ClearPref(kNtpMicrosoftAuthLastDismissedTime);
  profile_prefs->ClearPref(kNtpTabGroupsLastDismissedTime);
  // Added 09/2026.
  // TODO(b/566104798): Clean up deleted pref code after 1 year.
  profile_prefs->ClearPref(kAudioApiStableDeviceIds);

  // Added 09/2026.
  profile_prefs->ClearPref(
      kAddressSignInPromoShownCountPerProfileForLimitsExperiment);
  profile_prefs->ClearPref(
      kBookmarkSignInPromoShownCountPerProfileForLimitsExperiment);
  profile_prefs->ClearPref(
      kPasswordSignInPromoShownCountPerProfileForLimitsExperiment);
  profile_prefs->ClearPref(
      kAddressSignInPromoDismissCountPerProfileForLimitsExperiment);
  profile_prefs->ClearPref(
      kBookmarkSignInPromoDismissCountPerProfileForLimitsExperiment);
  profile_prefs->ClearPref(
      kPasswordSignInPromoDismissCountPerProfileForLimitsExperiment);


  // Added 10/2026.
  profile_prefs->ClearPref(kDismissedHistoryEmbeddingsScopePromo);
  profile_prefs->ClearPref(kDismissedHistoryEmbeddingsSettingsPromo);
  profile_prefs->ClearPref(kShownCountHistoryEmbeddingsScopePromo);
  profile_prefs->ClearPref(kShownCountHistoryEmbeddingsSettingsPromo);

  // Added 10/2026.
  profile_prefs->ClearPref(kSupervisedUserSafeSites);
  profile_prefs->ClearPref(kSupervisedUserManualHosts);
  profile_prefs->ClearPref(kSupervisedUserManualUrls);

  // Added 10/2026.
  profile_prefs->ClearPref(kTabSearchUsed);

  // Added 10/2026.
  profile_prefs->ClearPref(kSyncEncryptionBootstrapToken);
  profile_prefs->ClearPref(
      kSyncEncryptionBootstrapTokenPerAccountMigrationDone);

  // Added 10/2026.
  profile_prefs->ClearPref(kTailoredSecuritySyncFlowLastRunTime);
  profile_prefs->ClearPref(kTailoredSecuritySyncFlowLastUserInteractionState);
  profile_prefs->ClearPref(
      kTailoredSecuritySyncFlowObservedOutcomeUnsetTimestamp);
  profile_prefs->ClearPref(kSyncingThemePrefsMigratedToNonSyncing);
  profile_prefs->ClearPref(kShouldReadIncomingSyncingThemePrefs);
  profile_prefs->ClearPref(kDeprecatedBrowserColorSchemeDoNotUse);
  profile_prefs->ClearPref(kDeprecatedUserColorDoNotUse);
  profile_prefs->ClearPref(kDeprecatedBrowserColorVariantDoNotUse);
  profile_prefs->ClearPref(kDeprecatedGrayscaleThemeEnabledDoNotUse);
  profile_prefs->ClearPref(kDeprecatedNtpCustomBackgroundDictDoNotUse);

  // Added 10/2026.
  profile_prefs->ClearPref(kTabsFromOtherDevicesAutoPinnedMigration);

  // Added 10/2026.
  profile_prefs->ClearPref(kHistorySearchSettingState);


  // Added 11/2026.
  profile_prefs->ClearPref(kBundledSettingsCheckedMigrateUserToEnhancedBundle);

  // Added 11/2026.
  profile_prefs->ClearPref(kGlicPromotionSourceCohort);

  // Please don't delete the following line. It is used by PRESUBMIT.py.
  // END_MIGRATE_OBSOLETE_PROFILE_PREFS

  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.
}
