// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/contextual_tasks/contextual_tasks_context_service_factory.h"

#include "base/feature_list.h"
#include "chrome/browser/contextual_tasks/contextual_tasks_context_service.h"
#include "chrome/browser/contextual_tasks/contextual_tasks_service_factory.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/page_content_annotations/page_content_extraction_service_factory.h"
#include "chrome/browser/page_content_annotations/page_embeddings_service_factory.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "components/contextual_tasks/public/contextual_tasks_service.h"
#include "components/contextual_tasks/public/features.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

namespace contextual_tasks {

// static
ContextualTasksContextService*
ContextualTasksContextServiceFactory::GetForProfile(Profile* profile) {
  return nullptr;
}

// static
ContextualTasksContextServiceFactory*
ContextualTasksContextServiceFactory::GetInstance() {
  static base::NoDestructor<ContextualTasksContextServiceFactory> instance;
  return instance.get();
}

ContextualTasksContextServiceFactory::ContextualTasksContextServiceFactory()
    : ProfileKeyedServiceFactory(
          "ContextualTasksContextService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(
      page_content_annotations::PageEmbeddingsServiceFactory::GetInstance());
  DependsOn(OptimizationGuideKeyedServiceFactory::GetInstance());
  DependsOn(page_content_annotations::PageContentExtractionServiceFactory::
                GetInstance());
}

ContextualTasksContextServiceFactory::~ContextualTasksContextServiceFactory() =
    default;

std::unique_ptr<KeyedService>
ContextualTasksContextServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

}  // namespace contextual_tasks
