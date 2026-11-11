// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ttc/core/tool_controller.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ttc/core/page_context_util.h"
#include "chrome/browser/ttc/core/session_controller_impl.h"
#include "chrome/browser/ttc/core/session_journal_impl.h"
#include "chrome/browser/ttc/core/ttc_keyed_service.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/actor.mojom.h"
#include "chrome/common/actor/action_result.h"
#include "chrome/common/webui_url_constants.h"
#include "components/actor/core/journal_details_builder.h"
#include "components/actor/core/shared_types.h"
#include "components/page_content_annotations/content/page_context_fetcher.h"
#include "components/sessions/core/session_id.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"


namespace ttc {

namespace {

void OnToolCallFinished(
    std::unique_ptr<SessionJournalImpl::PendingAsyncEvent> journal_event,
    ToolResponseCallback callback,
    ToolResponse response) {
  if (response.Ok()) {
    details.Add("result", "ok");
  } else {
    const ToolError& error = response.error();
    if (error.code) {
      details.Add("code", *error.code);
    }
    details.AddError(error.message.value_or("Tool call failed"));
  }
  journal_event->EndEntry(std::move(details).Build());
  std::move(callback).Run(std::move(response));
}

// Whether window management tools are offered. They only make sense where
// Chrome runs in a desktop-style windowing environment with multiple
// user-visible windows: desktop platforms, and Android in desktop mode.
bool SupportsWindowManagementTools() {
  return true;
}

// Parses the `node_id` argument sent by the server. The model sees `node_id`
// as an integer, but the server post-processes it into a dictionary of the
// form {"document_id": <serialized_token>, "dom_node_id": <int>}.



}  // namespace

ToolController::ToolCallState::ToolCallState(ToolRequest request,
                                             ToolResponseCallback callback)
    : request(std::move(request)), callback(std::move(callback)) {}

ToolController::ToolCallState::ToolCallState(ToolCallState&&) = default;

ToolController::ToolCallState& ToolController::ToolCallState::operator=(
    ToolCallState&&) = default;

ToolController::ToolCallState::~ToolCallState() = default;

ToolController::ToolController(SessionControllerImpl& session_controller)
    : session_controller_(session_controller) {}

ToolController::~ToolController() = default;

// The actor only runs one action at a time per task, so tool calls are run one
// at a time in the order they're received: if a tool call is already
// running, `tool_request` is queued until all earlier calls have finished.
void ToolController::ProcessToolCall(const ToolRequest& tool_request,
                                     ToolResponseCallback callback) {
  const bool must_wait =
      in_flight_tool_call_.has_value() || !pending_tool_calls_.empty();
  if (must_wait) {
  }

  pending_tool_calls_.emplace_back(tool_request.Clone(), std::move(callback));
  if (!must_wait) {
    RunNextToolCall();
  }
}

void ToolController::RunNextToolCall() {
  CHECK(!in_flight_tool_call_);
  CHECK(!pending_tool_calls_.empty());
  in_flight_tool_call_.emplace(std::move(pending_tool_calls_.front()));
  pending_tool_calls_.pop_front();
  RunToolCall(in_flight_tool_call_->request,
              base::BindOnce(&ToolController::OnToolCallDone,
                             weak_factory_.GetWeakPtr()));
}

void ToolController::OnToolCallDone(ToolResponse response) {
  CHECK(in_flight_tool_call_);
  ToolCallState tool_call = std::move(*in_flight_tool_call_);
  in_flight_tool_call_.reset();

  // Start the next tool call asynchronously rather than from within the
  // previous call's completion, which may run synchronously from inside
  // RunToolCall() (e.g. on invalid arguments). This avoids reentrancy and
  // unbounded recursion when many queued calls fail synchronously.
  if (!pending_tool_calls_.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ToolController::RunNextToolCall,
                                  weak_factory_.GetWeakPtr()));
  }

  // Replying may end the session and destroy `this`, so do it last.
  std::move(tool_call.callback).Run(std::move(response));
}

