#!/usr/bin/env python3
#
# Copyright 2025 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This file contains a couple dictionaries of useful GN args we need in various
# scripts. Providing a central location to get all gn args, or only the
# platforms you are interested in, and to configure them as required.

import os
import subprocess


class GnConfigsImpl:
    # TODO: Consider switching to enum.Flag rather than a bool.
    def __init__(self, use_remoteexec) -> None:
        self.remoteexec_args = [
            'use_remoteexec=true',
            'use_siso=true',
        ]
        self.localexec_args = [
            'use_remoteexec=false',
            'use_siso=true',
        ]
        current_exec = (self.remoteexec_args
                        if use_remoteexec else self.localexec_args)

        # TODO: Consider switching to straight defining a YAML file or json
        # struct to separate code from data more completely.
        self.linux_configs = {}
        # CQ bots:
        # https://ci.chromium.org/ui/p/chromium/builders/try/linux-rel
        # Note: For other CQ bots simply replace "linux-rel" with the name.
        self.linux_configs["linux-rel"] = [
            'dcheck_always_on=true',
            'ffmpeg_branding="Chrome"',
            'is_component_build=false',
            'is_debug=false',
            'proprietary_codecs=true',
            'symbol_level=0',
            'target_cpu="x64"',
            'target_os="linux"',
            'use_clang_coverage=false',
        ] + current_exec
        self.linux_configs["linux-chromium-asan"] = [
            'dcheck_always_on=true',
            'fail_on_san_warnings=true',
            'is_asan=true',
            'is_component_build=false',
            'is_debug=false',
            'is_lsan=true',
            'symbol_level=1',
            'target_cpu="x64"',
            'target_os="linux"',
        ] + current_exec
        self.linux_configs["linux-libfuzzer-asan-rel"] = [
            'dcheck_always_on=true',
            'enable_mojom_fuzzer=true',
            'ffmpeg_branding="ChromeOS"',
            'generate_fuzzer_owners=false',
            'is_asan=true',
            'is_component_build=true',
            'is_debug=false',
            'optimize_for_fuzzing=true',
            'pdf_enable_xfa=true',
            'proprietary_codecs=true',
            'symbol_level=0',
            'target_cpu="x64"',
            'target_os="linux"',
            'use_libfuzzer=true',
        ] + current_exec
        self.linux_configs["linux-chromium-tsan-rel-ng"] = [
            'dcheck_always_on=true',
            'fail_on_san_warnings=true',
            'is_component_build=false',
            'is_debug=false',
            'is_tsan=true',
            'symbol_level=1',
            'target_cpu="x64"',
            'target_os="linux"',
        ] + current_exec
        # Finally Official linux builder args
        # See https://ci.chromium.org/ui/p/chrome/builders/official/linux64
        self.linux_configs["linux-official"] = [
            'dcheck_always_on=true',
            'is_chrome_branded=true',
            'is_debug=false',
            'is_official_build=true',
            'rtc_use_pipewire=true',
            # Added to make it explicit.
            'target_os="linux"',
        ] + current_exec

        self.mac_configs = {}
        # CQ bots
        # https://ci.chromium.org/ui/p/chromium/builders/try/mac-rel
        # Note: For other CQ bots simply replace "mac-rel" with the name.
        # Note: you can get to the args from there but it is a bit difficult you
        # can also look at:
        # infra/config/generated/builders/try/mac-rel/gn-args.json
        # To save compile time we've merged mac-rel with mac-official
        self.mac_configs["mac-rel"] = [
            # mac-rel
            # "coverage_instrumentation_input_file" removed didn't exist locally
            'dcheck_always_on=true',
            'enable_backup_ref_ptr_feature_flag=true',
            'enable_dangling_raw_ptr_checks=true',
            'enable_dangling_raw_ptr_feature_flag=true',
            'ffmpeg_branding="Chrome"',
            'is_component_build=false',
            'is_debug=false',
            'proprietary_codecs=true',
            'symbol_level=0',
            'target_cpu="x64"',
            'target_os="mac"',
            'use_clang_coverage=false',
            # Below are args needed by mac-official
            # 'is_chrome_branded=true', Doesn't build locally.
            # 'is_official_build=true', Only needed for optimizations
            'save_reproducers_on_lld_crash=true',
            # Set to false, although it is True on official bots this requires
            # extra set up so remove here.
            'ignore_missing_widevine_signing_cert=true',
            # See crbug.com/40249290 needed for cross compiling
            'enable_dsyms=false',
            # See (internal only): yaqs/4233495799914299392 also needed
            'enable_stripping=false',
        ] + current_exec
        self.win_configs = {}
        # CQ bot
        # See infra/config/generated/builders/try/win-rel/gn-args.json
        self.win_configs['win-rel'] = [
            # Coverage file is removed.
            'dcheck_always_on=true',
            'enable_dangling_raw_ptr_checks=true',
            'enable_dangling_raw_ptr_feature_flag=true',
            'enable_resource_allowlist_generation=false',
            'ffmpeg_branding="Chrome"',
            'is_component_build=false',
            'is_debug=false',
            'proprietary_codecs=true',
            'symbol_level=0',
            'target_cpu="x64"',
            'target_os="win"',
            'use_clang_coverage=false',
        ] + current_exec
        self.win_configs['win_chromium_compile_dbg_ng'] = [
            'ffmpeg_branding="Chrome"',
            'is_component_build=true',
            'is_debug=true',
            'proprietary_codecs=true',
            'symbol_level=0',
            'target_cpu="x86"',
            'target_os="win"',
        ] + current_exec

        # Project specific configs for standalone builds.
        # TODO(crbug.com/497912213): Add more platforms and configs for these.
        root_dir = os.path.abspath(
            os.path.join(os.path.dirname(__file__), '../../..'))
        clang_path = os.path.join(root_dir,
                                  'third_party/llvm-build/Release+Asserts/bin')
        self.skia_configs = {
            'linux': [
                'is_debug=false',
                'is_component_build=false',
                'target_cpu="x64"',
                'target_os="linux"',
                f'cc="{os.path.join(clang_path, "clang")}"',
                f'cxx="{os.path.join(clang_path, "clang++")}"',
            ]
        }

        clang_base_path = os.path.join(
            root_dir, 'third_party/llvm-build/Release+Asserts')
        self.dawn_configs = {
            'linux': [
                'is_debug=false',
                'is_component_build=false',
                'symbol_level=0',
                'target_cpu="x64"',
                'target_os="linux"',
            ] + current_exec
        }
        self.angle_configs = {
            'linux': [
                'is_debug=false',
                'is_component_build=false',
                'symbol_level=0',
                'target_cpu="x64"',
                'target_os="linux"',
                'build_with_chromium=false',
            ] + current_exec
        }
        self.webrtc_configs = {
            'linux': [
                'is_debug=false',
                'is_component_build=false',
                'symbol_level=0',
                'target_cpu="x64"',
                'target_os="linux"',
                'build_with_chromium=false',
            ] + current_exec
        }

        # A set of platforms that gives you minimum coverage.
        self.min_all_platforms = {}
        self.min_all_platforms['linux'] = self.linux_configs['linux-rel']
        self.min_all_platforms['mac'] = self.mac_configs['mac-rel']
        self.min_all_platforms['win'] = self.win_configs['win-rel']

        # A set of platforms that should give you reasonable CQ coverage.
        self.all_platforms_and_configs = (self.linux_configs
                                          | self.mac_configs
                                          | self.win_configs)

    def get_config(self, platform, project='chrome'):
        """
        Returns the GN arguments for a given platform and project.
        """
        if project == 'chrome' or project == 'partition_alloc':
            # Use standard chrome configs for these.
            args = self.min_all_platforms.get(platform)
            if args is None:
                args = self.all_platforms_and_configs.get(platform)
            return args

        # Check for project-specific configs.
        project_configs_key = '%s_configs' % project
        if hasattr(self, project_configs_key):
            project_configs = getattr(self, project_configs_key)
            return project_configs.get(platform)

        return None

    # Make it simple to get a certain config or all platform configs if you only
    # care about a single one.
    def __getitem__(self, key):
        # First check if it's a project name.
        potential_platform_key = '%s_configs' % key
        if hasattr(self, potential_platform_key):
            return getattr(self, potential_platform_key)

        # Then check if it's a specific config name (e.g., 'linux-rel').
        for name, config in self.all_platforms_and_configs.items():
            if key == name:
                return config

        # Finally, check if it's a platform name in min_all_platforms.
        return self.min_all_platforms.get(key)


