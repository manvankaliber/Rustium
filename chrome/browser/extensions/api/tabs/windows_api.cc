// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/tabs/windows_api.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/types/optional_util.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/api/tabs/tabs_util.h"
#include "chrome/browser/extensions/api/tabs/windows_util.h"
#include "chrome/browser/extensions/browser_extension_window_controller.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/open_tab_helper.h"
#include "chrome/browser/extensions/window_controller.h"
#include "chrome/browser/extensions/window_controller_list.h"
#include "chrome/browser/picture_in_picture/picture_in_picture_window_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/create_browser_window.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/web_applications/web_app_helpers.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/navigation_handle.h"
#include "extensions/common/extension.h"
#include "extensions/common/mojom/api_permission_id.mojom-shared.h"
#include "extensions/common/permissions/permissions_data.h"
#include "ui/base/base_window.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/base/mojom/window_show_state.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

#include "base/strings/stringprintf.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_init_state.h"
#include "chrome/browser/ui/waap/initial_webui_window_metrics_manager.h"
#include "chrome/browser/ui/waap/waap_utils.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/browser/web_applications/web_app_filter.h"
#include "chrome/browser/web_applications/web_app_provider.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "components/webapps/isolated_web_apps/scheme.h"


static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

namespace extensions {

namespace windows = api::windows;

namespace {

constexpr char kMissingLockWindowFullscreenPrivatePermission[] =
    "Cannot lock window to fullscreen or close a locked fullscreen window "
    "without lockWindowFullscreenPrivate manifest permission";

// A helper class to extract popular properties from different arguments.
template <typename T>
class ApiParameterExtractor {
 public:
  explicit ApiParameterExtractor(std::optional<T>& params) : params_(*params) {}
  ~ApiParameterExtractor() = default;

  bool populate_tabs() {
    if (params_->query_options && params_->query_options->populate) {
      return *params_->query_options->populate;
    }
    return false;
  }

  WindowController::TypeFilter type_filters() {
    if (params_->query_options && params_->query_options->window_types) {
      return WindowController::GetFilterFromWindowTypes(
          *params_->query_options->window_types);
    }
    return WindowController::kNoWindowFilter;
  }