void ToolController::RunToolCall(const ToolRequest& tool_request,
                                 ToolResponseCallback callback) {
  // The task may have been stopped while this call was queued. Replace it
  // before journaling so that the call is journaled under the task it runs in.



  // TODO(b/573150754): Migrate click_element, set_text, select_option, and
  // scroll_page to kClick, kType, kSelectOption, and kScrollPage once MES
  // supports rewriting dom_node_id for TOCTOU checks.
  if (tool_request.name == "click_element") {
    ClickElement(tool_request.arguments, std::move(callback));
    return;
  }

  if (tool_request.name == "set_text") {
    SetText(tool_request.arguments, std::move(callback));
    return;
  }

  if (tool_request.name == "select_option") {
    SelectOption(tool_request.arguments, std::move(callback));
    return;
  }

  if (tool_request.name == "scroll_page") {
    ScrollPage(tool_request.arguments, std::move(callback));
    return;
  }

  if (tool_request.name == "get_screenshot") {
    GetScreenshot(std::move(callback));
    return;
  }

}

std::vector<ToolDefinition> ToolController::GetToolDefinitions() {
  CHECK(actor_service);

  // TODO(b/573150754): Add kClick, kType, and kSelectOption once MES supports
  // rewriting dom_node_id for TOCTOU checks and ProcessToolCall() dispatches
  // their registry names.
  static constexpr actor::ToolIdSet kSupportedToolIds = {
      actor::ToolId::kPerformSearch,    actor::ToolId::kGoBack,
      actor::ToolId::kGoForward,        actor::ToolId::kReloadPage,
      actor::ToolId::kFindAndHighlight, actor::ToolId::kPlayVideo,
      actor::ToolId::kPauseVideo,       actor::ToolId::kSeekToTimestamp,
      actor::ToolId::kTranslatePage,    actor::ToolId::kPromptGeminiPanel,
      actor::ToolId::kCloseGeminiPanel, actor::ToolId::kCloseTab,
      actor::ToolId::kCreateTab,        actor::ToolId::kSwitchTab,
      actor::ToolId::kOpenUrl,          actor::ToolId::kEnterFullscreen,
      actor::ToolId::kExitFullscreen,   actor::ToolId::kOpenKnownPage,
  };

  if (SupportsWindowManagementTools()) {
  }

  // Legacy tools defined below: click_element, set_text, select_option,
  // scroll_page, and get_screenshot; plus close_window where window tools are
  // supported.
  constexpr size_t kNumLegacyTools = 6;
  std::vector<ToolDefinition> tools;
  tools.reserve(actor_tools.size() + kNumLegacyTools);
  for (const actor::ToolDefinition* actor_tool : actor_tools) {
    ToolDefinition tool;
    tool.name = actor_tool->name;
    tool.description = actor_tool->description;
    tool.parameters_json_schema = actor_tool->parameters_json_schema.Clone();
    tool.behavior = ToolDefinition::Behavior::kBlocking;
    tool.verbalization = ToolDefinition::Verbalization::kSilentAction;
    tools.push_back(std::move(tool));
  }

  // TODO(b/573150754): Remove legacy click_element, set_text, select_option,
  // and scroll_page definitions once MES supports rewriting dom_node_id for
  // TOCTOU checks and ProcessToolCall() dispatches kClick, kType,
  // kSelectOption, and kScrollPage.
  ToolDefinition click_element;
  click_element.name = "click_element";
  click_element.description =
      "Click or toggle an interactive element (such as a button, radio "
      "button, or checkbox) on the active webpage.";
  click_element.parameters_json_schema =
      base::DictValue()
          .Set("type", "object")
          .Set("properties",
               base::DictValue().Set("node_id",
                                     base::DictValue()
                                         .Set("type", "integer")
                                         .Set("description",
                                              "The numeric node ID of the "
                                              "target element (e.g. 101).")))
          .Set("required", base::ListValue().Append("node_id"));
  click_element.behavior = ToolDefinition::Behavior::kBlocking;
  click_element.verbalization = ToolDefinition::Verbalization::kSilentAction;
  tools.push_back(std::move(click_element));

  ToolDefinition set_text;
  set_text.name = "set_text";
  set_text.description =
      "Replace the contents of a text input field (such as a text box or text "
      "area) on the active webpage with the given text.";
  set_text.parameters_json_schema =
      base::DictValue()
          .Set("type", "object")
          .Set("properties",
               base::DictValue()
                   .Set("node_id",
                        base::DictValue()
                            .Set("type", "integer")
                            .Set("description",
                                 "The numeric node ID of the target input "
                                 "field (e.g. 101)."))
                   .Set("text",
                        base::DictValue()
                            .Set("type", "string")
                            .Set("description",
                                 "The text to enter into the input field.")))
          .Set("required", base::ListValue().Append("node_id").Append("text"));
  set_text.behavior = ToolDefinition::Behavior::kBlocking;
  set_text.verbalization = ToolDefinition::Verbalization::kSilentAction;
  tools.push_back(std::move(set_text));

  ToolDefinition select_option;
  select_option.name = "select_option";
  select_option.description =
      "Select an option from a dropdown (<select>) element on the active "
      "webpage.";
  select_option.parameters_json_schema =
      base::DictValue()
          .Set("type", "object")
          .Set("properties",
               base::DictValue()
                   .Set("node_id",
                        base::DictValue()
                            .Set("type", "integer")
                            .Set("description",
                                 "The numeric node ID of the target dropdown "
                                 "element (e.g. 101)."))
                   .Set("text", base::DictValue()
                                    .Set("type", "string")
                                    .Set("description",
                                         "The text of the <option> element to "
                                         "select.")))
          .Set("required", base::ListValue().Append("node_id").Append("text"));
  select_option.behavior = ToolDefinition::Behavior::kBlocking;
  select_option.verbalization = ToolDefinition::Verbalization::kSilentAction;
  tools.push_back(std::move(select_option));

  ToolDefinition scroll_page;
  scroll_page.name = "scroll_page";
  scroll_page.description =
      "Scrolls the page or a specific scrollable element in the given "
      "direction by one page, or all the way to its start or end.";
  scroll_page.parameters_json_schema =
      base::DictValue()
          .Set("type", "object")
          .Set(
              "properties",
              base::DictValue()
                  .Set("granularity",
                       base::DictValue()
                           .Set("type", "string")
                           .Set("description",
                                "The units to scroll by. \"page\" scrolls by "
                                "one page of the scroller. \"document\" "
                                "scrolls to the start or end of the "
                                "scroller.")
                           .Set("enum", base::ListValue().Append("page").Append(
                                            "document")))
                  .Set("direction",
                       base::DictValue()
                           .Set("type", "string")
                           .Set("description", "The direction to scroll.")
                           .Set("enum", base::ListValue()
                                            .Append("up")
                                            .Append("down")
                                            .Append("left")
                                            .Append("right")))
                  .Set("node_id",
                       base::DictValue()
                           .Set("type", "integer")
                           .Set("description",
                                "The DOM node ID of the scrollable element, "
                                "or 0 to scroll the main viewport.")))
          .Set("required", base::ListValue()
                               .Append("granularity")
                               .Append("direction")
                               .Append("node_id"));
  scroll_page.behavior = ToolDefinition::Behavior::kBlocking;
  scroll_page.verbalization = ToolDefinition::Verbalization::kSilentAction;
  tools.push_back(std::move(scroll_page));

  ToolDefinition get_screenshot;
  get_screenshot.name = "get_screenshot";
  get_screenshot.description =
      "Capture a screenshot of the active webpage's current viewport.";
  // The tool takes no arguments, so its schema is left empty.
  get_screenshot.behavior = ToolDefinition::Behavior::kBlocking;
  get_screenshot.verbalization = ToolDefinition::Verbalization::kSilentAction;
  tools.push_back(std::move(get_screenshot));

  if (SupportsWindowManagementTools()) {
    // Defined here rather than taken from the registry: the registry's
    // close_window takes a window_id, but the model has no window IDs. TTC
    // closes the window containing the tracked tab, as enter_fullscreen does.
    ToolDefinition close_window;
    close_window.description = "Close the active browser window.";
    // The tool takes no arguments, so its schema is left empty.
    close_window.behavior = ToolDefinition::Behavior::kBlocking;
    close_window.verbalization = ToolDefinition::Verbalization::kSilentAction;
    tools.push_back(std::move(close_window));
  }

  return tools;
}

