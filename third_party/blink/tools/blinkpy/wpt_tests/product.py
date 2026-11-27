# Copyright 2018 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Product classes that encapsulate the interfaces for the testing targets"""

import argparse
import contextlib
import functools
import logging
from typing import List

from blinkpy.common import path_finder
from blinkpy.common.memoized import memoized
from blinkpy.web_tests.port.base import Port

_log = logging.getLogger(__name__)




@memoized
def make_product_registry():
    """Create a mapping from all product names (including aliases) to their
    respective classes.
    """
    product_registry = {}
    product_classes = [
        Chrome,
        HeadlessShell,
    ]
    for product_cls in product_classes:
        names = [product_cls.name] + product_cls.aliases
        product_registry.update((name, product_cls) for name in names)
    return product_registry


class Product:
    """A product (browser or browser component) that can run web platform tests.

    Attributes:
        name (str): The official wpt-accepted name of this product.
        aliases (list[str]): Human-friendly aliases for the official name.
    """

    name = ''
    aliases = []

    def __init__(self, port: Port, options):
        self._port = port
        self._host = port.host
        self._options = options
        self._tasks = contextlib.ExitStack()

    @contextlib.contextmanager
    def test_env(self):
        """Set up and clean up the test environment."""
        with self._tasks:
            yield

    def update_runner_options(self, options: argparse.Namespace):
        options.processes = self.processes
        options.browser_version = self.get_version()
        if self._options.stable:
            options.channel = 'stable'
            # Don't use a local trunk build of `chromedriver`, whose major
            # version might be ahead of stable. Instead, download a compatible
            # `chromedriver` version from Chrome for Testing.
            options.install_webdriver = True
            options.yes = True
        else:
            options.webdriver_binary = (
                self._options.webdriver_binary or self.webdriver_binary
            )
        options.webdriver_args.extend(self.additional_webdriver_args())

    @functools.cached_property
    def processes(self) -> int:
        if self._options.child_processes:
            return self._options.child_processes
        elif self._options.wrapper:
            _log.info(
                'Defaulting to 1 worker because of debugging option `--wrapper`'
            )
            return 1
        else:
            return self._port.default_child_processes()

    def additional_webdriver_args(self):
        """Additional webdriver parameters for the product"""
        return []

    def get_version(self):
        """Get the product version, if available."""
        return None

    @property
    def webdriver_binary(self):
        if self._host.platform.is_win():
            path = 'chromedriver.exe'
        else:
            path = 'chromedriver'  # linux and mac
        return self._port.build_path(path)


class DesktopProduct(Product):
    def update_runner_options(self, options: argparse.Namespace):
        super().update_runner_options(options)
        options.binary = self._port.path_to_driver()
        options.binary_args.extend(self.additional_binary_args())

    def additional_binary_args(self) -> List[str]:
        # Base args applicable to all embedders.
        args = [
            # Expose the non-standard `window.gc()` for `wpt_internal/` tests.
            '--js-flags=--expose-gc',
            # Disable overlay scrollbar fadeout for consistent screenshots.
            '--disable-features=ScrollbarAnimations',
            # Keep focused carets visible for deterministic screenshots.
            '--disable-blink-features=CaretBlinking',
            # Allow headless embedders to use SwiftShader for WebGL/WebGPU.
            '--enable-unsafe-swiftshader',
        ]
        fs = self._host.filesystem
        if (
            self._options.wrapper
            and fs.basename(self._options.wrapper[0]) == 'rr'
        ):
            debug_args = [
                '--no-sandbox',
                '--disable-hang-monitor',
            ]
            args.extend(debug_args)
            _log.info(
                f'Running {self.name!r} with {" ".join(debug_args)!r} '
                'because of debugging option `--wrapper=rr`'
            )
        if self._options.wrapper and self._host.platform.is_win():
            # The adapter will generate a batch file wrapping the browser
            # command. Because `cmd.exe` doesn't have an equivalent of Unix's
            # `exec`, there will be a real process in between chromedriver and
            # the browser process. Because the batch file doesn't know how to
            # relay the file handles it receives to the browser, the default
            # `--remote-debugging-pipe` won't work. Use any free network port
            # instead for chromedriver-browser traffic.
            args.append('--remote-debugging-port=0')
        return args


class Chrome(DesktopProduct):
    name = 'chrome'


class HeadlessShell(DesktopProduct):
    name = 'headless_shell'

    def additional_binary_args(self):
        rv = [
            *super().additional_binary_args(),
            "--canvas-2d-layers",
            '--enable-bfcache',
            '--enable-field-trial-config',
            '--force-reporting-destination-attested',
            # `headless_shell` doesn't send the `Accept-Language` header by
            # default, so set an arbitrary one that some tests expect.
            '--accept-lang=en-US,en',
        ]
        if self._port.operating_system() != 'linux':
            rv.append('--disable-site-isolation-trials')
        return rv
