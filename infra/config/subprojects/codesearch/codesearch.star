# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@chromium-luci//builders.star", "cpu", "os")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//gn_args.star", "gn_args")
load("@chromium-luci//try.star", "try_")
load("//lib/siso.star", "siso")

luci.bucket(
    name = "codesearch",
    acls = [
        acl.entry(
            roles = acl.BUILDBUCKET_READER,
            groups = "all",
        ),
        acl.entry(
            roles = acl.BUILDBUCKET_TRIGGERER,
            groups = "project-chromium-tryjob-access",
        ),
    ],
)

try_.defaults.set(
    bucket = "codesearch",
    executable = "recipe:chromium_codesearch",
    builder_group = "tryserver.chromium.codesearch",
    pool = "luci.chromium.try",
    builderless = True,
    cores = 8,
    os = os.LINUX_DEFAULT,
    cpu = cpu.X86_64,
    build_numbers = True,
    contact_team_email = "chops-source-team@google.com",
    cq_group = "cq",
    execution_timeout = 9 * time.hour,
    expiration_timeout = 2 * time.hour,
    service_account = "chromium-try-builder@chops-service-accounts.iam.gserviceaccount.com",
    siso_project = siso.project.DEFAULT_UNTRUSTED,
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

consoles.list_view(
    name = "tryserver.chromium.codesearch",
)

try_.builder(
    name = "gen-linux-try",
    description_html = "Trybuilder for generating Code Search Kythe index packs and generated files for Linux.",
    gn_args = gn_args.config(
        configs = [
            "codesearch_builder",
            "clang",
            "debug_builder",
            "minimal_symbols",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    ssd = True,
)

try_.builder(
    name = "gen-mac-try",
    description_html = "Trybuilder for generating Code Search Kythe index packs and generated files for Mac.",
    gn_args = gn_args.config(
        configs = [
            "codesearch_builder",
            "clang",
            "debug_builder",
            "minimal_symbols",
            "remoteexec",
            "mac",
            "arm64",
        ],
    ),
    cores = None,
    os = os.MAC_DEFAULT,
    cpu = cpu.ARM64,
    properties = {
        "recipe_properties": {
            "build_config": "mac",
            "platform": "mac",
        },
    },
)

try_.builder(
    name = "gen-win-try",
    description_html = "Trybuilder for generating Code Search Kythe index packs and generated files for Windows.",
    gn_args = gn_args.config(
        configs = [
            "codesearch_builder",
            "clang",
            "debug_builder",
            "minimal_symbols",
            "remoteexec",
            "win",
            "x64",
        ],
    ),
    os = os.WINDOWS_10,
    properties = {
        "recipe_properties": {
            "platform": "win",
        },
    },
)