Profile* ToolController::GetProfile() {
  return session_controller_->GetProfile();
}

void ToolController::ClickElement(const base::DictValue& arguments,
                                  ToolResponseCallback callback) {
  if (!page_target.has_value()) {
    return;
  }

  PerformActionOnTrackedContents(
      [&page_target](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::ClickToolRequest>(
            actor_surface_handle, *page_target, actor::mojom::ClickType::kLeft,
            actor::mojom::ClickCount::kSingle);
      },
      std::move(callback));
}

void ToolController::CloseCurrentTab(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::CloseTabToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::CloseCurrentWindow(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        // The tracked tab is always in a window, so this can't be null.
        const BrowserWindowInterface& browser =
            CHECK_DEREF(actor_surface_handle.GetTabHandle()
                            .Get()
                            ->GetBrowserWindowInterface());
        return std::make_unique<actor::CloseWindowToolRequest>(
            browser.GetSessionID().id());
      },
      std::move(callback));
}

void ToolController::CloseGeminiPanel(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::CloseGeminiPanelToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::CreateNewTab(ToolResponseCallback callback) {
  // `actor::CreateTabToolRequest` opens an about:blank tab, so open the New
  // Tab Page in a new tab via `actor::OpenUrlToolRequest` instead, matching
  // what Ctrl/Cmd+T does.
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::OpenUrlToolRequest>(
            actor_surface_handle, GURL(chrome::kChromeUINewTabURL),
            /*open_in_new_tab=*/true);
      },
      std::move(callback));
}

