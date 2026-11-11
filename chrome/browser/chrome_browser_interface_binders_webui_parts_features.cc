// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "build/android_buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_browser_interface_binders_webui_parts.h"
#include "chrome/browser/contextual_tasks/contextual_tasks_ui.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/webui/private_ai_internals/private_ai_internals_ui.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_features.h"
#include "components/compose/buildflags.h"
#include "components/contextual_tasks/public/features.h"
#include "components/enterprise/buildflags/buildflags.h"
#include "components/on_device_translation/buildflags/buildflags.h"
#include "components/private_ai/features.h"
#include "components/private_ai/private_ai_internals/webui/private_ai_internals.mojom.h"
#include "components/safe_browsing/buildflags.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_ui_browser_interface_broker_registry.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/binder_map.h"

#include "chrome/browser/ttc/core/buildflags.h"
#if BUILDFLAG(ENABLE_TTC_WEBUI)
#include "chrome/browser/ttc/core/eval_harness.mojom.h"
#include "chrome/browser/ttc/core/switches.h"
#endif
#include "chrome/browser/ui/webui/ai_overlay_dialog/ai_overlay_dialog.mojom.h"
#include "chrome/browser/ui/webui/ai_overlay_dialog/ai_overlay_dialog_untrusted_ui.h"

#if BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)
#include "chrome/browser/resources/certificate_manager/certificate_manager.mojom.h"
#include "chrome/browser/ui/webui/certificate_manager/certificate_manager_ui.h"
#endif  // BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)

#if BUILDFLAG(ENABLE_COMPOSE)
#include "chrome/browser/ui/webui/compose/compose_untrusted_ui.h"
#include "chrome/common/compose/compose.mojom.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/ui/webui/signin/batch_upload/batch_upload.mojom.h"
#include "chrome/browser/ui/webui/signin/batch_upload_ui.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/ui/webui/feature_showcase/default_browser.mojom.h"
#include "chrome/browser/ui/webui/feature_showcase/feature_showcase.mojom.h"
#include "chrome/browser/ui/webui/feature_showcase/feature_showcase_ui.h"
#include "chrome/browser/ui/webui/feature_showcase/google_lens.mojom.h"
#include "chrome/browser/ui/webui/feature_showcase/password_manager.mojom.h"
#include "chrome/browser/ui/webui/history/history_cross_device_signin_promo_handler.h"
#include "chrome/browser/ui/webui/history/history_ui.h"
#include "chrome/browser/ui/webui/signin/cross_device_signin_qr_bubble/cross_device_signin_qr_bubble.mojom.h"
#include "chrome/browser/ui/webui/signin/cross_device_signin_qr_bubble_ui.h"
#include "chrome/browser/ui/webui/signin/signout_confirmation/signout_confirmation.mojom.h"
#include "chrome/browser/ui/webui/signin/signout_confirmation/signout_confirmation_ui.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/ui/webui/extensions_zero_state_promo/zero_state_promo_ui.h"
#endif

#if !BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "components/guest_view/browser/slim_web_view/slim_web_view.mojom.h"  // nogncheck
#endif


#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/ui/webui/tab_strip_internals/tab_strip_internals_ui.h"
#endif

#if BUILDFLAG(ENTERPRISE_WATERMARK)
#include "chrome/browser/ui/webui/watermark/watermark_ui.h"
#endif

#if BUILDFLAG(FULL_SAFE_BROWSING)
#include "chrome/browser/ui/webui/reset_password/reset_password.mojom.h"
#include "chrome/browser/ui/webui/reset_password/reset_password_ui.h"
#endif  // BUILDFLAG(FULL_SAFE_BROWSING)