# To prevent initializing it repeatedly if people call it as a simple accessor.
# TODO: Consider just switching to a global object already initialized.
_remote_exec_config = None
_local_exec_config = None


# This function returns the config and ensures it only gets initialized once per
# python execution.
#
# It declares the following member variable dicts:
#  * linux_configs (accessible through ['linux']
#  * mac_configs (again accessible)
#  * win_configs (again accessible)
#  * all_platforms_and_configs -> This is all of the configs defined above.
#  * min_all_platforms -> This is a single representative config from each
#    platform listed above.
#
# In addition if know the name of the config you want you can access it directly
# through GnConfigs()['linux-rel'] for example. This returns an array of args
# used by the 'linux-rel' CQ bot.
#
# In general all dicts are a mapping of CQ (or official builder) to an array of
# GN args. You can provide this key, args list to GenerateGnTarget(key, args)
# Which will create the directory and ensure the args are properly updated.
def GnConfigs(use_remoteexec) -> GnConfigsImpl:
    # Needed to not create a local variable in the assignment.
    global _remote_exec_config, _local_exec_config
    if use_remoteexec:
        if _remote_exec_config is None:
            _remote_exec_config = GnConfigsImpl(True)
        return _remote_exec_config

    if _local_exec_config is None:
        _local_exec_config = GnConfigsImpl(False)
    return _local_exec_config


# This function will generate out/<target> to use <args> this is the same as
# running `gn args` yourself and manually entering <args>.
def GenerateGnTarget(target, args, cwd=None):
    # Configure the target.
    cmd = ['gn', 'gen', f'out/{target}', f'--args={" ".join(args)}']
    try:
        result = subprocess.run(cmd, check=True, text=True, cwd=cwd)
        return result.returncode == 0
    except subprocess.CalledProcessError:
        return False