void ToolController::CreateNewWindow(ToolResponseCallback callback) {
  // The new window becomes active, so the session's tracked tab follows it.
}

void ToolController::FindAndHighlight(const base::DictValue& arguments,
                                      ToolResponseCallback callback) {
  if (!query) {
    return;
  }

  PerformActionOnTrackedContents(
      [&query](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::FindAndHighlightToolRequest>(
            actor_surface_handle, *query);
      },
      std::move(callback));
}

// TODO(b/573253710): Migrate this to be an actor tool once ActorKeyedService
// returns `ToolResponse`s.
void ToolController::GetScreenshot(ToolResponseCallback callback) {
  content::WebContents* contents =
      session_controller_->GetVoiceFocusedWebContents();
  if (!contents) {
    return;
  }

  content::Page& page = contents->GetPrimaryPage();
  const GURL& url = page.GetMainDocument().GetLastCommittedURL();
  // The session this controller belongs to is owned by the TtcKeyedService.
  const TtcKeyedService& ttc_service =
      CHECK_DEREF(TtcKeyedService::Get(GetProfile()));

  IsUrlSupportedForPageContext(
      url, ttc_service.enterprise_policy_checker(),
      base::BindOnce(&ToolController::OnGetScreenshotUrlChecked,
                     weak_factory_.GetWeakPtr(), page.GetWeakPtr(),
                     std::move(callback)));
}

