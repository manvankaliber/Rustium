# Copyright 2012 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This file is used to assign starting resource ids for resources and strings
# used by Chromium.  This is done to ensure that resource ids are unique
# across all the grd files.  If you are adding a new grd file, please add
# a new entry to this file.
#
# The entries below are organized into sections. When adding new entries,
# please use the right section. Try to keep entries in alphabetical order.
#
# - chrome/app/
# - chrome/browser/
# - chrome/ WebUI
# - chrome/ miscellaneous
# - components/
# - content/
# - everything else
#
# The range of ID values, which is used by pak files, is from 0 to 2^16 - 1.
#
# IMPORTANT: For update instructions, see README.md.
{
  # The first entry in the file, SRCDIR, is special: It is a relative path from
  # this file to the base of your checkout.
  "SRCDIR": "../..",

  # START chrome/app section.
  #
  # Chromium strings and Google Chrome strings must start at the same id.
  # We only use one file depending on whether we're building Chromium or
  # Google Chrome.
  "chrome/app/chromium_strings.grd": {
    "messages": [800],
  },
  "chrome/app/google_chrome_strings.grd": {
    "messages": [800],
  },

  # Leave lots of space for generated_resources since it has most of our
  # strings.
  "chrome/app/generated_resources.grd": {
    # Big alignment since strings (previous item) are frequently added.
    "META": {"join": 2, "align": 200},
    "messages": [1000],
  },

  "chrome/app/resources/locale_settings.grd": {
    # Big alignment since strings (previous item) are frequently added.
    "META": {"align": 1000},
    "messages": [2000],
  },

  # These each start with the same resource id because we only use one
  # file for each desktop build (linux, mac, or win).
  "chrome/app/resources/locale_settings_linux.grd": {
    # Preserve the common locale-group alignment after retiring ChromeOS.
    "META": {"align": 100},
    "messages": [2100],
  },
  "chrome/app/resources/locale_settings_mac.grd": {
    "messages": [2100],
  },
  "chrome/app/resources/locale_settings_win.grd": {
    "messages": [2100],
  },

  "chrome/app/theme/chrome_unscaled_resources.grd": {
    "META": {"join": 3},
    "includes": [2120],
  },

  # Leave space for theme_resources since it has many structures.
  "chrome/app/theme/theme_resources.grd": {
    "structures": [2160],
  },
  "chrome/app/theme/webui_theme_resources.grd": {
    "structures": [2180],
  },
  # END chrome/app section.

  # START chrome/browser section.
  "chrome/browser/browser_resources.grd": {
    # Big alignment at start of section.
    "META": {"align": 100},
    "includes": [2200],
    "structures": [2220],
  },
  # Both browser_resources.grd for chrome/browser/actor/resources and
  # chrome/browser/actor/resources/internal start with the same ids because only
  # one of them is built depending on if is_chrome_branded is true.
  "chrome/browser/actor/resources/internal/browser_resources.grd": {
    "structures": [2230],
    "messages": [2235],
    "includes": [2240],
  },
  "chrome/browser/actor/resources/browser_resources.grd": {
    "structures": [2230],
    "messages": [2235],
    "includes": [2240],
  },
  "chrome/browser/actor/resources/common_resources.grd": {
    "META": {"join": 2},
    "messages": [2245],
  },
  "chrome/browser/dev_ui_browser_resources.grd": {
    "includes": [2250],
  },
  # Both indigo_strings.grd for chrome/browser/indigo/resources and
  # chrome/browser/indigo/resources/internal start with the same ids because
  # only one of them is built.
  "chrome/browser/indigo/resources/internal/indigo_strings.grd": {
    "messages": [2255],
  },
  "chrome/browser/indigo/resources/indigo_strings.grd": {
    "messages": [2255],
  },
  "chrome/browser/platform_experience/win/resources/platform_experience_win_resources.grd": {
    "includes": [2300],
    "messages": [2320],
  },
  "chrome/browser/resources/app_icon/app_icon_resources.grd": {
    "structures": [2360],
  },
  "chrome/browser/resources/component_extension_resources.grd": {
    "includes": [2440],
    "structures": [2460],
  },
  "chrome/browser/resources/office_web_app/resources.grd": {
    "includes": [2480],
  },
  "chrome/browser/resources/preinstalled_web_apps/resources.grd": {
    "includes": [2500],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/drive_picker_host/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [2505],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/drive_picker_host/untrusted/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [2515],
  },
  # chrome/browser/glic/resources/internal/browser_resources.grd and
  # chrome/browser/glic/resources/browser_resources.grd must share the same id
  # because they define the same resources, but only one of them is built
  # depending on whether src_internal is available.
  "chrome/browser/glic/resources/internal/browser_resources.grd": {
    "messages": [2540],
    "includes": [2600],
  },
  "chrome/browser/glic/resources/browser_resources.grd": {
    "messages": [2540],
    "includes": [2600],
  },
  # chrome/browser/indigo/resources/internal/browser_resources.grd and
  # chrome/browser/indigo/resources/browser_resources.grd must share the same id
  # because they define the same resources, but only one of them is built
  # depending on whether src_internal is available.
  "chrome/browser/indigo/resources/internal/browser_resources.grd": {
    "includes": [2640],
  },
  "chrome/browser/indigo/resources/browser_resources.grd": {
    "includes": [2640],
  },
  # END chrome/browser section.

  # START chrome/ WebUI resources section
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/about_sys/resources.grd": {
    # Big alignment at start of section.
    "META": {"align": 100, "sizes": {"includes": [10]}},
    "includes": [2700],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/access_code_cast/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [2720],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/accessibility/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [2740],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/actor/core/internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [2750],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/actor_overlay/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [2755],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/app_home/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [2760],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/app_service_internals/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [2780],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/app_settings/resources.grd": {
    "META": {"sizes": {"includes": [45]}},
    "includes": [2800],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/bluetooth_internals/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [2880],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/bookmarks/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [2900],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/browser_actuator_internals/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [2910],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/browser_switch/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [2920],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/certificate_manager/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [2950],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/certificate_viewer/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [2960],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/components/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [3720],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/compose/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [3740],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/conflicts/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [3750],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/enterprise/connectors/resources/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [3760],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/context_hub/resources.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [3768],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/contextual_cueing/internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [3769],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/contextual_tasks/extension/resources.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [3770],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/contextual_tasks/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [3771],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/craft/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [3772],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/critical_actions/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [3775],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/data_sharing/resources.grd": {
   "META": {"sizes": {"includes": [20]}},
    "includes": [3780],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/device_log/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [3820],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/discards/resources.grd": {
    "META": {"sizes": {"includes": [30],}},
    "includes": [3840],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/default_browser/default_browser_modal/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [3870],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/default_browser/visual_guided_setter/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [3875],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/downloads/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [3880],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/engagement/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [3900],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/extensions/resources.grd": {
    "META": {"sizes": {"includes": [120],}},
    "includes": [3920],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/extensions_zero_state_promo/resources.grd": {
    "META": {"sizes": {"includes": [8],}},
    "includes": [3925],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/family_link_user_internals/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [3930],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/feed/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [3940],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/feedback/resources.grd": {
    "META": {"sizes": {"includes": [40],}},
    "includes": [3980],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/gaia_auth_host/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [4000],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/glic/resources.grd": {
    "META": {"sizes": {"includes": [60]}},
    "includes": [4010],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/glic/experimental_opt_in/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4020],
  },
    "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/glic/selection_overlay/resources.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [4033],
  },
    "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/guest_view_shared/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4035],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/history/resources.grd": {
    "META": {"sizes": {"includes": [60]}},
    "includes": [4040],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/indigo/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [4043],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/indigo_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
     "includes": [4045],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/infobar_internals/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4050],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/inline_login/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4080],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/inspect/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [4085],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/invalidations/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4100],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/intro/resources.grd": {
    "META": {"sizes": {"includes": [80],}},
    "includes": [4110],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/iwa_dev/resources.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [4150],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/key_value_pair_viewer_shared/resources.grd": {
   "META": {"sizes": {"includes": [10]}},
    "includes": [4160],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/private_ai/private_ai_internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4170],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/lens/overlay/resources.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [4180],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/lens/shared/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [4240],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/location_internals/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4260],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/management/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4280],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/media/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [4300],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/media_router/cast_feedback/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4320],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/media_router/internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4340],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/multistep_filter_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4370],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/net_internals/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [4380],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_footer/resources.grd": {
    "META": {"sizes": {"includes": [15]}},
    "includes": [4390],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_page/resources.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [4400],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_page/untrusted/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [4410],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_page_extension/resources.grd": {
    "META": {"sizes": {"includes": [80]}},
    "includes": [4415],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_page_instant/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4420],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_page_third_party/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4440],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/new_tab_shared/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4450],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/omnibox_everywhere/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [4470],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/omnibox_internals/aim_eligibility_extension/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4473],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/omnibox_internals/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [4476],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/omnibox_popup/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [4480],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/on_device_internals/resources.grd": {
    "META": {"sizes": {"includes": [40]}},
    "includes": [4500],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/on_device_translation_internals/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [4510],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/organizer_panel/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [4515],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/page_action_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4518],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/password_manager/resources.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [4520],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/pdf/resources.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [4540],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/predictors/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [4560],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/print_preview/resources.grd": {
    "META": {"sizes": {"includes": [500],}},
    "includes": [4580],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/content_settings/resources.grd": {
    "META": {"sizes": {"includes": [30],}},
    "includes": [4600],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/profile_internals/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4640],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/quota_internals/quota_internals_resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [4660],
  },
   "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_toolbar/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [4665],
  },
   "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_toolbar/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [4670],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/sandbox_internals/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [4680],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/saved_tab_groups_unsupported/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [4690],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/search_engine_choice/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [4700],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/segmentation_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4720],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/reset_password/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4730],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/settings/resources.grd": {
    "META": {"sizes": {"includes": [800],}},
    "includes": [4740],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/settings_shared/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [4760],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/bookmarks/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [45],}},
    "includes": [4770],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/bookmarks/resources.grd": {
    "META": {"sizes": {"includes": [45],}},
    "includes": [4780],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/comments/resources.grd": {
    "META": {"sizes": {"includes": [30],}},
    "includes": [4790],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/commerce/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [4800],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/customize_chrome/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [80],}},
    "includes": [4820],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/customize_chrome/resources.grd": {
    "META": {"sizes": {"includes": [100],}},
    "includes": [4840],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/history/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [4850],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/history_clusters/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [4860],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/read_anything/resources.grd": {
    "META": {"sizes": {"includes": [150],}},
    "includes": [4880],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/reading_list/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [15],}},
    "includes": [4890],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/reading_list/resources.grd": {
    "META": {"sizes": {"includes": [15],}},
    "includes": [4900],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/shared/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [15],}},
    "includes": [4910],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/shared/resources.grd": {
    "META": {"sizes": {"includes": [18],}},
    "includes": [4920],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/tabs_from_other_devices/code_cache_resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4930],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/side_panel/tabs_from_other_devices/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4935],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/signin/batch_upload/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4940],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/signin/history_sync_optin/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [4950],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/signin/profile_picker/resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [4960],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/signin/resources.grd": {
    "META": {"sizes": {"includes": [120],}},
    "includes": [4980],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/signin/signout_confirmation/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [4990],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/skills/resources.grd": {
    "META": {"sizes": {"includes": [60],}},
    "includes": [4991],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/subresource_filter/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [4995],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/suggest_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [5000],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/support_tool/resources.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [5020],
  }, "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tab_group_home/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5030],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tab_group_shared/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5035],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tab_search/resources.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [5040],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tab_strip/resources.grd": {
    "META": {"sizes": {"includes": [40]}},
    "includes": [5060],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tts_engine/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5070],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/usb_internals/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5080],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/user_education_internals/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5090],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/web_app_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [5100],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_gallery/resources.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [5140],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_js_error/resources.grd": {
   "META": {"sizes": {"includes": [10],}},
   "includes": [5160],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/whats_new/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [5200],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/color_pipeline_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [5220],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/watermark/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [5230],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_browser/resources.grd": {
    "META": {"sizes": {"includes": [80]}},
    "includes": [5250],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_browser/internal/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [5260],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/updater/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [5280],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webnn_internals/resources.grd": {
    "META": {"sizes": {"includes": [16]}},
    "includes": [5290],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/tab_strip_internals/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [5350],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/ttc/resources/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [5370],
  },
  # END chrome/ WebUI resources section

  # START chrome/ miscellaneous section.
  "chrome/common/common_resources.grd": {
    # Big alignment at start of section.
    "META": {"align": 100},
    "includes": [5500],
  },
  "chrome/credential_provider/gaiacp/gaia_resources.grd": {
    "includes": [5540],
    "messages": [5560],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/renderer/resources/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [5570],
  },
  "chrome/renderer/resources/renderer_resources.grd": {
    "includes": [5580],
    "structures": [5600],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/test/data/webui/resources.grd": {
    "META": {"sizes": {"includes": [2500],}},
    "includes": [5620],
  },
  # END chrome/ miscellaneous section.

  # START components/ section.
  "components/autofill/core/browser/geo/autofill_address_rewriter_resources.grd":{
    "META": {"align": 1000},
    "includes": [7000]
  },
  # Both Chrome and Chromium alternative states data must start with the same id.
  # Depending on the build only one of them is used.
  "components/autofill/core/browser/geo/autofill_alternative_state_name_map_resources.grd":{
    "META": {"sizes": {"includes": [250]}}, # Reserve space for ~225 resources
    "includes": [7010]
  },
  "components/autofill/core/browser/geo/chrome_alternative_state_name_maps/alternative_state_name_map_resources.grd":{
    "META": {"sizes": {"includes": [250]}}, # Reserve space for ~225 resources
    "includes": [7010]
  },
  # Chromium strings and Google Chrome strings must start at the same id.
  # We only use one file depending on whether we're building Chromium or
  # Google Chrome.
  "components/components_chromium_strings.grd": {
    # Big alignment at start of section.
    "META": {"join": 2},
    "messages": [7020],
  },
  "components/components_google_chrome_strings.grd": {
    "messages": [7020],
  },
  "components/components_locale_settings.grd": {
    "META": {"join": 2},
    "includes": [7040],
    "messages": [7060],
  },
  "components/components_strings.grd": {
    "messages": [7080],
  },
  # components_variant_internal_strings.grd and
  # components_variant_public_strings.grd must share the same id because they
  # define the same strings, but only one of them is built depending on whether
  # src_internal is available.
  "components/internal/components_variant_internal_strings.grd": {
    "messages": [7090],
  },
  "components/components_variant_public_strings.grd": {
    "messages": [7090],
  },
  "components/guest_view/browser/slim_web_view/strings/slim_web_view_strings.grd": {
    "messages": [7110],
  },
  "components/headless/command_handler/headless_command.grd": {
    "includes": [7120],
  },
  # metrics/internal/server_urls.grd and metrics/server_urls.grd must share the
  # same id because they define the same strings, but only one of them is built
  # depending on whether src_internal is available.
  "components/metrics/internal/server_urls.grd": {
    "messages": [7130],
  },
  "components/metrics/server_urls.grd": {
    "messages": [7130],
  },
  "components/omnibox/resources/omnibox_pedal_synonyms.grd": {
    "META": {"join": 2},
    "messages": [7140],
  },
  # components/policy/resources/policy_templates.grd and
  # components/policy/resources/policy_templates.build.grd must share the same
  # id because they are based on the same structure, however they are used in
  # different pipelines.
  "components/policy/resources/policy_templates.grd": {
    "structures": [7180],
  },
  "components/policy/resources/policy_templates.build.grd": {
    "structures": [7180],
  },
  "components/privacy_sandbox_strings.grd": {
    "messages": [7200],
  },
  "components/resources/components_resources.grd": {
    "includes": [7220],
  },
  "components/resources/components_scaled_resources.grd": {
    "structures": [7240],
  },
  "components/resources/dev_ui_components_resources.grd": {
    "includes": [7260],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/autofill/core/browser/autofill_and_password_manager_internals/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7270],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/autofill_ml_internals/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [7280],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/browser_apis/tab_strip/resources/resources.grd": {
    "META": {"sizes": {"includes": [15]}},
    "includes": [7290],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/commerce/core/internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [7300],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/data_sharing/data_sharing_internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7310],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/download/resources/download_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7320],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/crash/core/browser/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7330],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/gcm_driver/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7350],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/history_clusters/history_clusters_internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7360],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/management/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7370],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/metrics/debug/resources.grd": {
    "META": {"sizes": {"includes": [25]}},
    "includes": [7380],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/net_log/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7385],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/notebooks/internals/webui/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7390],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/ntp_tiles/webui/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7395],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/optimization_guide/optimization_guide_internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7400],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/policy/resources/webui/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [7420],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/safe_browsing/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7425],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/signin/core/browser/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7430],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/translate/translate_internals/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7435],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/ukm/debug/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7440],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/chrome_urls/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7450],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/flags/resources/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7455],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/internal_debug_pages_disabled/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7460],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/user_actions/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7465],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/webui/version/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7470],
  },
  "<(SHARED_INTERMEDIATE_DIR)/components/sync/service/resources/resources.grd": {
   "META": {"sizes": {"includes": [30],}},
    "includes": [7480],
  },
    "<(SHARED_INTERMEDIATE_DIR)/components/webui/regional_capabilities_internals/resources/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [7490],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/unexportable_keys_internals/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [7495],
  },
  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/feature_showcase/resources.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [7505],
  },
  # END components/ section.

  # START content/ section.
  "content/content_resources.grd": {
    # Big alignment at start of section.
    "META": {"join": 2, "align": 100},
    "includes": [8000],
  },
  "content/shell/shell_resources.grd": {
    "includes": [8020],
  },
  "content/test/web_ui_mojo_test_resources.grd": {
    "includes": [8040],
  },

  # These files are generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/gpu/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [8080],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/histograms/resources.grd": {
    "META": {"sizes": {"includes": [5]}},
    "includes": [8100],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/indexed_db/resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [8120],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/media/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [8140],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/net/resources.grd": {
    "META": {"sizes": {"includes": [5],}},
    "includes": [8160],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/process/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [8180],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/service_worker/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [8200],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/quota/resources.grd": {
    "META": {"sizes": {"includes": [10],}},
    "includes": [8220],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/traces_internals/resources.grd": {
    "META": {"sizes": {"includes": [30],}},
    "includes": [8240],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/resources/webxr_internals/resources.grd": {
    "META": {"sizes": {"includes": [20,],}},
    "includes": [8260],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/tracing/tracing_resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [8300],
  },
  "<(SHARED_INTERMEDIATE_DIR)/content/browser/webrtc/resources/resources.grd": {
    "META": {"sizes": {"includes": [20],}},
    "includes": [8320],
  },
  # END content/ section.

  # START "everything else" section.
  # Everything but chrome/, components/, and content/
  # TODO(b/207518736): Input overlay resources will be changed to proto soon,
  # thus not rushing to update it for now.
  "base/tracing/protos/resources.grd": {
    "includes": [9240],
  },

  "device/bluetooth/bluetooth_strings.grd": {
    "messages": [9320],
  },

  "device/fido/fido_strings.grd": {
    "messages": [9340],
  },

  "extensions/browser/resources/extensions_browser_resources.grd": {
    "structures": [9360],
  },
  "extensions/extensions_resources.grd": {
    "includes": [9370],
  },
  "extensions/renderer/resources/extensions_renderer_resources.grd": {
    "structures": [9390],
  },
  "<(SHARED_INTERMEDIATE_DIR)/extensions/renderer/resources/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [9400],
  },
  "extensions/strings/extensions_strings.grd": {
    "messages": [9420],
  },

  "mojo/public/js/mojo_bindings_resources.grd": {
    "includes": [9480],
  },

  "net/base/net_resources.grd": {
    "includes": [9500],
  },

  "remoting/resources/remoting_strings.grd": {
    "messages": [9520],
  },

  "services/device/public/cpp/usb/resources/usb_ids_resources.grd": {
    "includes": [9530],
  },
  "services/services_strings.grd": {
    "messages": [9540],
  },
  "third_party/blink/public/blink_image_resources.grd": {
    "structures": [9560],
  },
  "third_party/blink/public/blink_resources.grd": {
    "includes": [9580],
  },
  "third_party/blink/public/strings/blink_strings.grd": {
    "messages": [9600],
  },
  "third_party/blink/public/strings/permission_element_strings.grd": {
    "messages": [9620],
  },
  "third_party/blink/renderer/modules/media_controls/resources/media_controls_resources.grd": {
    "includes": [9640],
    "structures": [9640],
  },
  "third_party/libaddressinput/chromium/address_input_strings.grd": {
    "messages": [9680],
  },

  "ui/base/test/ui_base_test_resources.grd": {
    "messages": [9700],
  },
  "ui/resources/ui_resources.grd": {
    "structures": [9820],
  },
  "ui/resources/ui_lottie_resources.grd": {
    "structures": [9830],
  },
  "ui/resources/ui_unscaled_resources.grd": {
    "includes": [9840],
  },
  "ui/strings/app_locale_settings.grd": {
    "messages": [9860],
  },
  "ui/strings/auto_image_annotation_strings.grd": {
    "messages": [9870],
  },
  "ui/strings/ax_strings.grd": {
    "messages": [9880],
  },
  "ui/strings/ui_strings.grd": {
    "messages": [9900],
  },
  "ui/views/examples/views_examples_resources.grd": {
    "messages": [9920],
  },
  "ui/views/resources/views_resources.grd": {
    "structures": [9940],
  },
  "ui/webui/examples/resources/webui_examples_resources.grd": {
    "messages": [9960],
  },
  "<(SHARED_INTERMEDIATE_DIR)/ui/webui/examples/resources/browser/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [9980],
  },
  "<(SHARED_INTERMEDIATE_DIR)/ui/webui/resources/webui_code_cache_resources.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [9990],
  },
  "<(SHARED_INTERMEDIATE_DIR)/ui/webui/resources/webui_resources.grd": {
    "META": {"sizes": {"includes": [1100]}},
    "includes": [10000],
  },

  # This file is generated during the build.
  # .grd extension is required because it's checked before var interpolation.
  "<(DEVTOOLS_GRD_PATH).grd": {
    # In debug build, devtools frontend sources are not bundled and therefore
    # includes a lot of individual resources
    "META": {"sizes": {"includes": [4000],}},
    "includes": [10040],
  },

  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/resources/inspector_overlay/inspector_overlay_resources.grd": {
    "META": {"sizes": {"includes": [50],}},
    "includes": [10060],
  },

  "<(SHARED_INTERMEDIATE_DIR)/third_party/blink/public/strings/permission_element_generated_strings.grd": {
    "META": {"sizes": {"messages": [2000],}},
    "messages": [10080],
  },

  "third_party/search_engines_data/resources_internal/search_engines_scaled_resources.grd": {
    "structures": [10100],
  },

  "third_party/search_engines_data/resources_internal/search_engine_descriptions_strings.grd": {
    "messages": [10120],
  },

  "<(SHARED_INTERMEDIATE_DIR)/chrome/browser/resources/webui_toolbar_shared/resources.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [10140],
  },

  "<(SHARED_INTERMEDIATE_DIR)/THIS_IS_A_PLACEHOLDER.grd": {
    "META": {"sizes": {"includes": [800]}},
    "includes": [12000],
  },

  # END "everything else" section.
  # Everything but chrome/, components/, and content/

  # Thinking about appending to the end?
  # Please read the header and find the right section above instead.
}
