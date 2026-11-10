# Copyright 2014 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark
from core import platforms

from telemetry import benchmark
from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement

import page_sets

# TODO(rnephew): Revisit the re-enabled benchmarks on Wed, Aug 8 2017.


class _MediaBenchmark(perf_benchmark.PerfBenchmark):
  """Base class for TBMv2-based desktop media benchmarks."""

  def CreateCoreTimelineBasedMeasurementOptions(self):
    category_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter()

    # 'toplevel' category provides CPU time slices used by # cpuTimeMetric.
    category_filter.AddIncludedCategory('toplevel')

    # Collect media related events required by mediaMetric.
    category_filter.AddIncludedCategory('media')

    # Collect memory data.
    category_filter.AddDisabledByDefault('disabled-by-default-memory-infra')

    options = timeline_based_measurement.Options(category_filter)

    # Setting an empty memory dump config disables periodic dumps.
    options.config.chrome_trace_config.SetMemoryDumpConfig(
        chrome_trace_config.MemoryDumpConfig())

    # Note that memoryMetric is added using GetExtraTracingMetrics() for
    # certain stories.
    options.SetTimelineBasedMetrics(['mediaMetric', 'cpuTimeMetric'])
    return options

  def SetExtraBrowserOptions(self, options):
    # bf-cache messes with the time_to_play numbers when we do several runs
    # in a row. More info crbug.com/1309294
    options.AppendExtraBrowserArgs('--disable-features=BackForwardCache')


@benchmark.Info(
    emails=['dalecurtis@chromium.org'],
    component='Internals>Media',
    documentation_url=(
        'https://chromium.googlesource.com/chromium/src/+/main/docs/speed/'
        'benchmark/harnesses/media.md'),
)
class MediaDesktop(_MediaBenchmark):
  """Obtains media performance for key user scenarios on desktop."""

  # TODO(johnchen): Remove either the SUPPORTED_PLATFORMS or
  # SUPPORTED_PLATFORMS_TAGS lists. Only one is necessary.
  SUPPORTED_PLATFORM_TAGS = [platforms.DESKTOP]
  SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]

  def CreateStorySet(self, options):
    return page_sets.MediaCasesDesktopStorySet()

  @classmethod
  def Name(cls):
    return 'media.desktop'