 private:
  raw_ref<T> params_;
};

// Returns true if the given `extension` has API access to the locked
// fullscreen permission.
bool ExtensionHasLockedFullscreenPermission(const Extension* extension) {
  return extension && extension->permissions_data()->HasAPIPermission(
                          mojom::APIPermissionID::kLockWindowFullscreenPrivate);
}

// Converts the given `state` to the mojom::WindowShowState equivalent.
ui::mojom::WindowShowState ConvertToWindowShowState(
    windows::WindowState state) {
  switch (state) {
    case windows::WindowState::kNormal:
      return ui::mojom::WindowShowState::kNormal;
    case windows::WindowState::kMinimized:
      return ui::mojom::WindowShowState::kMinimized;
    case windows::WindowState::kMaximized:
      return ui::mojom::WindowShowState::kMaximized;
    case windows::WindowState::kFullscreen:
    case windows::WindowState::kLockedFullscreen:
      return ui::mojom::WindowShowState::kFullscreen;
    case windows::WindowState::kNone:
      return ui::mojom::WindowShowState::kDefault;
  }
  NOTREACHED();
}

// Returns whether the given `bounds` intersect with at least 50% of all the
// displays.
bool WindowBoundsIntersectDisplays(const gfx::Rect& bounds) {
  // Bail if `bounds` has an overflown area.
  auto checked_area = bounds.size().GetCheckedArea();
  if (!checked_area.IsValid()) {
    return false;
  }

  // An empty rect cannot intersect any display.
  if (bounds.IsEmpty()) {
    return false;
  }

  const int bounds_area = checked_area.ValueOrDie();

  int64_t intersect_area = 0;
  for (const auto& display : display::Screen::Get()->GetAllDisplays()) {
    gfx::Rect display_bounds = display.bounds();
    display_bounds.Intersect(bounds);
    intersect_area += display_bounds.size().GetArea();
  }
  // Compare using multiplication rather than division, which would truncate
  // to zero for areas smaller than two.
  return 2 * intersect_area >= bounds_area;
}

constexpr char kInvalidWindowTypeError[] = "Invalid value for type";

constexpr char kWindowCreateSupportsOnlySingleIwaUrlError[] =
    "When creating a window for a URL with the 'isolated-app:' scheme, only "
    "one tab can be added to the window.";
constexpr char kWindowCreateCannotParseIwaUrlError[] =
    "Unable to parse 'isolated-app:' URL: %s";
constexpr char kWindowCreateCannotUseTabIdWithIwaError[] =
    "Creating a new window for an Isolated Web App does not support adding a "
    "tab by its ID.";


bool IsValidStateForWindowsCreateFunction(
    const windows::Create::Params::CreateData* create_data) {
  if (!create_data) {
    return true;
  }

  bool has_bound = create_data->left || create_data->top ||
                   create_data->width || create_data->height;

  switch (create_data->state) {
    case windows::WindowState::kMinimized:
      // If minimised, default focused state should be unfocused.
      return !(create_data->focused && *create_data->focused) && !has_bound;
    case windows::WindowState::kMaximized:
    case windows::WindowState::kFullscreen:
    case windows::WindowState::kLockedFullscreen:
      // If maximised/fullscreen, default focused state should be focused.
      return !(create_data->focused && !*create_data->focused) && !has_bound;
    case windows::WindowState::kNormal:
    case windows::WindowState::kNone:
      return true;
  }
  NOTREACHED();
}


// Returns the IsolatedWebAppUrlInfo for the given call to windows.create() if
// the call is to create a new IWA window.
// Populates `error` with an error if the call is invalid.
// Note that returning std::nullopt *can* be valid (if error is unpopulated);
// this indicates the call is not for an IWA window.
std::optional<web_app::IsolatedWebAppUrlInfo> GetIsolatedWebAppInfo(
    const std::optional<windows::Create::Params::CreateData>& create_data,
    const std::vector<GURL>& parsed_urls,
    std::string* error) {
  if (parsed_urls.size() > 1) {
    if (std::ranges::any_of(parsed_urls, [](const GURL& url) {
          return url.SchemeIs(webapps::kIsolatedAppScheme);
        })) {
      // Invalid. Can only open a single IWA URL.
      *error = kWindowCreateSupportsOnlySingleIwaUrlError;
      return std::nullopt;
    }
  }

  if (parsed_urls.empty() ||
      !parsed_urls[0].SchemeIs(webapps::kIsolatedAppScheme)) {
    // Valid; not opening an IWA.
    return std::nullopt;
  }

  base::expected<web_app::IsolatedWebAppUrlInfo, std::string> maybe_url_info =
      web_app::IsolatedWebAppUrlInfo::Create(parsed_urls[0]);

  if (!maybe_url_info.has_value()) {
    // Invalid. Failed to create IWA info.
    *error = base::StringPrintf(kWindowCreateCannotParseIwaUrlError,
                                maybe_url_info.error().c_str());
    return std::nullopt;
  }

  // Validate `create_data` params to make sure they're compatible with IWAs.
  if (create_data) {
    if (create_data->tab_id) {
      // Invalid. Can't specify tab ID with IWAs.
      *error = kWindowCreateCannotUseTabIdWithIwaError;
      return std::nullopt;
    }

    switch (create_data->type) {
      case windows::CreateType::kNone:
      case windows::CreateType::kNormal:
        break;  // Valid type.
      case windows::CreateType::kPopup:
      case windows::CreateType::kPanel:
        // Invalid window type for IWAs.
        *error = kInvalidWindowTypeError;
        return std::nullopt;
    }

    if (create_data->set_self_as_opener && *create_data->set_self_as_opener) {
      // Invalid. Can't have openers with IWAs.
      *error = "Cannot specify setSelfAsOpener for isolated-app:// URLs.";
      return std::nullopt;
    }
  }

  // Valid IWA parameters.
  return *maybe_url_info;
}

class ScopedPinBrowserAtFront {
 public:
  explicit ScopedPinBrowserAtFront(BrowserWindowInterface* bwi)
      : bwi_(bwi->GetWeakPtr()) {
    old_z_order_level_ = bwi->GetWindow()->GetZOrderLevel();
    bwi->GetWindow()->SetZOrderLevel(ui::ZOrderLevel::kFloatingWindow);
  }

  ~ScopedPinBrowserAtFront() {
    if (bwi_) {
      bwi_->GetWindow()->SetZOrderLevel(old_z_order_level_);
    }
  }