void ToolController::OnGetScreenshotUrlChecked(
    base::WeakPtr<content::Page> page,
    ToolResponseCallback callback,
    bool is_url_supported) {
  if (!is_url_supported) {
    return;
  }

  content::WebContents* web_contents =
      session_controller_->GetVoiceFocusedWebContents();
  if (!web_contents) {
    return;
  }

  if (!page || &*page != &web_contents->GetPrimaryPage()) {
    return;
  }

  if (web_contents->IsCrashed()) {
    return;
  }

  page_content_annotations::ScreenshotOptions::ScreenshotCollectionOptions
      collection_options;
  collection_options.screenshot_image_format =
      page_content_annotations::ScreenshotOptions::ScreenshotImageFormat::kJpeg;
  collection_options.screenshot_compression_quality = page_content_annotations::
      ScreenshotOptions::ScreenshotCompressionQuality::kHigh;

  page_content_annotations::FetchPageContextOptions options;
  options.screenshot_options =
      page_content_annotations::ScreenshotOptions::ViewportOnly(
          /*paint_preview_options=*/std::nullopt, collection_options);

  screenshot_fetcher_ =
      std::make_unique<page_content_annotations::PageContextFetcher>(
          /*get_screenshot_service_callback=*/base::NullCallback(),
          /*progress_listener=*/nullptr,
          /*fetch_pdf_content_callback=*/base::NullCallback());
  screenshot_fetcher_->FetchStart(
      *web_contents, options,
      base::BindOnce(&ToolController::OnGetScreenshotFetched,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ToolController::OnGetScreenshotFetched(
    ToolResponseCallback callback,
    base::expected<
        std::unique_ptr<page_content_annotations::FetchPageContextResult>,
        page_content_annotations::FetchPageContextErrorDetails> result) {
  screenshot_fetcher_.reset();

  if (!result.has_value()) {
    switch (result.error().error_code) {
      case page_content_annotations::FetchPageContextError::
          kWebContentsWentAway:
        break;
      case page_content_annotations::FetchPageContextError::kWebContentsChanged:
        break;
      case page_content_annotations::FetchPageContextError::
          kPageContextNotEligible:
        break;
      default:
        break;
    }

    std::move(callback).Run(ToolResponse::Error(error, result.error().message));
    return;
  }

  // `PageContextFetcher` only reports success with a non-null result.
  CHECK(result.value());
  if (!result.value()->screenshot_result.has_value()) {
    return;
  }

  page_content_annotations::ScreenshotResult& screenshot_result =
      *result.value()->screenshot_result;
  if (screenshot_result.screenshot_data.empty()) {
    return;
  }

  base::DictValue result_dict;
  result_dict.Set(
      "image_b64",
      base::Base64Encode(std::move(screenshot_result.screenshot_data)));
  result_dict.Set("mime_type", screenshot_result.mime_type);
  std::move(callback).Run(ToolResponse::Success(std::move(result_dict)));
}

void ToolController::GoBack(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::HistoryBackToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::GoForward(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::HistoryForwardToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::OpenKnownPage(const base::DictValue& arguments,
                                   ToolResponseCallback callback) {
  if (!query || query->empty()) {
    return;
  }

}

void ToolController::OpenUrl(const base::DictValue& arguments,
                             ToolResponseCallback callback) {
  if (!url || url->empty()) {
    return;
  }

  if (!new_tab) {
    return;
  }

  PerformActionOnTrackedContents(
      [&url, &new_tab](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::OpenUrlToolRequest>(
            actor_surface_handle, GURL(*url), *new_tab);
      },
      std::move(callback));
}

void ToolController::PauseVideo(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::PauseMediaToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::PerformSearch(const base::DictValue& arguments,
                                   ToolResponseCallback callback) {
  if (!query) {
    return;
  }

  PerformActionOnTrackedContents(
      [&query](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::PerformSearchToolRequest>(
            actor_surface_handle, *query);
      },
      std::move(callback));
}

void ToolController::PlayVideo(ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::PlayMediaToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::PromptGeminiPanel(const base::DictValue& arguments,
                                       ToolResponseCallback callback) {
  if (!prompt || prompt->empty()) {
    return;
  }

  PerformActionOnTrackedContents(
      [&prompt](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::PromptGeminiPanelToolRequest>(
            glic::InvokeWithAutoSubmitPasskeyProvider::GetPassKey(),
            actor_surface_handle, *prompt);
      },
      std::move(callback));
}

void ToolController::ReloadPage(ToolResponseCallback callback) {
  // The actor rejects every action on a crashed tab, including reloading it,
  // and leaves recovering the tab to its client (see
  // ExecutionEngine::KickOffNextAction()), so reload a crashed tab directly.
  content::WebContents* contents =
      session_controller_->GetVoiceFocusedWebContents();
  if (contents && contents->IsCrashed()) {
    // Tool calls run one at a time, so no other reload can be in progress.
    CHECK(!crashed_tab_reload_callback_);
    crashed_tab_reload_callback_ = std::move(callback);
    Observe(contents);
    contents->GetController().Reload(content::ReloadType::NORMAL,
                                     /*check_for_repost=*/true);
    // Reload() doesn't start a navigation if it asks the user to confirm
    // resubmitting a form.
    if (crashed_tab_reload_callback_ &&
        !contents->GetController().GetPendingEntry()) {
      FinishCrashedTabReload();
    }
    return;
  }

  PerformActionOnTrackedContents(
      [](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::ReloadPageToolRequest>(
            actor_surface_handle);
      },
      std::move(callback));
}

void ToolController::FinishCrashedTabReload() {
  Observe(nullptr);
  std::move(crashed_tab_reload_callback_).Run(ToolResponse::Success());
}

void ToolController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInPrimaryMainFrame() &&
      !navigation_handle->IsSameDocument()) {
    FinishCrashedTabReload();
  }
}

void ToolController::ScrollPage(const base::DictValue& arguments,
                                ToolResponseCallback callback) {
  if (!granularity) {
    return;
  }

  if (!direction) {
    return;
  }

  if (!page_target.has_value()) {
    return;
  }

  PerformActionOnTrackedContents(
      [&page_target, &granularity,
       &direction](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::ScrollPageToolRequest>(
            actor_surface_handle, *page_target, *granularity, *direction);
      },
      std::move(callback));
}

void ToolController::SeekToTimestamp(const base::DictValue& arguments,
                                     ToolResponseCallback callback) {
  if (!timecode) {
    return;
  }

  if (!seek_time) {
    return;
  }

  PerformActionOnTrackedContents(
      [&seek_time](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::SeekMediaToolRequest>(
            actor_surface_handle, *seek_time);
      },
      std::move(callback));
}

void ToolController::SelectOption(const base::DictValue& arguments,
                                  ToolResponseCallback callback) {
  if (!page_target.has_value()) {
    return;
  }

  const std::string* text = arguments.FindString("text");
  if (!text) {
    return;
  }

  // In Ttc options are selected based on visible text.
  PerformActionOnTrackedContents(
      [&page_target, &text](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::SelectToolRequest>(
            actor_surface_handle, *page_target, *text,
            actor::mojom::SelectAction::OptionMatchMode::kText);
      },
      std::move(callback));
}

void ToolController::SetFullscreen(bool fullscreen,
                                   ToolResponseCallback callback) {
  PerformActionOnTrackedContents(
      [fullscreen](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        // The tracked tab is always in a window, so this can't be null.
        const BrowserWindowInterface& browser =
            CHECK_DEREF(actor_surface_handle.GetTabHandle()
                            .Get()
                            ->GetBrowserWindowInterface());
        const int32_t window_id = browser.GetSessionID().id();
        if (fullscreen) {
          return std::make_unique<actor::EnterFullscreenToolRequest>(window_id);
        }
        return std::make_unique<actor::ExitFullscreenToolRequest>(window_id);
      },
      std::move(callback));
}

void ToolController::SetText(const base::DictValue& arguments,
                             ToolResponseCallback callback) {
  if (!page_target.has_value()) {
    return;
  }

  if (!text) {
    return;
  }

  PerformActionOnTrackedContents(
      [&page_target, &text](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::TypeToolRequest>(
            actor_surface_handle, *page_target, *text,
            /*follow_by_enter=*/false, actor::TypeToolRequest::Mode::kReplace);
      },
      std::move(callback));
}

void ToolController::SwitchTab(const base::DictValue& arguments,
                               ToolResponseCallback callback) {
  if (!query || query->empty()) {
    return;
  }

}

void ToolController::TranslatePage(const base::DictValue& arguments,
                                   ToolResponseCallback callback) {
  // If `target_language` is empty, the page is translated to the user's
  // preferred language.
  if (!target_language) {
    return;
  }

  PerformActionOnTrackedContents(
      [&target_language](actor::ActorSurfaceHandle actor_surface_handle)
          -> std::unique_ptr<actor::ToolRequest> {
        return std::make_unique<actor::TranslatePageToolRequest>(
            actor_surface_handle, *target_language);
      },
      std::move(callback));
}




}  // namespace ttc
