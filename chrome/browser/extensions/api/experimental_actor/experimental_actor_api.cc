// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/experimental_actor/experimental_actor_api.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "base/check.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/task/single_thread_task_runner.h"
#include "base/version_info/channel.h"
#include "chrome/browser/ai/ai_data_keyed_service.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/actor.mojom-shared.h"
#include "chrome/common/actor/action_result.h"
#include "chrome/common/extensions/api/experimental_actor.h"
#include "chrome/common/extensions/api/tabs.h"
#include "components/actor/core/aggregated_journal_file_serializer.h"
#include "components/actor/core/journal_details_builder.h"
#include "components/actor/core/task_id.h"
#include "components/actor/core/task_source_info.h"
#include "components/actor/public/mojom/actor_types.mojom.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/optimization_guide/proto/features/model_prototyping.pb.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/tabs/public/tab_handle_factory.h"
#include "components/tabs/public/tab_interface.h"
#include "extensions/browser/api/constants.h"
#include "extensions/common/features/feature_channel.h"

namespace extensions {

namespace {


// Converts a session tab id to a tab handle.
int32_t ConvertSessionTabIdToTabHandle(
    int32_t session_tab_id,
    content::BrowserContext* browser_context) {
  return tabs::SessionMappedTabHandleFactory::GetInstance()
      .GetHandleForSessionId(session_tab_id);
}

// Converts a tab handle to a session tab id.
int32_t ConvertTabHandleToSessionTabId(
    int32_t tab_handle,
    content::BrowserContext* browser_context) {
  return tabs::SessionMappedTabHandleFactory::GetInstance()
      .GetSessionIdForHandle(tab_handle)
      .value_or(api::tabs::TAB_ID_NONE);
}

// Helper function to convert the session tab id to a tab handle for any action
// that has a `tab_id` field.
template <typename T>
void ConvertActionTabId(T* action_payload,
                        content::BrowserContext* browser_context) {
  action_payload->set_tab_id(ConvertSessionTabIdToTabHandle(
      action_payload->tab_id(), browser_context));
}

const void* const kSerializerKey = &kSerializerKey;

// File location that the actor journal should be serialized to.
const char kExperimentalActorJournalLog[] = "experimental-actor-journal";

class Serializer : public base::SupportsUserData::Data {
 public:


 private:
  void InitDone(bool success) {
    if (!success) {
      serializer_.reset();
    }
  }

};

}  // namespace

ExperimentalActorApiFunction::ExperimentalActorApiFunction() = default;

ExperimentalActorApiFunction::~ExperimentalActorApiFunction() = default;

bool ExperimentalActorApiFunction::PreRunValidation(std::string* error) {
  if (GetCurrentChannel() == version_info::Channel::STABLE &&
      !AiDataKeyedService::IsExtensionAllowlistedForStable(extension_id())) {
    *error = "API access not allowed on this channel.";
    return false;
  }

  if (!AiDataKeyedService::IsExtensionAllowlistedForActions(extension_id())) {
    *error = "Actions API access restricted for this extension.";
    return false;
  }

  if (!actor_service) {
    *error = "Incognito profile not supported.";
    return false;
  }

  Serializer::EnsureInitialized(browser_context(), actor_service->GetJournal());
  return true;
}

ExperimentalActorStopTaskFunction::ExperimentalActorStopTaskFunction() =
    default;

ExperimentalActorStopTaskFunction::~ExperimentalActorStopTaskFunction() =
    default;

ExtensionFunction::ResponseAction ExperimentalActorStopTaskFunction::Run() {
  auto params = api::experimental_actor::StopTask::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);


  return RespondNow(
      ArgumentList(api::experimental_actor::StopTask::Results::Create()));
}

ExperimentalActorCreateTaskFunction::ExperimentalActorCreateTaskFunction() =
    default;
ExperimentalActorCreateTaskFunction::~ExperimentalActorCreateTaskFunction() =
    default;

ExtensionFunction::ResponseAction ExperimentalActorCreateTaskFunction::Run() {

  return RespondNow(ArgumentList(
      api::experimental_actor::CreateTask::Results::Create(task_id.value())));
}

ExperimentalActorPerformActionsFunction::
    ExperimentalActorPerformActionsFunction() = default;
ExperimentalActorPerformActionsFunction::
    ~ExperimentalActorPerformActionsFunction() = default;

