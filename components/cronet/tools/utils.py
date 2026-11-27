#!/usr/bin/env python3
#
# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Contains general-purpose methods that can be used to execute shell,
GN and Ninja commands.
"""

import shlex
import subprocess
import os
import re
import pathlib
import difflib
from typing import Set, List

REPOSITORY_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))

GN_PATH = os.path.join(REPOSITORY_ROOT, 'buildtools/linux64/gn')
NINJA_PATH = os.path.join(REPOSITORY_ROOT, 'third_party/ninja/ninja')
_GN_ARG_MATCHER = re.compile("^.*=.*$")
# The current value of 500 is a heuristic that seems to work. If command
# line length limitation is exceeded, reduce this number.
_MAX_TARGETS_PER_NINJA_EXECUTION = 500


def build_targets_list_chunking(out_path: str, targets: List[str]) -> None:
    """Builds the provided targets by chunking them and passing each chunk into GN. This is
    generally faster than building each target separately. However, the |chunk_size| must be
    tweaked carefully to avoid exceeding the command-line length.

    Args:
      out_path: GN output path
      targets: List of targets to build.
    """
    # Split the build script actions into chunk of _MAX_TARGETS_PER_NINJA_EXECUTION.
    # This is needed in order not to exceed the command-line length.
    build_script_actions_chunks = [
        targets[i:i + _MAX_TARGETS_PER_NINJA_EXECUTION]
        for i in range(0, len(targets), _MAX_TARGETS_PER_NINJA_EXECUTION)
    ]
    for chunk in build_script_actions_chunks:
        build_all(out_path, chunk)


def run(command, verbose=True, **kwargs):
    """See the official documentation for subprocess.check_call.

    Args:
      command (list[str]): command to be executed
    """
    if kwargs.get("shell"):
        quoted_cmd = command
    else:
        quoted_cmd = ' '.join(shlex.quote(arg) for arg in command)
    if verbose:
        print('Executing: ' + quoted_cmd)
    subprocess.check_call(command, **kwargs)


def run_and_get_stdout(command, verbose=True, **kwargs):
    """See the official documentation for subprocess.run.

    Args:
      command (list[str]): command to be executed

    Returns:
      str: stdout for the executed command
    """
    if verbose:
        print('Executing: ' + ' '.join(shlex.quote(arg) for arg in command))
    return (subprocess.run(command, capture_output=True, check=True,
                           **kwargs).stdout.decode('utf-8').strip())


def gn(out_dir, gn_args, gn_extra=None, **kwargs):
    """Executes `gn gen`.

    Runs `gn gen |out_dir| |gn_args + gn_extra|` which will generate
    a GN configuration that lives under |out_dir|. This is done
    locally on the same chromium checkout.

    Args:
      out_dir (str): Path to delegate to `gn gen`.
      gn_args (str): Args as a string delimited by space.
      gn_extra (str): extra args as a string delimited by space.
    """
    cmd = [GN_PATH, 'gen', out_dir, '--args=%s' % gn_args]
    if gn_extra:
        cmd += gn_extra
    run(cmd, **kwargs)


def compare_text_and_generate_diff(generated_text, golden_text,
                                   golden_file_path):
    """
    Compares the generated text with the golden text.

    returns a diff that can be applied with `patch` if exists.
    """
    golden_lines = [line.rstrip() for line in golden_text.splitlines()]
    generated_lines = [line.rstrip() for line in generated_text.splitlines()]
    if golden_lines == generated_lines:
        return None

    expected_path = os.path.relpath(golden_file_path, REPOSITORY_ROOT)

    diff = difflib.unified_diff(
        golden_lines,
        generated_lines,
        fromfile=os.path.join('before', expected_path),
        tofile=os.path.join('after', expected_path),
        n=0,
        lineterm='',
    )

    return '\n'.join(diff)


def read_file(path):
    """Reads a file as a string"""
    return pathlib.Path(path).read_text()


def write_file(path, contents):
    """Writes contents to a file, creating parent directories if they don't exist"""
    path_obj = pathlib.Path(path)
    path_obj.parent.mkdir(parents=True, exist_ok=True)
    return path_obj.write_text(contents)


def build(out_dir, build_target, extra_options=None):
    """Runs `ninja build`.

    Runs `ninja -C |out_dir| |build_target| |extra_options|` which will build
    the target |build_target| for the GN configuration living under |out_dir|.
    This is done locally on the same chromium checkout.
    """
    cmd = [NINJA_PATH, '-C', out_dir, build_target]
    if extra_options:
        cmd += extra_options
    run(cmd)


def build_all(out_dir, build_targets, extra_options=None):
    """Runs `ninja build`.

    Runs `ninja -C |out_dir| |build_targets| |extra_options|` which will build
    the targets |build_targets| for the GN configuration living under |out_dir|.
    This is done locally on the same chromium checkout.
    """
    cmd = [NINJA_PATH, '-C', out_dir]
    cmd.extend(build_targets)
    if extra_options:
        cmd += extra_options
    run(cmd)


def get_transitive_deps_build_files(repo_path: str, out_dir: str,
                                    gn_targets: List[str]) -> Set[str]:
    """Executes gn desc |out_dir| |gn_target| deps --all --as=buildfile for each gn target"""
    all_deps = set()
    for gn_target in gn_targets:
        all_deps.update(
            subprocess.check_output([
                GN_PATH,
                "desc",
                out_dir,
                gn_target,
                "deps",
                "--all",
                "--as=buildfile",
            ]).decode("utf-8").split("\n"))
        # gn desc deps does not return the build file that includes the target
        # which we want to find its transitive dependencies, in order to
        # account for this corner case, the BUILD file for the current target
        # is added manually.
        all_deps.add(
            f"{os.path.join(repo_path, gn_target[2 : gn_target.find(':')])}/BUILD.gn"
        )
    # It seems that we always get an empty string as part of the output. This
    # could happen if we get an empty line in the output which can happen so
    # let's remove that so downstream consumers don't have to check for it.
    all_deps.remove('')
    return all_deps


def get_path_from_gn_label(gn_label: str) -> str:
    """Returns the path part from a GN Label

    GN label consist of two parts, path and target_name, this will
    remove the target name and return the path or throw an error
    if it can't remove the target_name or if it doesn't exist.
    """
    if ":" not in gn_label:
        raise ValueError(f"Provided gn label {gn_label} is not a proper label")
    return gn_label[:gn_label.find(":")]


def _should_remove_arg(arg, keys):
    """An arg is removed if its key appear in the list of |keys|"""
    return arg.split("=")[0].strip() in keys


def filter_gn_args(gn_args, keys_to_remove):
    """Returns a list of filtered GN args.

    (1) GN arg's returned must match the regex |_GN_ARG_MATCHER|.
    (2) GN arg's key must not be in |keys_to_remove|.

    Args:
      gn_args: list of GN args.
      keys_to_remove: List of string that will be removed from gn_args.
    """
    filtered_args = []
    for arg in gn_args:
        if _GN_ARG_MATCHER.match(arg) and not _should_remove_arg(
                arg, keys_to_remove):
            filtered_args.append(arg)
    return filtered_args
