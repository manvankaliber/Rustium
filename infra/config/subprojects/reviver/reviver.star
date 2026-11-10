# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@chromium-luci//builders.star", "builder", "cpu", "defaults", "free_space", "os")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//dimensions.star", "dimensions")
load("@chromium-luci//polymorphic.star", "polymorphic")
load("//lib/ci_constants.star", "ci_constants")

luci.bucket(
    name = "reviver",
    acls = [
        acl.entry(
            roles = acl.BUILDBUCKET_READER,
            groups = "all",
        ),
        acl.entry(
            roles = acl.BUILDBUCKET_TRIGGERER,
            # TODO(crbug.com/40232487) Switch this to something more sensible once
            # the builders are verified
            users = [
                "reviver-builder@chops-service-accounts.iam.gserviceaccount.com",
            ],
        ),
        acl.entry(
            roles = acl.BUILDBUCKET_OWNER,
            groups = "project-chromium-admins",
        ),
    ],
)

consoles.list_view(
    name = "reviver",
)

defaults.set(
    bucket = "reviver",
    pool = ci_constants.DEFAULT_POOL,
    cores = 8,
    os = os.LINUX_DEFAULT,
    list_view = "reviver",
    experiments = {
        "chromium_tests.resultdb_module": 100,
    },
    service_account = "reviver-builder@chops-service-accounts.iam.gserviceaccount.com",
)

polymorphic.launcher(
    name = "linux-launcher",
    description_html = "Reviver launcher for Linux.",
    # To avoid peak hours, we run it at 5~11 UTC, 21~27 PST.
    schedule = "0 5-11/3 * * *",
    ssd = None,
    contact_team_email = "chrome-desktop-engprod@google.com",
    runner = "reviver/runner",
    target_builders = [
        polymorphic.target_builder(
            builder = "ci/Linux Builder",
            testers = [
                "ci/Linux Tests",
            ],
        ),
    ],
)

polymorphic.launcher(
    name = "win-launcher",
    description_html = "Reviver launcher for Windows.",
    # To avoid peak hours, we run it at 5~11 UTC, 21~27 PST.
    schedule = "0 5-11/3 * * *",
    ssd = None,
    contact_team_email = "chrome-desktop-engprod@google.com",
    runner = "reviver/runner",
    target_builders = [
        polymorphic.target_builder(
            builder = "ci/Win x64 Builder",
            dimensions = dimensions.dimensions(
                builderless = True,
                os = os.WINDOWS_DEFAULT,
                cpu = cpu.X86_64,
                free_space = free_space.standard,
            ),
            testers = [
                "ci/Win10 Tests x64",
            ],
        ),
    ],
)

polymorphic.launcher(
    name = "mac-launcher",
    description_html = "Reviver launcher for Mac.",
    # To avoid peak hours, we run it at 5~11 UTC, 21~27 PST.
    schedule = "0 5-11/3 * * *",
    ssd = None,
    contact_team_email = "chrome-desktop-engprod@google.com",
    runner = "reviver/runner",
    target_builders = [
        polymorphic.target_builder(
            builder = "ci/Mac Builder",
            dimensions = dimensions.dimensions(
                builderless = True,
                os = os.MAC_15,
                cpu = cpu.X86_64,
                ssd = True,
                free_space = free_space.standard,
            ),
            testers = [
                "ci/Mac13 Tests",
            ],
        ),
    ],
)

builder(
    name = "runner",
    description_html = "Runs disabled tests on target builder configurations for Test Reviver.",
    executable = "recipe:reviver/chromium/runner",
    pool = ci_constants.DEFAULT_POOL,
    builderless = 1,
    os = os.LINUX_DEFAULT,
    cpu = cpu.X86_64,
    ssd = None,
    free_space = free_space.standard,
    auto_builder_dimension = False,
    contact_team_email = "test-reviver@google.com",
    execution_timeout = 6 * time.hour,
    resultdb_bigquery_exports = [
        resultdb.export_test_results(
            bq_table = "chrome-luci-data.chromium.reviver_test_results",
        ),
    ],
    # TODO(crbug.com/40232487) Remove this once the reviver service account has
    # necessary permissions
    service_account = ci_constants.DEFAULT_SERVICE_ACCOUNT,
)

builder(
    name = "coverage-runner",
    description_html = "Runs disabled tests on target code coverage builder configurations for Test Reviver.",
    executable = "recipe:reviver/chromium/runner",
    pool = ci_constants.DEFAULT_POOL,
    builderless = 1,
    cores = 32,
    os = os.LINUX_DEFAULT,
    cpu = cpu.X86_64,
    ssd = True,
    free_space = free_space.standard,
    auto_builder_dimension = False,
    contact_team_email = "test-reviver@google.com",
    execution_timeout = 6 * time.hour,
    resultdb_bigquery_exports = [
        resultdb.export_test_results(
            bq_table = "chrome-luci-data.chromium.reviver_test_results",
        ),
    ],
    # TODO(crbug.com/40232487) Remove this once the reviver service account has
    # necessary permissions
    service_account = ci_constants.DEFAULT_SERVICE_ACCOUNT,
)