ExtensionFunction::ResponseAction
ExperimentalActorPerformActionsFunction::Run() {
  base::TimeTicks start_time = base::TimeTicks::Now();
  auto params = api::experimental_actor::PerformActions::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  optimization_guide::proto::Actions actions;
  if (!actions.ParseFromArray(params->actions_proto.data(),
                              params->actions_proto.size())) {
    return RespondNow(
        Error("Parsing optimization_guide::proto::Actions failed."));
  }

  // Convert from extension tab ids to TabHandles.
  for (auto& action : *actions.mutable_actions()) {
    switch (action.action_case()) {
      case optimization_guide::proto::Action::kClick:
        ConvertActionTabId(action.mutable_click(), browser_context());
        break;
      case optimization_guide::proto::Action::kType:
        ConvertActionTabId(action.mutable_type(), browser_context());
        break;
      case optimization_guide::proto::Action::kScroll:
        ConvertActionTabId(action.mutable_scroll(), browser_context());
        break;
      case optimization_guide::proto::Action::kMoveMouse:
        ConvertActionTabId(action.mutable_move_mouse(), browser_context());
        break;
      case optimization_guide::proto::Action::kDragAndRelease:
        ConvertActionTabId(action.mutable_drag_and_release(),
                           browser_context());
        break;
      case optimization_guide::proto::Action::kSelect:
        ConvertActionTabId(action.mutable_select(), browser_context());
        break;
      case optimization_guide::proto::Action::kNavigate:
        ConvertActionTabId(action.mutable_navigate(), browser_context());
        break;
      case optimization_guide::proto::Action::kBack:
        ConvertActionTabId(action.mutable_back(), browser_context());
        break;
      case optimization_guide::proto::Action::kForward:
        ConvertActionTabId(action.mutable_forward(), browser_context());
        break;
      case optimization_guide::proto::Action::kCloseTab:
        ConvertActionTabId(action.mutable_close_tab(), browser_context());
        break;
      case optimization_guide::proto::Action::kActivateTab:
        ConvertActionTabId(action.mutable_activate_tab(), browser_context());
        break;
      case optimization_guide::proto::Action::kAttemptLogin:
        ConvertActionTabId(action.mutable_attempt_login(), browser_context());
        break;
      case optimization_guide::proto::Action::kScriptTool:
        ConvertActionTabId(action.mutable_script_tool(), browser_context());
        break;
      case optimization_guide::proto::Action::kScrollTo:
        ConvertActionTabId(action.mutable_scroll_to(), browser_context());
        break;
      case optimization_guide::proto::Action::kAttemptFormFilling:
        ConvertActionTabId(action.mutable_attempt_form_filling(),
                           browser_context());
        break;
      case optimization_guide::proto::Action::kAttemptOtpFilling:
        ConvertActionTabId(action.mutable_attempt_otp_filling(),
                           browser_context());
        break;
      case optimization_guide::proto::Action::kChangePassword:
        ConvertActionTabId(action.mutable_change_password(), browser_context());
        break;
      case optimization_guide::proto::Action::kTranslatePage:
        ConvertActionTabId(action.mutable_translate_page(), browser_context());
        break;
      case optimization_guide::proto::Action::kWait:
      case optimization_guide::proto::Action::kCreateTab:
      case optimization_guide::proto::Action::kCreateWindow:
      case optimization_guide::proto::Action::kCloseWindow:
      case optimization_guide::proto::Action::kActivateWindow:
      case optimization_guide::proto::Action::kYieldToUser:
      case optimization_guide::proto::Action::kMediaControl:
      case optimization_guide::proto::Action::kLoadAndExtractContent:
      case optimization_guide::proto::Action::ACTION_NOT_SET:
        // No tab id to convert.
        break;
      default:
        NOTIMPLEMENTED();
        break;
    }
  }



  // If the client didn't create a task or passed in the wrong task id, return
  // failure.
  if (!task) {
    return RespondNow(
        Error(absl::StrFormat("Invalid task_id[%d].", task_id.value())));
  }


  bool skip_async_observation_information =
      actions.has_skip_async_observation_collection() &&
      actions.skip_async_observation_collection();
  if (!requests.has_value()) {
    return RespondLater();
  }


  return RespondLater();
}



ExperimentalActorRequestTabObservationFunction::
    ExperimentalActorRequestTabObservationFunction() = default;
ExperimentalActorRequestTabObservationFunction::
    ~ExperimentalActorRequestTabObservationFunction() = default;

ExtensionFunction::ResponseAction
ExperimentalActorRequestTabObservationFunction::Run() {
  auto params =
      api::experimental_actor::RequestTabObservation::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  content::WebContents* web_contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(params->tab_id, browser_context(),
                                    include_incognito_information(),
                                    &web_contents)) {
    return RespondNow(Error(ErrorUtils::FormatErrorMessage(
        kTabNotFoundError, base::NumberToString(params->tab_id))));
  }
  tabs::TabInterface* tab =
      tabs::TabInterface::MaybeGetFromContents(web_contents);
  // Can be null for pre-render web-contents.
  // TODO(crbug.com/369319589): Remove this logic.
  if (!tab) {
    return RespondNow(Error(ErrorUtils::FormatErrorMessage(
        kTabNotFoundError, base::NumberToString(params->tab_id))));
  }


  // TODO(dtapuska): We may want to add an optional task_id to the API so
  // we can attribute this tab observation to an appropriate task.

  return RespondLater();
}


}  // namespace extensions
