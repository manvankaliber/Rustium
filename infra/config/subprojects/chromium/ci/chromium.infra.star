# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Definitions of builders in the chromium.infra builder group."""

load("@chromium-luci//branches.star", "branches")
load("@chromium-luci//builder_health_indicators.star", "health_spec")
load("@chromium-luci//builders.star", "cpu", "os")
load("@chromium-luci//ci.star", "ci")
load("@chromium-luci//consoles.star", "consoles")
load("//lib/ci_constants.star", "ci_constants")
load("//lib/gardener_rotations.star", "gardener_rotations")

ci.defaults.set(
    builder_group = "chromium.infra",
    pool = ci_constants.DEFAULT_POOL,
    cores = 8,
    os = os.LINUX_DEFAULT,
    execution_timeout = ci_constants.DEFAULT_EXECUTION_TIMEOUT,
    health_spec = health_spec.default(),
    service_account = ci_constants.DEFAULT_SERVICE_ACCOUNT,
    shadow_service_account = ci_constants.DEFAULT_SHADOW_SERVICE_ACCOUNT,
)

consoles.console_view(
    name = "chromium.infra",
)

def packager_builder(**kwargs):
    return ci.builder(
        service_account = "chromium-cipd-builder@chops-service-accounts.iam.gserviceaccount.com",
        shadow_service_account = "chromium-cipd-try-builder@chops-service-accounts.iam.gserviceaccount.com",
        **kwargs
    )

packager_builder(
    name = "3pp-linux-amd64-packager",
    description_html = "Chromium 3PP packager on Linux AMD64 platform.",
    executable = "recipe:chromium_3pp",
    # Every 6 hours starting at 5am UTC.
    schedule = "0 5/6 * * * *",
    triggered_by = [],
    pool = "luci.chromium.provenance.ci",
    builderless = False,
    console_view_entry = consoles.console_view_entry(
        category = "packager|3pp|linux",
        short_name = "amd64",
    ),
    execution_timeout = 7 * time.hour,
    notifies = ["chromium-infra"],
    properties = {
        "$build/chromium_3pp": {
            "platform": "linux-amd64",
            "gclient_config": "chromium",
            "gclient_apply_config": [],
        },
    },
)

packager_builder(
    name = "3pp-mac-amd64-packager",
    description_html = "Chromium 3PP packager on Mac AMD64 platform.",
    executable = "recipe:chromium_3pp",
    # Every 6 hours starting at 5am UTC.
    schedule = "0 5/6 * * * *",
    triggered_by = [],
    builderless = True,
    cores = None,
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    console_view_entry = consoles.console_view_entry(
        category = "packager|3pp|mac",
        short_name = "amd64",
    ),
    notifies = ["chromium-infra"],
    properties = {
        "$build/chromium_3pp": {
            "platform": "mac-amd64",
            "gclient_config": "chromium",
        },
    },
)

packager_builder(
    name = "3pp-mac-arm64-packager",
    description_html = "chromium 3pp packager on Mac ARM64 platform.",
    executable = "recipe:chromium_3pp",
    # Every 6 hours starting at 5am UTC.
    schedule = "0 5/6 * * * *",
    triggered_by = [],
    builderless = True,
    cores = None,
    os = os.MAC_DEFAULT,
    cpu = cpu.ARM64,
    console_view_entry = consoles.console_view_entry(
        category = "packager|3pp|mac",
        short_name = "arm64",
    ),
    contact_team_email = "clank-engprod@google.com",
    notifies = ["chromium-infra"],
    properties = {
        "$build/chromium_3pp": {
            "platform": "mac-arm64",
            "gclient_config": "chromium",
        },
    },
)

packager_builder(
    name = "3pp-windows-amd64-packager",
    description_html = "3PP Packager for Windows",
    executable = "recipe:chromium_3pp",
    # Every 6 hours starting at 5am UTC.
    schedule = "0 5/6 * * * *",
    triggered_by = [],
    builderless = True,
    cores = None,
    os = os.WINDOWS_DEFAULT,
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "packager|3pp|win",
        short_name = "amd64",
    ),
    contact_team_email = "chrome-browser-infra-team@google.com",
    notifies = ["chromium-infra"],
    properties = {
        "$build/chromium_3pp": {
            "platform": "windows-amd64",
            "gclient_config": "chromium",
        },
    },
)

packager_builder(
    name = "rts-model-packager",
    description_html = "Builds and packages the Regression Test Selection (RTS) model daily.",
    executable = "recipe:chromium_rts/create_model",
    schedule = "0 9 * * *",  # at 1AM or 2AM PT (depending on DST), once a day.
    triggered_by = [],
    builderless = False,
    cores = None,
    console_view_entry = consoles.console_view_entry(
        category = "packager|rts",
        short_name = "create-model",
    ),
    contact_team_email = "chrome-test-infra-mx@google.com",
    execution_timeout = 10 * time.hour,
    notifies = [
        luci.notifier(
            name = "rts-model-packager-notifier",
            notify_emails = ["chrome-test-infra-mx+alerts@google.com"],
            on_occurrence = ["FAILURE", "INFRA_FAILURE"],
        ),
    ],
)

ci.builder(
    name = "rts-suite-analysis",
    description_html = "Analyzes test suite execution data for Regression Test Selection (RTS).",
    executable = "recipe:chromium_rts/rts_analyze",
    schedule = "0 9 * * *",  # at 1AM or 2AM PT (depending on DST), once a day.
    triggered_by = [],
    builderless = False,
    cores = None,
    console_view_entry = consoles.console_view_entry(
        category = "analysis|rts",
        short_name = "suite-analysis",
    ),
    execution_timeout = 10 * time.hour,
    service_account = "chromium-cipd-builder@chops-service-accounts.iam.gserviceaccount.com",
)

luci.bucket(
    name = "ssci",
    acls = [
        acl.entry(
            roles = [
                acl.BUILDBUCKET_READER,
                acl.SCHEDULER_READER,
            ],
            groups = "all",
        ),
        acl.entry(
            roles = [acl.SCHEDULER_OWNER],
            groups = ["mdb/chrome-troopers", "mdb/chops-security-oncallers"],
        ),
    ],
)

ci.builder(
    name = "chromium-ssci-linux-amd64",
    # TODO(b/464370790): Move this to CI bucket when recipe development is complete.
    bucket = "ssci",
    description_html = "Triggers Crowbar workflows on chromium/src.",
    executable = "recipe:chromium/crowbar",
    # TODO(b/464370790): Make this a routine job (regular update) and CI
    # triggered job (after submitting a CL modifying Crowbar spec) in the prod
    # pool when recipe development is complete.
    schedule = "triggered",
    triggered_by = [],
    pool = "luci.chromium.provenance.ci",
    builderless = False,
    cores = None,
    os = os.LINUX_ANY,
    console_view_entry = consoles.console_view_entry(
        category = "packager|crowbar",
        short_name = "ssci",
    ),
    contact_team_email = "chops-security-core@google.com",
    properties = {
        "crowbar_role": "ROLE_CI_BUILDER",
    },
    service_account = "chromium-roller@chops-crowbar.iam.gserviceaccount.com",
    shadow_pool = None,
    shadow_service_account = None,
)