 private:
  base::WeakPtr<BrowserWindowInterface> bwi_;
  ui::ZOrderLevel old_z_order_level_;
};



// Updates `window_bounds` from `params`. Returns true if bounds were set.
bool UpdateWindowBoundsFromParams(const api::windows::Update::Params& params,
                                  gfx::Rect& window_bounds) {
  bool set_window_bounds = false;
  if (params.update_info.left) {
    window_bounds.set_x(*params.update_info.left);
    set_window_bounds = true;
  }
  if (params.update_info.top) {
    window_bounds.set_y(*params.update_info.top);
    set_window_bounds = true;
  }
  if (params.update_info.width) {
    window_bounds.set_width(*params.update_info.width);
    set_window_bounds = true;
  }
  if (params.update_info.height) {
    window_bounds.set_height(*params.update_info.height);
    set_window_bounds = true;
  }
  return set_window_bounds;
}

}  // namespace

ExtensionFunction::ResponseAction WindowsGetFunction::Run() {
  std::optional<windows::Get::Params> params =
      windows::Get::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  ApiParameterExtractor<windows::Get::Params> extractor(params);
  WindowController* window_controller = nullptr;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(this, params->window_id,
                                               extractor.type_filters(),
                                               &window_controller, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  WindowController::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? WindowController::kPopulateTabs
                                : WindowController::kDontPopulateTabs;
  base::DictValue windows = window_controller->CreateWindowValueForExtension(
      extension(), populate_tab_behavior, source_context_type());
  return RespondNow(WithArguments(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetCurrentFunction::Run() {
  std::optional<windows::GetCurrent::Params> params =
      windows::GetCurrent::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  ApiParameterExtractor<windows::GetCurrent::Params> extractor(params);
  WindowController* window_controller = nullptr;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, extension_misc::kCurrentWindowId, extractor.type_filters(),
          &window_controller, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  WindowController::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? WindowController::kPopulateTabs
                                : WindowController::kDontPopulateTabs;
  base::DictValue windows = window_controller->CreateWindowValueForExtension(
      extension(), populate_tab_behavior, source_context_type());
  return RespondNow(WithArguments(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetLastFocusedFunction::Run() {
  std::optional<windows::GetLastFocused::Params> params =
      windows::GetLastFocused::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  ApiParameterExtractor<windows::GetLastFocused::Params> extractor(params);

  BrowserWindowInterface* last_focused_browser = nullptr;
  ForEachCurrentBrowserWindowInterfaceOrderedByActivation(
      [&](BrowserWindowInterface* browser) {
        if (windows_util::CanOperateOnWindow(
                this, BrowserExtensionWindowController::From(browser),
                extractor.type_filters())) {
          last_focused_browser = browser;
          return false;  // Stop iterating.
        }
        return true;  // Continue iterating.
      });
  if (!last_focused_browser) {
    return RespondNow(Error(tabs_constants::kNoLastFocusedWindowError));
  }

  WindowController::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? WindowController::kPopulateTabs
                                : WindowController::kDontPopulateTabs;
  base::DictValue windows = ExtensionTabUtil::CreateWindowValueForExtension(
      *last_focused_browser, extension(), populate_tab_behavior,
      source_context_type());
  return RespondNow(WithArguments(std::move(windows)));
}

ExtensionFunction::ResponseAction WindowsGetAllFunction::Run() {
  std::optional<windows::GetAll::Params> params =
      windows::GetAll::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  ApiParameterExtractor<windows::GetAll::Params> extractor(params);
  base::ListValue window_list;
  WindowController::PopulateTabBehavior populate_tab_behavior =
      extractor.populate_tabs() ? WindowController::kPopulateTabs
                                : WindowController::kDontPopulateTabs;
  for (WindowController* controller : *WindowControllerList::GetInstance()) {
    if (!controller->GetBrowserWindowInterface() ||
        !windows_util::CanOperateOnWindow(this, controller,
                                          extractor.type_filters())) {
      continue;
    }
    window_list.Append(ExtensionTabUtil::CreateWindowValueForExtension(
        *controller->GetBrowserWindowInterface(), extension(),
        populate_tab_behavior, source_context_type()));
  }

  return RespondNow(WithArguments(std::move(window_list)));
}

WindowsCreateFunction::WindowsCreateFunction() = default;
WindowsCreateFunction::~WindowsCreateFunction() = default;

ExtensionFunction::ResponseAction WindowsCreateFunction::Run() {
  std::optional<windows::Create::Params> params =
      windows::Create::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  CHECK(extension() || source_context_type() == mojom::ContextType::kWebUi ||
            source_context_type() == mojom::ContextType::kUntrustedWebUi,
        base::NotFatalUntil::M161);
  create_data_ = std::move(params->create_data);

  // Look for optional url.
  if (create_data_ && create_data_->url) {
    std::vector<std::string> url_strings;
    // First, get all the URLs the client wants to open.
    if (create_data_->url->as_string) {
      url_strings.push_back(std::move(*create_data_->url->as_string));
    } else if (create_data_->url->as_strings) {
      url_strings = std::move(*create_data_->url->as_strings);
    }

    // Second, resolve, validate and convert them to GURLs.
    for (auto& url_string : url_strings) {
      auto url = ExtensionTabUtil::PrepareURLForNavigation(
          url_string, extension(), browser_context());
      if (!url.has_value()) {
        return RespondNow(Error(std::move(url.error())));
      }
      urls_.push_back(*url);
    }
  }

  std::string error;

  isolated_web_app_url_info_ =
      GetIsolatedWebAppInfo(create_data_, urls_, &error);
  if (!error.empty()) {
    return RespondNow(Error(std::move(error)));
  }

  // Decide whether we are opening a normal window or an incognito window.
  Profile* calling_profile = Profile::FromBrowserContext(browser_context());
  windows_util::IncognitoResult incognito_result =
      windows_util::ShouldOpenIncognitoWindow(
          calling_profile,
          create_data_ && create_data_->incognito
              ? std::optional<bool>(*create_data_->incognito)
              : std::nullopt,
          &urls_, &error);
  if (incognito_result == windows_util::IncognitoResult::kError) {
    return RespondNow(Error(std::move(error)));
  }

  Profile* window_profile =
      incognito_result == windows_util::IncognitoResult::kIncognito
          ? calling_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true)
          : calling_profile;

  if (!IsValidStateForWindowsCreateFunction(
          base::OptionalToPtr(create_data_))) {
    return RespondNow(Error(tabs_constants::kInvalidWindowStateError));
  }

  // Look for optional tab id.
  bool is_locked_fullscreen =
      create_data_ &&
      create_data_->state == windows::WindowState::kLockedFullscreen;
  WindowController* source_window = nullptr;
  if (create_data_ && create_data_->tab_id) {
    // Find the tab.
    content::WebContents* web_contents = nullptr;
    if (!tabs_util::GetTabById(*create_data_->tab_id, calling_profile,
                               include_incognito_information(), &source_window,
                               &web_contents,
                               /*index_out=*/nullptr, &error)) {
      return RespondNow(Error(std::move(error)));
    }

    // Validate the tab information. Return an error if it's not valid.
    auto tab_validation = tabs_util::ValidateTabForMove(
        source_window, window_profile, web_contents, is_locked_fullscreen);
    if (!tab_validation.has_value()) {
      return RespondNow(Error(std::move(tab_validation.error())));
    }
  }

  if (is_locked_fullscreen) {
    if (!ExtensionHasLockedFullscreenPermission(extension())) {
      return RespondNow(Error(kMissingLockWindowFullscreenPrivatePermission));
    }
  }

  BrowserWindowInterface::Type window_type =
      BrowserWindowInterface::TYPE_NORMAL;

  gfx::Rect window_bounds;
  std::string extension_id;

  if (create_data_) {
    // Figure out window type before figuring out bounds so that default
    // bounds can be set according to the window type.
    switch (create_data_->type) {
      // TODO(stevenjb): Remove 'panel' from windows.json.
      case windows::CreateType::kPanel:
      case windows::CreateType::kPopup:
        window_type = BrowserWindowInterface::TYPE_POPUP;
        if (extension()) {
          extension_id = extension()->id();
        }
        break;
      case windows::CreateType::kNone:
      case windows::CreateType::kNormal:
        break;
      default:
        return RespondNow(Error(kInvalidWindowTypeError));
    }

      // Initialize default window bounds according to window type.
      // TODO(https://crbug.com/545671279): Properly initialize window bounds.
    ui::mojom::WindowShowState ignored_show_state =
        ui::mojom::WindowShowState::kDefault;
    WindowSizer::GetBrowserWindowBoundsAndShowState(
        gfx::Rect(), nullptr, &window_bounds, &ignored_show_state);

    // Update the window bounds based on the create parameters.
    std::string bounds_error = SetWindowBounds(*create_data_, window_bounds);
    if (!bounds_error.empty()) {
      return RespondNow(Error(std::move(bounds_error)));
    }

    set_self_as_opener_ =
        create_data_->set_self_as_opener && *create_data_->set_self_as_opener;
    if (is_from_service_worker() && set_self_as_opener_) {
      // TODO(crbug.com/40636155): Add test for this.
      return RespondNow(
          Error("Cannot specify setSelfAsOpener Service Worker extension."));
    }
  }

  // Create a new BrowserWindow if possible.
  if (GetBrowserWindowCreationStatusForProfile(*window_profile) !=
      BrowserWindowInterface::CreationStatus::kOk) {
    return RespondNow(Error(ExtensionTabUtil::kBrowserWindowNotAllowed));
  }
  BrowserWindowCreateParams create_params(window_type, *window_profile,
                                          user_gesture());

  bool initialized_type = false;
  if (isolated_web_app_url_info_.has_value()) {
    create_params.type = BrowserWindowInterface::TYPE_APP;
    create_params.app_name = web_app::GenerateApplicationNameFromAppId(
        isolated_web_app_url_info_->app_id());
    // For Isolated Web Apps, the actual navigating-to URL will be the app's
    // start_url to prevent deep-linking attacks, while the original URL will be
    // accessible via window.launchQueue; for this reason the browser is marked
    // trusted.
    create_params.is_trusted_source = true;
    initialized_type = true;
  }

  if (!initialized_type && !extension_id.empty()) {
    // extension_id is only set for CREATE_TYPE_POPUP.

    // On non-Android platforms, we use TYPE_APP_POPUP. On Android, this is
    // unsupported, so we use TYPE_POPUP.
    // TODO(https://crbug.com/469000733): Investigate if we can just use
    // TYPE_POPUP everywhere.
    create_params.type =
        BrowserWindowInterface::TYPE_APP_POPUP;

    // TODO(https://crbug.com/545671279): Initialize app name on android, or
    // verify this is unnecessary.
    create_params.app_name =
        web_app::GenerateApplicationNameFromAppId(extension_id);
    create_params.is_trusted_source = false;
    initialized_type = true;
  }
  create_params.initial_bounds = window_bounds;
  create_params.initial_show_state = ui::mojom::WindowShowState::kNormal;

  if (create_data_ && create_data_->state != windows::WindowState::kNone) {
    create_params.initial_show_state =
        ConvertToWindowShowState(create_data_->state);
  }

  const base::TimeTicks now = base::TimeTicks::Now();
  BrowserWindowInterface* new_window =
      CreateBrowserWindow(std::move(create_params));
  if (auto* manager = InitialWebUIWindowMetricsManager::From(new_window)) {
    manager->SetWindowCreationInfo(waap::NewWindowCreationSource::kExtension,
                                   now);
  }
  ExtensionFunction::ResponseValue response =
      OnBrowserWindowCreated(new_window);
  return RespondNow(std::move(response));
}


ExtensionFunction::ResponseValue WindowsCreateFunction::OnBrowserWindowCreated(
    BrowserWindowInterface* new_window) {
  if (!new_window) {
    return Error(ExtensionTabUtil::kBrowserWindowNotAllowed);
  }
  // NOTE: Even though `new_window` was returned, it may not be fully
  // initialized on non-desktop platforms. See documentation on
  // CreateBrowserWindow().

  auto create_nav_params = [&](const GURL& url, bool is_first_nav) {
    NavigateParams navigate_params(new_window, url, ui::PAGE_TRANSITION_LINK);

    navigate_params.disposition =
        ui::mojom::WindowOpenDisposition::kNewForegroundTab;

    // Ensure that these navigations will not get 'captured' into PWA windows,
    // as this means that `new_window` could be ignored. It may be
    // useful/desired in the future to allow this behavior, but this may require
    // an API change, or at least a re-write of how these navigations are called
    // to be compatible with the navigation capturing behavior.
    navigate_params.web_app_navigation_data.emplace();
    navigate_params.web_app_navigation_data->SetNavigationCapturingForceOff(
        true);

    if (OpenTabHelper::MaybeSetPdfNavigateParams(*this, navigate_params)) {
      return navigate_params;
    }

    if (set_self_as_opener_) {
      // Depending on the `setSelfAsOpener` option, we need to put the new
      // contents in the same BrowsingInstance as their opener.  See also
      // https://crbug.com/40516654.
      //
      // TODO(crbug.com/40636155): Add tests for checking opener SiteInstance
      // behavior from a SW based extension's extension frame (e.g. from popup).
      // See ExtensionApiTest.WindowsCreate* tests for details.
      navigate_params.initiator_origin =
          extension() ? extension()->origin()
                      : render_frame_host()->GetLastCommittedOrigin();
      navigate_params.opener = render_frame_host();
      navigate_params.source_site_instance =
          render_frame_host()->GetSiteInstance();
    }

    return navigate_params;
  };

  bool navigated = false;
  if (isolated_web_app_url_info_) {
    CHECK_EQ(urls_.size(), 1U);
    const GURL& original_url = urls_[0];

    const webapps::AppId& iwa_id = isolated_web_app_url_info_->app_id();
    web_app::WebAppRegistrar& registrar =
        web_app::WebAppProvider::GetForWebApps(new_window->GetProfile())
            ->registrar_unsafe();

    // TODO(crbug.com/424128443): create an dummy tab in the browser so that the
    // returned window's tab count is always equal to 1 -- this will limit the
    // extension's ability to figure out which IWAs are installed without the
    // `tabs` permission.
    if (registrar.AppMatches(iwa_id, web_app::WebAppFilter::IsIsolatedApp())) {
      NavigateParams navigate_params = create_nav_params(
          registrar.GetAppStartUrl(iwa_id), /*is_first_nav=*/true);
      webapps::LaunchParams launch_params;
      CHECK(navigate_params.web_app_navigation_data);
      launch_params.set_app_id(iwa_id);
      launch_params.set_target_url(original_url);
      navigate_params.web_app_navigation_data->SetLaunchParams(
          std::move(launch_params));

      // Navigate() takes care of enqueueing the launch params once the
      // navigation commits.
      base::WeakPtr<content::NavigationHandle> handle =
          Navigate(&navigate_params);
      CHECK(handle);
    }
    navigated = true;
  }

  if (!navigated) {
    bool is_first_nav = true;
    for (const GURL& url : urls_) {
      NavigateParams navigate_params = create_nav_params(url, is_first_nav);
      is_first_nav = false;
      Navigate(&navigate_params);
    }
  }

  TabListInterface* tab_list = TabListInterface::From(new_window);
  CHECK(tab_list);

  bool moved_tab = false;
  // Move the tab into the created window only if it's an empty popup or it's
  // a tabbed window.
  if (new_window->GetType() == BrowserWindowInterface::TYPE_NORMAL ||
      urls_.empty()) {
    if (create_data_ && create_data_->tab_id) {
      std::string error;
      // -1 means "move tab to the end", which is what we want.
      int new_index = -1;
      if (tabs_util::MoveTabToWindow(
              this, *create_data_->tab_id, new_window, new_index,
              /*allow_other_window_types=*/true, &error) < 0) {
        return Error(std::move(error));
      }

      moved_tab = true;
    }
  }

  // Create a new tab if the created window is still empty. Don't create a new
  // tab when it is intended to create an empty popup.
  // TODO(https://crbug.com/545671279): Port to desktop android.
  if (!moved_tab && urls_.empty() &&
      new_window->GetType() == BrowserWindowInterface::TYPE_NORMAL) {
    // TODO(crbug.com/452431839) Make a new NewTabTypes value for
    // when new tabs are made because of an empty window.
    chrome::NewTab(new_window, NewTabTypes::kNoUserAction);
  }

  // Select the first tab in the window, if there's at least one tab. There may
  // be no tabs, since we allow the creation of an empty popup above.
  if (tab_list->GetTabCount() > 0) {
    tab_list->ActivateTab(tab_list->GetTab(0)->GetHandle());
  }

  bool focused = true;
  if (create_data_ && create_data_->focused) {
    focused = *create_data_->focused;
  }

  // Showing or transitioning the window state can spin a nested event loop on
  // some platforms (e.g., macOS fullscreen transitions) and synchronously close
  // the window, destroying `new_window`.
  base::WeakPtr<BrowserWindowInterface> weak_window = new_window->GetWeakPtr();

  if (focused) {
    new_window->GetWindow()->Show();
  } else {
    // TODO(https://crbug.com/545671279): Port to desktop android.
    // Show an unfocused new window.
    BrowserWindowInterface* const last_active_bwi =
        GetLastActiveBrowserWindowInterfaceWithAnyProfile();

    // On some OSes the new unfocused window is shown on top by default.
    // ScopedPinBrowserAtFront prevents the new browser from being shown above
    // the old active browser.
    if (last_active_bwi && last_active_bwi->IsActive()) {
      ScopedPinBrowserAtFront scoper(last_active_bwi);
      new_window->GetWindow()->ShowInactive();
    } else {
      new_window->GetWindow()->ShowInactive();
    }
  }
  if (!weak_window) {
    return Error(ExtensionTabUtil::kNoCurrentWindowError);
  }

// Despite creating the window with initial_show_state() ==
// ui::mojom::WindowShowState::kMinimized above, on Linux the window is not
// created as minimized.
// TODO(crbug.com/40254339): Remove this workaround when linux is fixed.
// TODO(crbug.com/40254339): Find a fix for wayland as well.
#if BUILDFLAG(IS_LINUX) && BUILDFLAG(SUPPORTS_OZONE_X11)
  if (BrowserInitState::From(new_window)->initial_show_state() ==
      ui::mojom::WindowShowState::kMinimized) {
    new_window->GetWindow()->Minimize();
    if (!weak_window) {
      return Error(ExtensionTabUtil::kNoCurrentWindowError);
    }
  }
#endif  // BUILDFLAG(IS_LINUX) && BUILDFLAG(SUPPORTS_OZONE_X11)

  // Lock the window fullscreen only after the new tab has been created
  // (otherwise the tabstrip is empty), and window()->show() has been called
  // (otherwise that resets the locked mode for devices in tablet mode).
  // TODO(crbug.com/438540029) - Remove once the migration is complete.
  if (create_data_ &&
      create_data_->state == windows::WindowState::kLockedFullscreen) {
  }

  if (new_window->GetProfile()->IsOffTheRecord() &&
      !browser_context()->IsOffTheRecord() &&
      !include_incognito_information()) {
    // Don't expose incognito windows if extension itself works in non-incognito
    // profile and CanCrossIncognito isn't allowed.
    return WithArguments(base::Value());
  }

  return WithArguments(ExtensionTabUtil::CreateWindowValueForExtension(
      *new_window, extension(), WindowController::kPopulateTabs,
      source_context_type()));
}

// static
// static
std::string WindowsCreateFunction::SetWindowBounds(
    const api::windows::Create::Params::CreateData& create_data,
    gfx::Rect& window_bounds) {
  bool set_window_position = false;
  bool set_window_size = false;
  if (create_data.left) {
    window_bounds.set_x(*create_data.left);
    set_window_position = true;
  }
  if (create_data.top) {
    window_bounds.set_y(*create_data.top);
    set_window_position = true;
  }
  if (create_data.width) {
    window_bounds.set_width(*create_data.width);
    set_window_size = true;
  }
  if (create_data.height) {
    window_bounds.set_height(*create_data.height);
    set_window_size = true;
  }

  // If the extension specified the window size but no position, adjust the
  // window to fit in the display.
  if (!set_window_position && set_window_size) {
    const display::Display& display =
        display::Screen::Get()->GetDisplayMatching(window_bounds);
    window_bounds.AdjustToFit(display.bounds());
  }

  // Immediately fail if the window bounds don't intersect the displays. If
  // only a position was specified and the default bounds have not been
  // initialized (see above), there is no size to validate against yet.
  const bool has_bounds_to_validate =
      set_window_size || (set_window_position && !window_bounds.IsEmpty());
  if (has_bounds_to_validate && !WindowBoundsIntersectDisplays(window_bounds)) {
    return tabs_constants::kInvalidWindowBoundsError;
  }

  return std::string();  // No error.
}


ExtensionFunction::ResponseAction WindowsUpdateFunction::Run() {
  std::optional<windows::Update::Params> params =
      windows::Update::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  WindowController* window_controller = nullptr;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, params->window_id, WindowController::GetAllWindowFilter(),
          &window_controller, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  BrowserWindowInterface* browser =
      window_controller->GetBrowserWindowInterface();
  if (!browser) {
    return RespondNow(Error(ExtensionTabUtil::kNoCrashBrowserError));
  }
  ui::BaseWindow* browser_window = browser->GetWindow();

  // Before changing any of a window's state, validate the update parameters.
  // This prevents Chrome from performing "half" an update.

  // Update the window bounds if the bounds from the update parameters intersect
  // the displays.
  gfx::Rect window_bounds = browser_window->IsMinimized()
                                ? browser_window->GetRestoredBounds()
                                : browser_window->GetBounds();
  const bool set_window_bounds =
      UpdateWindowBoundsFromParams(*params, window_bounds);

  if (set_window_bounds && !WindowBoundsIntersectDisplays(window_bounds)) {
    return RespondNow(Error(tabs_constants::kInvalidWindowBoundsError));
  }

  ui::mojom::WindowShowState show_state =
      ConvertToWindowShowState(params->update_info.state);
  if (set_window_bounds &&
      (show_state == ui::mojom::WindowShowState::kMinimized ||
       show_state == ui::mojom::WindowShowState::kMaximized ||
       show_state == ui::mojom::WindowShowState::kFullscreen)) {
    return RespondNow(Error(tabs_constants::kInvalidWindowStateError));
  }

  // Prevent Picture-in-Picture windows from being updated to fullscreen,
  // maximized, or minimized states (https://crbug.com/514080341).
  content::WebContents* active_contents = window_controller->GetActiveTab();
  if (active_contents &&
      PictureInPictureWindowManager::IsChildWebContents(active_contents)) {
    if (show_state == ui::mojom::WindowShowState::kFullscreen ||
        show_state == ui::mojom::WindowShowState::kMaximized ||
        show_state == ui::mojom::WindowShowState::kMinimized) {
      return RespondNow(
          Error(tabs_constants::kNotAllowedForPictureInPictureError));
    }
  }

  if (params->update_info.focused) {
    bool focused = *params->update_info.focused;
    // A window cannot be focused and minimized, or not focused and maximized
    // or fullscreened.
    if (focused && show_state == ui::mojom::WindowShowState::kMinimized) {
      return RespondNow(Error(tabs_constants::kInvalidWindowStateError));
    }
    if (!focused && (show_state == ui::mojom::WindowShowState::kMaximized ||
                     show_state == ui::mojom::WindowShowState::kFullscreen)) {
      return RespondNow(Error(tabs_constants::kInvalidWindowStateError));
    }
  }


  // Parameters are valid. Now to perform the actual updates. Platform window
  // operations (such as entering or exiting fullscreen) may spin a nested event
  // loop on some platforms (e.g. macOS) and synchronously close the window,
  // destroying `browser` and `window_controller`.

  if (!UpdateWindowState(*params, browser, window_controller, show_state,
                         set_window_bounds, window_bounds)) {
    return RespondNow(Error(ExtensionTabUtil::kNoCurrentWindowError));
  }

  return RespondNow(
      WithArguments(window_controller->CreateWindowValueForExtension(
          extension(), WindowController::kDontPopulateTabs,
          source_context_type())));
}

bool WindowsUpdateFunction::UpdateWindowState(
    const api::windows::Update::Params& params,
    BrowserWindowInterface* browser,
    WindowController* window_controller,
    ui::mojom::WindowShowState show_state,
    bool set_window_bounds,
    const gfx::Rect& window_bounds) {
  base::WeakPtr<BrowserWindowInterface> weak_browser = browser->GetWeakPtr();
  base::WeakPtr<WindowController> weak_window_controller =
      window_controller->GetWeakPtr();
  ui::BaseWindow* browser_window = browser->GetWindow();

  if (show_state != ui::mojom::WindowShowState::kFullscreen &&
      show_state != ui::mojom::WindowShowState::kDefault) {
    window_controller->SetFullscreenMode(false, extension()->url());
    if (!weak_browser || !weak_window_controller) {
      return false;
    }
  }

  switch (show_state) {
    case ui::mojom::WindowShowState::kMinimized:
      browser_window->Minimize();
      break;
    case ui::mojom::WindowShowState::kMaximized:
      browser_window->Maximize();
      break;
    case ui::mojom::WindowShowState::kFullscreen:
      if (browser_window->IsMinimized() || browser_window->IsMaximized()) {
        browser_window->Restore();
        if (!weak_browser || !weak_window_controller) {
          return false;
        }
      }
      window_controller->SetFullscreenMode(true, extension()->url());
      break;
    case ui::mojom::WindowShowState::kNormal:
      browser_window->Restore();
      break;
    default:
      break;
  }
  if (!weak_browser || !weak_window_controller) {
    return false;
  }

  if (set_window_bounds) {
    // TODO(varkha): Updating bounds during a drag can cause problems and a more
    // general solution is needed. See http://crbug.com/40322435 .
    browser_window->SetBounds(window_bounds);
    if (!weak_browser || !weak_window_controller) {
      return false;
    }
  }

  if (params.update_info.focused) {
    if (*params.update_info.focused) {
      browser_window->Activate();
    } else {
      browser_window->Deactivate();
    }
    if (!weak_browser || !weak_window_controller) {
      return false;
    }
  }

  if (params.update_info.draw_attention) {
    browser_window->FlashFrame(*params.update_info.draw_attention);
  }

  return weak_browser && weak_window_controller;
}

ExtensionFunction::ResponseAction WindowsRemoveFunction::Run() {
  std::optional<windows::Remove::Params> params =
      windows::Remove::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  WindowController* window_controller = nullptr;
  std::string error;
  if (!windows_util::GetControllerFromWindowID(
          this, params->window_id, WindowController::kNoWindowFilter,
          &window_controller, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  // TODO(https://crbug.com/432056907): Determine if we need locked-fullscreen
  // support on desktop android.

  TabListInterface* tab_list =
      TabListInterface::From(window_controller->GetBrowserWindowInterface());
  if (tab_list && !tab_list->IsThisTabListEditable()) {
    return RespondNow(Error(ExtensionTabUtil::kTabStripNotEditableError));
  }
  window_controller->window()->Close();  // nocheck
  return RespondNow(NoArguments());
}

}  // namespace extensions