namespace chrome::internal {

using content::RegisterWebUIControllerInterfaceBinder;

void PopulateChromeWebUIFrameBindersPartsFeatures(
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map,
    content::RenderFrameHost* render_frame_host) {
#if BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)
  RegisterWebUIControllerInterfaceBinder<
      certificate_manager::mojom::CertificateManagerPageHandlerFactory,
      CertificateManagerUI>(map);
#endif  // BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  RegisterWebUIControllerInterfaceBinder<
      batch_upload::mojom::PageHandlerFactory, BatchUploadUI>(map);
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  RegisterWebUIControllerInterfaceBinder<
      cross_device_signin::mojom::PageHandlerFactory,
      CrossDeviceSigninQrBubbleUI>(map);
  RegisterWebUIControllerInterfaceBinder<
      signout_confirmation::mojom::PageHandlerFactory, SignoutConfirmationUI>(
      map);
  RegisterWebUIControllerInterfaceBinder<
      history_cross_device_signin_promo::mojom::
          HistoryCrossDeviceSigninPromoHandler,
      HistoryUI>(map);
  RegisterWebUIControllerInterfaceBinder<
      feature_showcase::mojom::DefaultBrowserPageHandlerFactory,
      FeatureShowcaseUI>(map);
  RegisterWebUIControllerInterfaceBinder<
      feature_showcase::mojom::FeatureShowcasePageHandlerFactory,
      FeatureShowcaseUI>(map);
  RegisterWebUIControllerInterfaceBinder<
      feature_showcase::mojom::PasswordManagerPageHandlerFactory,
      FeatureShowcaseUI>(map);
  RegisterWebUIControllerInterfaceBinder<
      feature_showcase::mojom::ThemesAndCustomizationPageHandlerFactory,
      FeatureShowcaseUI>(map);

  RegisterWebUIControllerInterfaceBinder<
      feature_showcase::mojom::GoogleLensPageHandlerFactory, FeatureShowcaseUI>(
      map);
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

#if BUILDFLAG(ENABLE_EXTENSIONS)
  RegisterWebUIControllerInterfaceBinder<
      zero_state_promo::mojom::PageHandlerFactory,
      extensions::ZeroStatePromoController>(map);
  RegisterWebUIControllerInterfaceBinder<
      custom_help_bubble::mojom::CustomHelpBubbleHandlerFactory,
      extensions::ZeroStatePromoController>(map);
#endif


#if !BUILDFLAG(ENABLE_EXTENSIONS_CORE)
  if (base::FeatureList::IsEnabled(contextual_tasks::kContextualTasks)) {
    RegisterWebUIControllerInterfaceBinder<
        guest_view::mojom::PageHandlerFactory, ContextualTasksUI>(map);
  }
#endif


#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  RegisterWebUIControllerInterfaceBinder<
      tab_strip_internals::mojom::PageHandlerFactory, TabStripInternalsUI>(map);
#endif

#if BUILDFLAG(ENTERPRISE_WATERMARK)
  RegisterWebUIControllerInterfaceBinder<watermark::mojom::PageHandlerFactory,
                                         WatermarkUI>(map);
#endif

  if (base::FeatureList::IsEnabled(private_ai::kPrivateAi)) {
    RegisterWebUIControllerInterfaceBinder<
        private_ai_internals::mojom::PrivateAiInternalsPageHandler,
        private_ai::PrivateAiInternalsUI>(map);
  }

#if BUILDFLAG(FULL_SAFE_BROWSING)
  RegisterWebUIControllerInterfaceBinder<::mojom::ResetPasswordHandler,
                                         ResetPasswordUI>(map);
#endif
}

void PopulateChromeWebUIFrameInterfaceBrokersUntrustedPartsFeatures(
    content::WebUIBrowserInterfaceBrokerRegistry& registry) {
#if BUILDFLAG(ENABLE_COMPOSE)
  registry.ForWebUI<ComposeUntrustedUI>()
      .Add<compose::mojom::ComposeSessionUntrustedPageHandlerFactory>();
#endif  // BUILDFLAG(ENABLE_COMPOSE)

  if (base::FeatureList::IsEnabled(features::kAiOverlayDialog)
#if BUILDFLAG(ENABLE_TTC_WEBUI)
      || base::CommandLine::ForCurrentProcess()->HasSwitch(
             ttc::switches::kEnableTtcTestApi)
#endif
  ) {
    auto registration = registry.ForWebUI<ttc::AiOverlayDialogUntrustedUI>();
    registration.Add<ai_overlay_dialog::mojom::PageHandlerFactory>();
#if BUILDFLAG(ENABLE_TTC_WEBUI)
    registration.Add<ttc::mojom::HarnessPageHandlerFactory>();
#endif
  }
}

}  // namespace chrome::internal
