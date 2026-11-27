// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/searchbox/lens_searchbox_handler.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/webui/cr_components/searchbox/searchbox_omnibox_client.h"
#include "chrome/browser/ui/webui/searchbox/lens_searchbox_client.h"
#include "components/lens/lens_features.h"
#include "components/lens/proto/server/lens_overlay_response.pb.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "ui/base/mojom/window_open_disposition.mojom-shared.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/ui_base_features.h"
#include "url/gurl.h"

namespace {
// Interface that allows the Lens searchbox to interact with its embedder
// (i.e., LensSearchboxController).
class LensOmniboxClient : public SearchboxOmniboxClient {
 public:
  ~LensOmniboxClient() override;

  // OmniboxClient:
  SessionID GetSessionID() const override;
  const GURL& GetURL() const override;
  metrics::OmniboxEventProto::PageClassification GetPageClassification(
      bool is_prefetch) const override;
  void OnThumbnailRemoved() override;
  void OnAutocompleteAccept(
      const GURL& destination_url,
      TemplateURLRef::PostContent* post_content,
      ui::mojom::WindowOpenDisposition disposition,
      ui::PageTransition transition,
      omnibox::AutocompleteMatchType match_type,
      base::TimeTicks match_selection_timestamp,
      bool destination_url_entered_without_scheme,
      bool destination_url_entered_with_http_scheme,
      const std::u16string& text,
      const AutocompleteMatch& match,
      const AutocompleteMatch& alternative_nav_match) override;

 private:
};

LensOmniboxClient::~LensOmniboxClient() = default;

SessionID LensOmniboxClient::GetSessionID() const {
  return lens_searchbox_client_->GetTabId();
}

const GURL& LensOmniboxClient::GetURL() const {
  return lens_searchbox_client_->GetPageURL();
}


void LensOmniboxClient::OnThumbnailRemoved() {
  lens_searchbox_client_->OnThumbnailRemoved();
}

metrics::OmniboxEventProto::PageClassification
LensOmniboxClient::GetPageClassification(bool is_prefetch) const {
  return lens_searchbox_client_->GetPageClassification();
}

void LensOmniboxClient::OnAutocompleteAccept(
    const GURL& destination_url,
    TemplateURLRef::PostContent* post_content,
    ui::mojom::WindowOpenDisposition disposition,
    ui::PageTransition transition,
    omnibox::AutocompleteMatchType match_type,
    base::TimeTicks match_selection_timestamp,
    bool destination_url_entered_without_scheme,
    bool destination_url_entered_with_http_scheme,
    const std::u16string& text,
    const AutocompleteMatch& match,
    const AutocompleteMatch& alternative_nav_match) {
  lens_searchbox_client_->OnSuggestionAccepted(
      destination_url, match.type,
      match.subtypes.contains(omnibox::SUBTYPE_ZERO_PREFIX));
}

}  // namespace
