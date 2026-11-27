#!/usr/bin/env vpython3
# Copyright 2012 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import functools
import io
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
import urllib.request
from unittest.mock import (
    ANY,
    Mock,
    MagicMock,
    mock_open,
    patch,
    call,
    PropertyMock,
)

bisect_builds = __import__('bisect-builds')

if 'NO_MOCK_SERVER' not in os.environ:
  maybe_patch = patch
else:
  # SetupEnvironment for gsutil to connect to real server.
  options = bisect_builds.ParseCommandLine(['-a', 'linux64', '-g', '1'])
  bisect_builds.SetupEnvironment(options)

  # Mock object that always wraps for the spec.
  # This will pass the call through and ignore the return_value and side_effect.
  class WrappedMock(MagicMock):

    def __init__(self,
                 spec=None,
                 return_value=None,
                 side_effect=None,
                 *args,
                 **kwargs):
      wraps = kwargs.pop('wraps', spec)
      super().__init__(spec, *args, **kwargs, wraps=wraps)

  maybe_patch = functools.partial(patch, spec=True, new_callable=WrappedMock)
  maybe_patch.object = functools.partial(patch.object,
                                         spec=True,
                                         new_callable=WrappedMock)


class BisectTestCase(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    # Patch the name pattern for pkgutil to accept "bisect-builds" as module
    # name.
    if sys.version_info[:2] > (3, 8):
      dotted_words = r'(?!\d)([\w-]+)(\.(?!\d)(\w+))*'
      name_pattern = re.compile(
          f'^(?P<pkg>{dotted_words})(?P<cln>:(?P<obj>{dotted_words})?)?$',
          re.UNICODE,
      )
      cls.name_pattern_patcher = patch('pkgutil._NAME_PATTERN', name_pattern)
      cls.name_pattern_patcher.start()

    # patch cache filename to prevent pollute working dir.
    fd, cls.tmp_cache_file = tempfile.mkstemp(suffix='.json')
    os.close(fd)
    cls.cache_filename_patcher = patch(
        'bisect-builds.ArchiveBuild._rev_list_cache_filename',
        new=PropertyMock(return_value=cls.tmp_cache_file),
    )
    cls.cache_filename_patcher.start()

  @classmethod
  def tearDownClass(cls):
    if sys.version_info[:2] > (3, 8):
      cls.name_pattern_patcher.stop()
    cls.cache_filename_patcher.stop()
    os.unlink(cls.tmp_cache_file)


class BisectTest(BisectTestCase):
  max_rev = 10000

  def setUp(self):
    self.patchers = []
    self.patchers.append(patch('bisect-builds.DownloadJob._fetch'))
    self.patchers.append(
        patch('bisect-builds.ArchiveBuild.run_revision',
              return_value=(0, '', '')))
    self.patchers.append(
        patch(
            'bisect-builds.SnapshotBuild._get_rev_list',
            return_value=range(self.max_rev),
        ))
    for each in self.patchers:
      each.start()

  def tearDown(self):
    for each in self.patchers:
      each.stop()

  def bisect(self, good_rev, bad_rev, evaluate, num_runs=1):
    options = bisect_builds.ParseCommandLine([
        '-a',
        'linux64',
        '-g',
        str(good_rev),
        '-b',
        str(bad_rev),
        '--times',
        str(num_runs),
        '--no-local-cache',
    ])
    archive_build = bisect_builds.create_archive_build(options)
    (minrev, maxrev) = bisect_builds.Bisect(archive_build=archive_build,
                                            evaluate=evaluate,
                                            try_args=options.args)
    return (minrev, maxrev)

  @patch('builtins.print')
  def testBisectConsistentAnswer(self, mock_print):

    def get_steps():
      steps = []
      for call in mock_print.call_args_list:
        if call.args and call.args[0].startswith('You have'):
          steps.append(int(re.search(r'(\d+) steps', call.args[0])[1]))
      return steps

    self.assertEqual(self.bisect(1000, 100, lambda *args: 'g'), (100, 101))
    self.assertSequenceEqual(get_steps(), range(10, 1, -1))

    mock_print.reset_mock()
    self.assertEqual(self.bisect(100, 1000, lambda *args: 'b'), (100, 101))
    self.assertSequenceEqual(get_steps(), range(10, 0, -1))

    mock_print.reset_mock()
    self.assertEqual(self.bisect(2000, 200, lambda *args: 'b'), (1999, 2000))
    self.assertSequenceEqual(get_steps(), range(11, 0, -1))

    mock_print.reset_mock()
    self.assertEqual(self.bisect(200, 2000, lambda *args: 'g'), (1999, 2000))
    self.assertSequenceEqual(get_steps(), range(11, 1, -1))

  @patch('bisect-builds.ArchiveBuild.run_revision', return_value=(0, '', ''))
  def test_bisect_should_retry(self, mock_run_revision):
    evaluator = Mock(side_effect='rgrgrbr')
    self.assertEqual(self.bisect(9, 1, evaluator), (2, 3))
    tested_revisions = [c.args[0] for c in evaluator.call_args_list]
    self.assertEqual(tested_revisions, [5, 5, 3, 3, 2, 2])
    self.assertEqual(mock_run_revision.call_count, 6)

    evaluator = Mock(side_effect='rgrrrgrbr')
    self.assertEqual(self.bisect(1, 10, evaluator), (8, 9))
    tested_revisions = [c.args[0] for c in evaluator.call_args_list]
    self.assertEqual(tested_revisions, [6, 6, 8, 8, 8, 8, 9, 9])

  def test_bisect_should_unknown(self):
    evaluator = Mock(side_effect='uuuggggg')
    self.assertEqual(self.bisect(9, 1, evaluator), (1, 2))
    tested_revisions = [c.args[0] for c in evaluator.call_args_list]
    self.assertEqual(tested_revisions, [5, 3, 6, 7, 2])

    evaluator = Mock(side_effect='uuugggggg')
    self.assertEqual(self.bisect(1, 9, evaluator), (8, 9))
    tested_revisions = [c.args[0] for c in evaluator.call_args_list]
    self.assertEqual(tested_revisions, [5, 7, 4, 3, 8])

  def test_bisect_should_quit(self):
    evaluator = Mock(side_effect=SystemExit())
    with self.assertRaises(SystemExit):
      self.assertEqual(self.bisect(9, 1, evaluator), (None, None))

  def test_edge_cases(self):
    with self.assertRaises(bisect_builds.BisectException):
      self.assertEqual(self.bisect(1, 1, Mock()), (1, 1))
    self.assertEqual(self.bisect(2, 1, Mock()), (1, 2))
    self.assertEqual(self.bisect(1, 2, Mock()), (1, 2))


class DownloadJobTest(BisectTestCase):

  @patch('bisect-builds.gsutil_download')
  def test_fetch_gsutil(self, mock_gsutil_download):
    fetch = bisect_builds.DownloadJob('gs://some-file.zip', 123)
    fetch.start()
    fetch.wait_for()
    mock_gsutil_download.assert_called_once()

  @patch('urllib.request.urlretrieve')
  def test_fetch_http(self, mock_urlretrieve):
    fetch = bisect_builds.DownloadJob('http://some-file.zip', 123)
    fetch.start()
    fetch.wait_for()
    mock_urlretrieve.assert_called_once()

  @patch('tempfile.mkstemp', return_value=(321, 'some-file.zip'))
  @patch('urllib.request.urlretrieve')
  @patch('os.close')
  @patch('os.unlink')
  def test_should_del(self, mock_unlink, mock_close, mock_urlretrieve,
                      mock_mkstemp):
    fetch = bisect_builds.DownloadJob('http://some-file.zip', 123)
    fetch.start().wait_for()
    fetch.stop()
    mock_mkstemp.assert_called_once()
    mock_close.assert_called_once()
    mock_urlretrieve.assert_called_once()
    mock_unlink.assert_called_with('some-file.zip')

  @patch('urllib.request.urlretrieve')
  def test_stop_wait_for_should_be_able_to_reenter(self, mock_urlretrieve):
    fetch = bisect_builds.DownloadJob('http://some-file.zip', 123)
    fetch.start()
    fetch.wait_for()
    fetch.wait_for()
    fetch.stop()
    fetch.stop()

  @patch(
      'tempfile.mkstemp',
      side_effect=[(321, 'some-file.zip'), (123, 'file2.zip')],
  )
  @patch('bisect-builds.gsutil_download')
  @patch('os.close')
  @patch('os.unlink')
  def test_should_support_multiple_files(self, mock_unlink, mock_close,
                                         mock_gsutil, mock_mkstemp):
    urls = {
        'browser': ('gs://chrome-unsigned/desktop-5c0tCh/129.0.6626.0/linux64/'
                    'chrome-linux64.zip'),
        'driver': ('gs://chrome-unsigned/desktop-5c0tCh/129.0.6626.0/linux64/'
                   'chromedriver_linux64.zip'),
    }
    fetch = bisect_builds.DownloadJob(urls, 123)
    result = fetch.start().wait_for()
    fetch.stop()
    self.assertDictEqual(
        result,
        {
            'browser': 'some-file.zip',
            'driver': 'file2.zip',
        },
    )
    self.assertEqual(mock_mkstemp.call_count, 2)
    self.assertEqual(mock_close.call_count, 2)
    mock_unlink.assert_has_calls(
        [call('some-file.zip'), call('file2.zip')], any_order=True)
    self.assertEqual(mock_gsutil.call_count, 2)

  @patch(
      "urllib.request.urlopen",
      side_effect=urllib.request.HTTPError('url', 404, 'Not Found', None, None),
  )
  @patch('subprocess.Popen', spec=subprocess.Popen)
  @patch('bisect-builds.GSUTILS_PATH', new='/some/path')
  def test_download_failure_should_raised(self, mock_Popen, mock_urlopen):
    fetch = bisect_builds.DownloadJob('http://some-file.zip', 123)
    with self.assertRaises(urllib.request.HTTPError):
      fetch.start().wait_for()

    mock_Popen.return_value.communicate.return_value = (b'', b'status=403')
    mock_Popen.return_value.returncode = 1
    fetch = bisect_builds.DownloadJob('gs://some-file.zip', 123)
    with self.assertRaises(bisect_builds.BisectException):
      fetch.start().wait_for()


class ArchiveBuildTest(BisectTestCase):

  def setUp(self):
    self.patcher = patch.multiple(
        bisect_builds.ArchiveBuild,
        __abstractmethods__=set(),
        build_type='release',
        _get_rev_list=Mock(return_value=list(map(str, range(10)))),
        _rev_list_cache_key='abc',
    )
    self.patcher.start()

  def tearDown(self):
    self.patcher.stop()

  def create_build(self, *args):
    args = ['-a', 'linux64', '-g', '0', '-b', '9', *args]
    options = bisect_builds.ParseCommandLine(args)
    return bisect_builds.ArchiveBuild(options)

  def test_cache_should_not_work_if_not_enabled(self):
    build = self.create_build('--no-local-cache')
    self.assertFalse(build.use_local_cache)
    with patch('builtins.open') as m:
      self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
      bisect_builds.ArchiveBuild._get_rev_list.assert_called_once()
      m.assert_not_called()

  def test_cache_should_save_and_load(self):
    build = self.create_build()
    self.assertTrue(build.use_local_cache)
    # Load the non-existent cache and write to it.
    cached_data = []
    # The cache file would be opened 3 times:
    #   1. read by _load_rev_list_cache
    #   2. read by _save_rev_list_cache for existing cache
    #   3. write by _save_rev_list_cache
    write_mock = MagicMock()
    write_mock.__enter__().write.side_effect = lambda d: cached_data.append(d)
    with patch(
        'builtins.open',
        side_effect=[FileNotFoundError, FileNotFoundError, write_mock],
    ):
      self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
      bisect_builds.ArchiveBuild._get_rev_list.assert_called_once()
    cached_json = json.loads(''.join(cached_data))
    self.assertDictEqual(cached_json, {'abc': [str(x) for x in range(10)]})
    # Load cache with cached data.
    build = self.create_build('--use-local-cache')
    bisect_builds.ArchiveBuild._get_rev_list.reset_mock()
    with patch('builtins.open', mock_open(read_data=''.join(cached_data))):
      self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
      bisect_builds.ArchiveBuild._get_rev_list.assert_not_called()

  @patch.object(bisect_builds.ArchiveBuild, '_load_rev_list_cache')
  @patch.object(bisect_builds.ArchiveBuild, '_save_rev_list_cache')
  @patch.object(
      bisect_builds.ArchiveBuild,
      '_get_rev_list',
      return_value=[str(x) for x in range(10)],
  )
  def test_should_request_partial_rev_list(self, mock_get_rev_list,
                                           mock_save_rev_list_cache,
                                           mock_load_rev_list_cache):
    build = self.create_build('--no-local-cache')
    # missing latest
    mock_load_rev_list_cache.return_value = [str(x) for x in range(5)]
    self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
    mock_get_rev_list.assert_called_with('4', '9')
    # missing old and latest
    mock_load_rev_list_cache.return_value = [str(x) for x in range(1, 5)]
    self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
    mock_get_rev_list.assert_called_with('0', '9')
    # missing old
    mock_load_rev_list_cache.return_value = [str(x) for x in range(3, 10)]
    self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
    mock_get_rev_list.assert_called_with('0', '3')
    # no intersect
    mock_load_rev_list_cache.return_value = ['c', 'd', 'e']
    self.assertEqual(build.get_rev_list(), [str(x) for x in range(10)])
    mock_save_rev_list_cache.assert_called_with([str(x) for x in range(10)] +
                                                ['c', 'd', 'e'])
    mock_get_rev_list.assert_called_with('0', 'c')

  @patch.object(bisect_builds.ArchiveBuild, '_get_rev_list', return_value=[])
  def test_should_raise_error_when_no_rev_list(self, mock_get_rev_list):
    build = self.create_build('--no-local-cache')
    with self.assertRaises(bisect_builds.BisectException):
      build.get_rev_list()
    mock_get_rev_list.assert_any_call('0', '9')
    mock_get_rev_list.assert_any_call()

  @unittest.skipIf(
      'NO_MOCK_SERVER' not in os.environ,
      'The test is to ensure NO_MOCK_SERVER working correctly',
  )
  @maybe_patch('bisect-builds.GetRevisionFromVersion', return_value=123)
  def test_no_mock(self, mock_GetRevisionFromVersion):
    self.assertEqual(bisect_builds.GetRevisionFromVersion('127.0.6533.74'),
                     1313161)
    mock_GetRevisionFromVersion.assert_called()

  @patch('bisect-builds.ArchiveBuild._install_revision')
  @patch('bisect-builds.ArchiveBuild._launch_revision',
         return_value=(1, '', ''))
  def test_run_revision_should_return_early(self, mock_launch_revision,
                                            mock_install_revision):
    build = self.create_build()
    build.run_revision('', '', [])
    mock_launch_revision.assert_called_once()

  @patch('bisect-builds.ArchiveBuild._install_revision')
  @patch('bisect-builds.ArchiveBuild._launch_revision',
         return_value=(0, '', ''))
  def test_run_revision_should_do_all_runs(self, mock_launch_revision,
                                           mock_install_revision):
    build = self.create_build('--time', '10')
    build.run_revision('', '', [])
    self.assertEqual(mock_launch_revision.call_count, 10)

  @patch('bisect-builds.UnzipFilenameToDir')
  @patch('glob.glob', return_value=['temp-dir/linux64/chrome'])
  @patch('os.path.abspath', return_value='/tmp/temp-dir/linux64/chrome')
  def test_install_revision_should_unzip_and_search_executable(
      self, mock_abspath, mock_glob, mock_UnzipFilenameToDir):
    build = self.create_build()
    self.assertEqual(
        build._install_revision('some-file.zip', 'temp-dir'),
        {'chrome': '/tmp/temp-dir/linux64/chrome'},
    )
    mock_UnzipFilenameToDir.assert_called_once_with('some-file.zip', 'temp-dir')
    mock_glob.assert_called_once_with('temp-dir/*/chrome')
    mock_abspath.assert_called_once_with('temp-dir/linux64/chrome')

  @patch('bisect-builds.UnzipFilenameToDir')
  @patch(
      'glob.glob',
      side_effect=[
          ['temp-dir/chrome-linux64/chrome'],
          ['temp-dir/chromedriver_linux64/chromedriver'],
      ],
  )
  @patch(
      'os.path.abspath',
      side_effect=[
          '/tmp/temp-dir/chrome-linux64/chrome',
          '/tmp/temp-dir/chromedriver_linux64/chromedriver',
      ],
  )
  def test_install_chromedriver(self, mock_abspath, mock_glob,
                                mock_UnzipFilenameToDir):
    build = self.create_build('--chromedriver')
    self.assertEqual(
        build._install_revision(
            {
                'chrome': 'some-file.zip',
                'chromedriver': 'some-other-file.zip',
            },
            'temp-dir',
        ),
        {
            'chrome': '/tmp/temp-dir/chrome-linux64/chrome',
            'chromedriver': '/tmp/temp-dir/chromedriver_linux64/chromedriver',
        },
    )
    mock_UnzipFilenameToDir.assert_has_calls([
        call('some-file.zip', 'temp-dir'),
        call('some-other-file.zip', 'temp-dir'),
    ])
    mock_glob.assert_has_calls([
        call('temp-dir/*/chrome'),
        call('temp-dir/*/chromedriver'),
    ])
    mock_abspath.assert_has_calls([
        call('temp-dir/chrome-linux64/chrome'),
        call('temp-dir/chromedriver_linux64/chromedriver'),
    ])

  @patch('subprocess.Popen', spec=subprocess.Popen)
  def test_launch_revision_should_run_command(self, mock_Popen):
    mock_Popen.return_value.communicate.return_value = ('', '')
    mock_Popen.return_value.returncode = 0
    build = self.create_build()
    build._launch_revision('temp-dir', {'chrome': 'temp-dir/linux64/chrome'},
                           [])
    mock_Popen.assert_called_once_with(
        'temp-dir/linux64/chrome --user-data-dir=temp-dir/profile',
        cwd=None,
        shell=True,
        bufsize=-1,
        stdout=ANY,
        stderr=ANY,
    )

  @unittest.skipIf(sys.platform.startswith('win'), 'This test is not for win')
  @patch('subprocess.Popen', spec=subprocess.Popen)
  def test_launch_revision_should_run_command_for_mac(self, mock_Popen):
    mock_Popen.return_value.communicate.return_value = ('', '')
    mock_Popen.return_value.returncode = 0
    build = self.create_build()
    build._launch_revision(
        'temp-dir',
        {
            'chrome':
            'temp-dir/full-build-mac/'
            'Google Chrome.app/Contents/MacOS/Google Chrome'
        },
        [],
    )
    mock_Popen.assert_called_once_with(
        "'temp-dir/full-build-mac/"
        "Google Chrome.app/Contents/MacOS/Google Chrome'"
        ' --user-data-dir=temp-dir/profile',
        cwd=None,
        shell=True,
        bufsize=-1,
        stdout=ANY,
        stderr=ANY,
    )

  @unittest.skipUnless(sys.platform.startswith('win'),
                       'This test is for win only')
  @patch('subprocess.Popen', spec=subprocess.Popen)
  def test_launch_revision_should_run_command_for_win(self, mock_Popen):
    mock_Popen.return_value.communicate.return_value = ('', '')
    mock_Popen.return_value.returncode = 0
    build = self.create_build()
    build._launch_revision(
        'C:\\temp-dir', {'chrome': 'C:\\temp-dir\\full-build-win\\chrome.exe'},
        [])
    mock_Popen.assert_called_once_with(
        "C:\\temp-dir\\full-build-win\\chrome.exe "
        '--user-data-dir=C:\\temp-dir/profile',
        cwd=None,
        shell=True,
        bufsize=-1,
        stdout=ANY,
        stderr=ANY,
    )

  @patch('subprocess.Popen', spec=subprocess.Popen)
  def test_command_replacement(self, mock_Popen):
    mock_Popen.return_value.communicate.return_value = ('', '')
    mock_Popen.return_value.returncode = 0
    build = self.create_build(
        '--chromedriver',
        '-c',
        'CHROMEDRIVER=%d BROWSER_EXECUTABLE_PATH=%p pytest %a',
    )
    build._launch_revision(
        '/tmp',
        {
            'chrome': '/tmp/chrome',
            'chromedriver': '/tmp/chromedriver'
        },
        ['--args', '--args2="word 1"', 'word 2'],
    )
    if sys.platform.startswith('win'):
      mock_Popen.assert_called_once_with(
          'CHROMEDRIVER=/tmp/chromedriver BROWSER_EXECUTABLE_PATH=/tmp/chrome '
          'pytest --user-data-dir=/tmp/profile --args "--args2=\\"word 1\\"" '
          '"word 2"',
          cwd=None,
          shell=True,
          bufsize=-1,
          stdout=ANY,
          stderr=ANY,
      )
    else:
      mock_Popen.assert_called_once_with(
          'CHROMEDRIVER=/tmp/chromedriver BROWSER_EXECUTABLE_PATH=/tmp/chrome '
          'pytest --user-data-dir=/tmp/profile --args \'--args2="word 1"\' '
          '\'word 2\'',
          cwd=None,
          shell=True,
          bufsize=-1,
          stdout=ANY,
          stderr=ANY,
      )


class ReleaseBuildTest(BisectTestCase):

  def test_should_look_up_path_context(self):
    options = bisect_builds.ParseCommandLine(
        ['-r', '-a', 'linux64', '-g', '127.0.6533.74', '-b', '127.0.6533.88'])
    self.assertEqual(options.archive, 'linux64')
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    self.assertEqual(build.binary_name, 'chrome')
    self.assertEqual(build.listing_platform_dir, 'linux64/')
    self.assertEqual(build.archive_name, 'chrome-linux64.zip')
    self.assertEqual(build.archive_extract_dir, 'chrome-linux64')

  @maybe_patch(
      'bisect-builds.GsutilList',
      return_value=[
          'gs://chrome-unsigned/desktop-5c0tCh/%s/linux64/chrome-linux64.zip' %
          x for x in ['127.0.6533.74', '127.0.6533.75', '127.0.6533.76']
      ],
  )
  def test_get_rev_list(self, mock_GsutilList):
    options = bisect_builds.ParseCommandLine([
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.76',
        '--no-local-cache',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    self.assertEqual(build.get_rev_list(),
                     ['127.0.6533.74', '127.0.6533.75', '127.0.6533.76'])
    mock_GsutilList.assert_any_call('gs://chrome-unsigned/desktop-5c0tCh')
    mock_GsutilList.assert_any_call(
        *[
            'gs://chrome-unsigned/desktop-5c0tCh/%s/linux64/chrome-linux64.zip'
            % x for x in ['127.0.6533.74', '127.0.6533.75', '127.0.6533.76']
        ],
        ignore_fail=True,
    )
    self.assertEqual(mock_GsutilList.call_count, 2)

  @patch(
      'bisect-builds.GsutilList',
      return_value=['127.0.6533.74', '127.0.6533.75', '127.0.6533.76'],
  )
  def test_should_save_and_load_cache(self, mock_GsutilList):
    options = bisect_builds.ParseCommandLine([
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.77',
        '--use-local-cache',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    # Load the non-existent cache and write to it.
    cached_data = []
    write_mock = MagicMock()
    write_mock.__enter__().write.side_effect = lambda d: cached_data.append(d)
    with patch(
        'builtins.open',
        side_effect=[FileNotFoundError, FileNotFoundError, write_mock],
    ):
      self.assertEqual(
          build.get_rev_list(),
          ['127.0.6533.74', '127.0.6533.75', '127.0.6533.76'],
      )
      mock_GsutilList.assert_called()
    cached_json = json.loads(''.join(cached_data))
    self.assertDictEqual(
        cached_json,
        {
            build._rev_list_cache_key: [
                '127.0.6533.74',
                '127.0.6533.75',
                '127.0.6533.76',
            ]
        },
    )
    # Load cache with cached data and merge with new data
    mock_GsutilList.return_value = ['127.0.6533.76', '127.0.6533.77']
    build = bisect_builds.create_archive_build(options)
    with patch('builtins.open', mock_open(read_data=''.join(cached_data))):
      self.assertEqual(
          build.get_rev_list(),
          ['127.0.6533.74', '127.0.6533.75', '127.0.6533.76', '127.0.6533.77'],
      )
    print(mock_GsutilList.call_args)
    mock_GsutilList.assert_any_call(
        'gs://chrome-unsigned/desktop-5c0tCh'
        '/127.0.6533.76/linux64/chrome-linux64.zip',
        'gs://chrome-unsigned/desktop-5c0tCh'
        '/127.0.6533.77/linux64/chrome-linux64.zip',
        ignore_fail=True,
    )

  def test_get_download_url(self):
    options = bisect_builds.ParseCommandLine(
        ['-r', '-a', 'linux64', '-g', '127.0.6533.74', '-b', '127.0.6533.77'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    download_urls = build.get_download_url('127.0.6533.74')
    self.assertEqual(
        download_urls,
        'gs://chrome-unsigned/desktop-5c0tCh'
        '/127.0.6533.74/linux64/chrome-linux64.zip',
    )

    options = bisect_builds.ParseCommandLine([
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.77',
        '--chromedriver',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    download_urls = build.get_download_url('127.0.6533.74')
    self.assertEqual(
        download_urls,
        {
            'chrome':
            'gs://chrome-unsigned/desktop-5c0tCh'
            '/127.0.6533.74/linux64/chrome-linux64.zip',
            'chromedriver':
            'gs://chrome-unsigned/desktop-5c0tCh'
            '/127.0.6533.74/linux64/chromedriver_linux64.zip',
        },
    )

  @unittest.skipUnless('NO_MOCK_SERVER' in os.environ,
                       'The test only valid when NO_MOCK_SERVER')
  @patch('bisect-builds.ArchiveBuild._run', return_value=(0, 'stdout', ''))
  def test_run_revision_with_real_zipfile(self, mock_run):
    options = bisect_builds.ParseCommandLine([
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.77',
        '--chromedriver',
        '-c',
        'driver=%d prog=%p',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ReleaseBuild)
    download_job = build.get_download_job('127.0.6533.74')
    zip_file = download_job.start().wait_for()
    with tempfile.TemporaryDirectory(prefix='bisect_tmp') as tempdir:
      build.run_revision(zip_file, tempdir, [])
    self.assertRegex(mock_run.call_args.args[0],
                     r'driver=.+/chromedriver prog=.+/chrome')


class ArchiveBuildWithCommitPositionTest(BisectTestCase):

  def setUp(self):
    patch.multiple(
        bisect_builds.ArchiveBuildWithCommitPosition,
        __abstractmethods__=set(),
        build_type='release',
    ).start()

  @maybe_patch('bisect-builds.GetRevisionFromVersion', return_value=1313161)
  @maybe_patch('bisect-builds.GetChromiumRevision', return_value=999999999)
  def test_should_convert_revision_as_commit_position(
      self, mock_GetChromiumRevision, mock_GetRevisionFromVersion):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '127.0.6533.74'])
    build = bisect_builds.ArchiveBuildWithCommitPosition(options)
    self.assertEqual(build.good_revision, 1313161)
    self.assertEqual(build.bad_revision, 999999999)
    mock_GetRevisionFromVersion.assert_called_once_with('127.0.6533.74')
    mock_GetChromiumRevision.assert_called()


class OfficialBuildTest(BisectTestCase):

  def test_should_lookup_path_context(self):
    options = bisect_builds.ParseCommandLine(
        ['-o', '-a', 'linux64', '-g', '0', '-b', '10'])
    self.assertEqual(options.archive, 'linux64')
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.OfficialBuild)
    self.assertEqual(build.binary_name, 'chrome')
    self.assertEqual(build.listing_platform_dir, 'linux-builder-perf/')
    self.assertEqual(build.archive_name, 'chrome-perf-linux.zip')
    self.assertEqual(build.archive_extract_dir, 'full-build-linux')

  @maybe_patch(
      'bisect-builds.GsutilList',
      return_value=[
          'full-build-linux_%d.zip' % x for x in range(1313161, 1313164)
      ],
  )
  def test_get_rev_list(self, mock_GsutilList):
    options = bisect_builds.ParseCommandLine([
        '-o',
        '-a',
        'linux64',
        '-g',
        '1313161',
        '-b',
        '1313163',
        '--no-local-cache',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.OfficialBuild)
    self.assertEqual(build.get_rev_list(), list(range(1313161, 1313164)))
    mock_GsutilList.assert_called_once_with(
        'gs://chrome-test-builds/official-by-commit/linux-builder-perf/')

  @unittest.skipUnless('NO_MOCK_SERVER' in os.environ,
                       'The test only valid when NO_MOCK_SERVER')
  @patch('bisect-builds.ArchiveBuild._run', return_value=(0, 'stdout', ''))
  def test_run_revision_with_real_zipfile(self, mock_run):
    options = bisect_builds.ParseCommandLine([
        '-o',
        '-a',
        'linux64',
        '-g',
        '1313161',
        '-b',
        '1313163',
        '--chromedriver',
        '-c',
        'driver=%d prog=%p',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.OfficialBuild)
    download_job = build.get_download_job(1334339)
    zip_file = download_job.start().wait_for()
    with tempfile.TemporaryDirectory(prefix='bisect_tmp') as tempdir:
      build.run_revision(zip_file, tempdir, [])
    self.assertRegex(mock_run.call_args.args[0],
                     r'driver=.+/chromedriver prog=.+/chrome')


class SnapshotBuildTest(BisectTestCase):

  def test_should_lookup_path_context(self):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '0', '-b', '10'])
    self.assertEqual(options.archive, 'linux64')
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    self.assertEqual(build.binary_name, 'chrome')
    self.assertEqual(build.listing_platform_dir, 'Linux_x64/')
    self.assertEqual(build.archive_name, 'chrome-linux.zip')
    self.assertEqual(build.archive_extract_dir, 'chrome-linux')

  CommonDataXMLContent = '''<?xml version='1.0' encoding='UTF-8'?>
    <ListBucketResult xmlns='http://doc.s3.amazonaws.com/2006-03-01'>
      <Name>chromium-browser-snapshots</Name>
      <Prefix>Linux_x64/</Prefix>
      <Marker></Marker>
      <NextMarker></NextMarker>
      <Delimiter>/</Delimiter>
      <IsTruncated>true</IsTruncated>
      <CommonPrefixes>
        <Prefix>Linux_x64/1313161/</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>Linux_x64/1313163/</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>Linux_x64/1313185/</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
  '''

  @maybe_patch(
      'urllib.request.urlopen',
      return_value=io.BytesIO(CommonDataXMLContent.encode('utf8')),
  )
  @patch('bisect-builds.GetChromiumRevision', return_value=1313185)
  def test_get_rev_list(self, mock_GetChromiumRevision, mock_urlopen):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '1313161', '-b', '1313185', '--no-local-cache'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    rev_list = build.get_rev_list()
    mock_urlopen.assert_any_call(
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots/'
        '?delimiter=/&prefix=Linux_x64/&marker=Linux_x64/1313161')
    self.assertEqual(mock_urlopen.call_count, 1)
    self.assertEqual(rev_list, [1313161, 1313163, 1313185])

  @patch(
      'bisect-builds.SnapshotBuild._fetch_and_parse',
      return_value=(
          [int(s) for s in sorted([str(x) for x in range(1, 11)])],
          None,
      ),
  )
  def test_get_rev_list_should_start_from_a_marker(self, mock_fetch_and_parse):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '0', '-b', '9', '--no-local-cache'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    rev_list = build._get_rev_list(0, 9)
    self.assertEqual(rev_list, list(range(1, 10)))
    mock_fetch_and_parse.assert_called_once_with(
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots/'
        '?delimiter=/&prefix=Linux_x64/&marker=Linux_x64/0')
    mock_fetch_and_parse.reset_mock()
    rev_list = build._get_rev_list(1, 9)
    self.assertEqual(rev_list, list(range(1, 10)))
    mock_fetch_and_parse.assert_called_once_with(
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots/'
        '?delimiter=/&prefix=Linux_x64/&marker=Linux_x64/1')

  @patch(
      'bisect-builds.SnapshotBuild._fetch_and_parse',
      return_value=(
          [int(s) for s in sorted([str(x) for x in range(1, 11)])],
          None,
      ),
  )
  def test_get_rev_list_should_scan_all_pages(self, mock_fetch_and_parse):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '3', '-b', '11', '--no-local-cache'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    rev_list = build._get_rev_list(0, 11)
    self.assertEqual(sorted(rev_list), list(range(1, 11)))
    mock_fetch_and_parse.assert_called_once_with(
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots/'
        '?delimiter=/&prefix=Linux_x64/')

  def test_get_download_url(self):
    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '3', '-b', '11'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    download_urls = build.get_download_url(123)
    self.assertEqual(
        download_urls,
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots'
        '/Linux_x64/123/chrome-linux.zip',
    )

    options = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '3', '-b', '11', '--chromedriver'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    download_urls = build.get_download_url(123)
    self.assertDictEqual(
        download_urls,
        {
            'chrome':
            'http://commondatastorage.googleapis.com/chromium-browser-snapshots'
            '/Linux_x64/123/chrome-linux.zip',
            'chromedriver':
            'http://commondatastorage.googleapis.com/chromium-browser-snapshots'
            '/Linux_x64/123/chromedriver_linux64.zip',
        },
    )

  @unittest.skipUnless('NO_MOCK_SERVER' in os.environ,
                       'The test only valid when NO_MOCK_SERVER')
  @patch('bisect-builds.ArchiveBuild._run', return_value=(0, 'stdout', ''))
  def test_run_revision_with_real_zipfile(self, mock_run):
    options = bisect_builds.ParseCommandLine([
        '-a',
        'linux64',
        '-g',
        '1313161',
        '-b',
        '1313185',
        '--chromedriver',
        '-c',
        'driver=%d prog=%p',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    download_job = build.get_download_job(1313161)
    zip_file = download_job.start().wait_for()
    with tempfile.TemporaryDirectory(prefix='bisect_tmp') as tempdir:
      build.run_revision(zip_file, tempdir, [])
    self.assertRegex(mock_run.call_args.args[0],
                     r'driver=.+/chromedriver prog=.+/chrome')

  @patch('bisect-builds.GetChromiumRevision', return_value=1313185)
  def test_get_bad_revision(self, mock_GetChromiumRevision):
    options = bisect_builds.ParseCommandLine(['-a', 'linux64', '-g', '1313161'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    mock_GetChromiumRevision.assert_called_once_with(
        'http://commondatastorage.googleapis.com/chromium-browser-snapshots'
        '/Linux_x64/LAST_CHANGE')
    self.assertEqual(build.bad_revision, 1313185)


class ChromeForTestingBuild(BisectTestCase):

  def test_should_lookup_path_context(self):
    options = bisect_builds.ParseCommandLine(
        ['-cft', '-a', 'linux64', '-g', '0', '-b', '10'])
    self.assertEqual(options.archive, 'linux64')
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    self.assertEqual(build.binary_name, 'chrome')
    self.assertEqual(build.listing_platform_dir, 'linux64/')
    self.assertEqual(build.archive_name, 'chrome-linux64.zip')
    self.assertEqual(build.chromedriver_binary_name, 'chromedriver')
    self.assertEqual(build.chromedriver_archive_name,
                     'chromedriver-linux64.zip')

  CommonDataXMLContent = '''<?xml version='1.0' encoding='UTF-8'?>
  <ListBucketResult xmlns="http://doc.s3.amazonaws.com/2006-03-01">
    <Name>chrome-for-testing-per-commit-public</Name>
    <Prefix>linux64/</Prefix>
    <Marker/>
    <Delimiter>/</Delimiter>
    <IsTruncated>false</IsTruncated>
    <Contents>
      <Key>linux64/LAST_CHANGE</Key>
      <Generation>1733959087133532</Generation>
      <MetaGeneration>1</MetaGeneration>
      <LastModified>2024-12-11T23:18:07.235Z</LastModified>
      <ETag>"dd60cb93e225ab33d7254beca56b507a"</ETag>
      <Size>7</Size>
    </Contents>
    <CommonPrefixes>
      <Prefix>linux64/r1390729/</Prefix>
    </CommonPrefixes>
    <CommonPrefixes>
      <Prefix>linux64/r1390746/</Prefix>
    </CommonPrefixes>
    <CommonPrefixes>
      <Prefix>linux64/r1390757/</Prefix>
    </CommonPrefixes>
  </ListBucketResult>
  '''

  @maybe_patch(
      'urllib.request.urlopen',
      return_value=io.BytesIO(CommonDataXMLContent.encode('utf8')),
  )
  @patch('bisect-builds.GetChromiumRevision', return_value=1390757)
  def test_get_rev_list(self, mock_GetChromiumRevision, mock_urlopen):
    options = bisect_builds.ParseCommandLine([
        '-cft',
        '-a',
        'linux64',
        '-g',
        '1390729',
        '-b',
        '1390757',
        '--no-local-cache',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    rev_list = build.get_rev_list()
    mock_urlopen.assert_any_call(
        'https://storage.googleapis.com/chrome-for-testing-per-commit-public/'
        '?delimiter=/&prefix=linux64/&marker=linux64/r1390729')
    self.assertEqual(mock_urlopen.call_count, 1)
    self.assertEqual(rev_list, [1390729, 1390746, 1390757])

  def test_get_marker(self):
    options = bisect_builds.ParseCommandLine(
        ['-cft', '-a', 'linux64', '-g', '1', '-b', '3'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    self.assertEqual('linux64/r1390729',
                     build._get_marker_for_revision(1390729))

  def test_get_download_url(self):
    options = bisect_builds.ParseCommandLine(
        ['-cft', '-a', 'linux64', '-g', '3', '-b', '11'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    download_urls = build.get_download_url(123)
    self.assertEqual(
        download_urls,
        'https://storage.googleapis.com/chrome-for-testing-per-commit-public'
        '/linux64/r123/chrome-linux64.zip',
    )

    options = bisect_builds.ParseCommandLine(
        ['-cft', '-a', 'linux64', '-g', '3', '-b', '11', '--chromedriver'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    download_urls = build.get_download_url(123)
    download_urls = build.get_download_url(123)
    self.assertEqual(
        download_urls,
        {
            'chrome':
            'https://storage.googleapis.com/chrome-for-testing-per-commit-public'
            '/linux64/r123/chrome-linux64.zip',
            'chromedriver':
            'https://storage.googleapis.com/chrome-for-testing-per-commit-public'
            '/linux64/r123/chromedriver-linux64.zip',
        },
    )

  @patch('bisect-builds.GetChromiumRevision', return_value=1390757)
  def test_get_bad_revision(self, mock_GetChromiumRevision):
    options = bisect_builds.ParseCommandLine(
        ['-cft', '-a', 'linux64', '-g', '3'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ChromeForTestingBuild)
    mock_GetChromiumRevision.assert_called_once_with(
        'https://storage.googleapis.com/chrome-for-testing-per-commit-public'
        '/linux64/LAST_CHANGE')
    self.assertEqual(build.bad_revision, 1390757)

  @unittest.skipUnless('NO_MOCK_SERVER' in os.environ,
                       'The test only valid when NO_MOCK_SERVER')
  @patch('bisect-builds.ArchiveBuild._run', return_value=(0, 'stdout', ''))
  def test_run_revision_with_real_zipfile(self, mock_run):
    options = bisect_builds.ParseCommandLine([
        '--cft',
        '-a',
        'linux64',
        '-g',
        '1',
        '--chromedriver',
        '-c',
        'driver=%d prog=%p',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.SnapshotBuild)
    download_job = build.get_download_job(build.bad_revision)
    zip_file = download_job.start().wait_for()
    with tempfile.TemporaryDirectory(prefix='bisect_tmp') as tempdir:
      build.run_revision(zip_file, tempdir, [])
    self.assertRegex(mock_run.call_args.args[0],
                     r'driver=.+/chromedriver prog=.+/chrome')


class ASANBuildTest(BisectTestCase):
  CommonDataXMLContent = '''<?xml version='1.0' encoding='UTF-8'?>
    <ListBucketResult xmlns='http://doc.s3.amazonaws.com/2006-03-01'>
      <Name>chromium-browser-asan</Name>
      <Prefix>mac-release/asan-mac-release</Prefix>
      <Marker></Marker>
      <NextMarker></NextMarker>
      <Delimiter>.zip</Delimiter>
      <IsTruncated>true</IsTruncated>
      <CommonPrefixes>
        <Prefix>mac-release/asan-mac-release-1313186.zip</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>mac-release/asan-mac-release-1313195.zip</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>mac-release/asan-mac-release-1313210.zip</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
  '''

  @maybe_patch(
      'urllib.request.urlopen',
      return_value=io.BytesIO(CommonDataXMLContent.encode('utf8')),
  )
  def test_get_rev_list(self, mock_urlopen):
    options = bisect_builds.ParseCommandLine([
        '--asan',
        '-a',
        'mac',
        '-g',
        '1313161',
        '-b',
        '1313210',
        '--no-local-cache',
    ])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.ASANBuild)
    rev_list = build.get_rev_list()
    # print(mock_urlopen.call_args_list)
    mock_urlopen.assert_any_call(
        'http://commondatastorage.googleapis.com/chromium-browser-asan/'
        '?delimiter=.zip&prefix=mac-release/asan-mac-release'
        '&marker=mac-release/asan-mac-release-1313161.zip')
    self.assertEqual(mock_urlopen.call_count, 1)
    self.assertEqual(rev_list, [1313186, 1313195, 1313210])


class LinuxReleaseBuildTest(BisectTestCase):

  @patch('subprocess.Popen', spec=subprocess.Popen)
  def test_launch_revision_should_has_no_sandbox(self, mock_Popen):
    mock_Popen.return_value.communicate.return_value = ('', '')
    mock_Popen.return_value.returncode = 0
    options = bisect_builds.ParseCommandLine(
        ['-r', '-a', 'linux64', '-g', '127.0.6533.74', '-b', '127.0.6533.88'])
    build = bisect_builds.create_archive_build(options)
    self.assertIsInstance(build, bisect_builds.LinuxReleaseBuild)
    build._launch_revision('temp-dir', {'chrome': 'temp-dir/linux64/chrome'},
                           [])
    mock_Popen.assert_called_once_with(
        'temp-dir/linux64/chrome --user-data-dir=temp-dir/profile --no-sandbox',
        cwd=None,
        shell=True,
        bufsize=-1,
        stdout=ANY,
        stderr=ANY,
    )


class MaybeSwitchBuildTypeTest(BisectTestCase):

  def test_generate_new_command_without_cache(self):
    command_line = [
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.88',
        '--no-local-cache',
    ]
    options = bisect_builds.ParseCommandLine(command_line)
    with patch('sys.argv', ['bisect-builds.py', *command_line]):
      new_cmd = bisect_builds.MaybeSwitchBuildType(
          options,
          bisect_builds.ChromiumVersion('127.0.6533.74'),
          bisect_builds.ChromiumVersion('127.0.6533.88'),
      )
      self.assertEqual(
          new_cmd[1:],
          [
              '-o',
              '-a',
              'linux64',
              '-g',
              '127.0.6533.74',
              '-b',
              '127.0.6533.88',
              '--verify-range',
              '--no-local-cache',
          ],
      )

  def test_no_official_build(self):
    command_line = [
        '-r',
        '-a',
        'mac64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.78',
        '--no-local-cache',
    ]
    options = bisect_builds.ParseCommandLine(command_line)
    with patch('sys.argv', ['bisect-builds.py', *command_line]):
      new_cmd = bisect_builds.MaybeSwitchBuildType(options, '127.0.6533.74',
                                                   '127.0.6533.88')
      self.assertEqual(new_cmd, None)

  @patch('bisect-builds.ArchiveBuild.get_rev_list', return_value=list(range(3)))
  def test_generate_suggestion_with_cache(self, mock_get_rev_list):
    command_line = [
        '-r',
        '-a',
        'linux64',
        '-g',
        '127.0.6533.74',
        '-b',
        '127.0.6533.88',
        '--use-local-cache',
    ]
    options = bisect_builds.ParseCommandLine(command_line)
    with patch('sys.argv', ['bisect-builds.py', *command_line]):
      new_cmd = bisect_builds.MaybeSwitchBuildType(options, '127.0.6533.74',
                                                   '127.0.6533.88')
      self.assertEqual(
          new_cmd[1:],
          [
              '-o',
              '-a',
              'linux64',
              '-g',
              '127.0.6533.74',
              '-b',
              '127.0.6533.88',
              '--verify-range',
              '--use-local-cache',
          ],
      )
      mock_get_rev_list.assert_called()


class MethodTest(BisectTestCase):

  @patch('sys.stderr', new_callable=io.StringIO)
  def test_ParseCommandLine(self, mock_stderr):
    opts = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '1', 'args1', 'args2 3', '-b', '2'])
    self.assertEqual(opts.build_type, 'snapshot')
    self.assertEqual(opts.args, ['args1', 'args2 3'])

    opts = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '1', 'args1', 'args2 3'])
    self.assertEqual(opts.args, ['args1', 'args2 3'])

    opts = bisect_builds.ParseCommandLine(
        ['-a', 'linux64', '-g', '1', '--', 'args1', 'args2 3', '-b', '2'])
    self.assertEqual(opts.args, ['args1', 'args2 3', '-b', '2'])

    with self.assertRaises(SystemExit):
      bisect_builds.ParseCommandLine(['-a', 'mac64', '-o', '-g', '1'])
      self.assertRegexpMatches(
          mock_stderr.getvalue(),
          r'To bisect for mac64, please choose from '
          r'release(-r), snapshot(-s)',
      )

  @patch('bisect-builds._DetectArchive', return_value='linux64')
  def test_ParseCommandLine_DetectArchive(self, mock_detect_archive):
    opts = bisect_builds.ParseCommandLine(['-o', '-g', '1'])
    self.assertEqual(opts.archive, 'linux64')

  @patch('sys.stderr', new_callable=io.StringIO)
  def test_ParseCommandLine_apk_error(self, mock_stderr):
    with self.assertRaises(SystemExit):
      bisect_builds.ParseCommandLine(
          ['-a', 'linux64', '--apk', 'chrome', '-g', '1'])
    self.assertIn('unrecognized arguments: --apk', mock_stderr.getvalue())

  @patch('sys.stderr', new_callable=io.StringIO)
  def test_ParseCommandLine_ipa_error(self, mock_stderr):
    with self.assertRaises(SystemExit):
      bisect_builds.ParseCommandLine(
          ['-a', 'linux64', '--ipa', 'stable', '-g', '1'])
    self.assertIn('unrecognized arguments: --ipa', mock_stderr.getvalue())

  @patch('sys.stderr', new_callable=io.StringIO)
  def test_ParseCommandLine_signed_error(self, mock_stderr):
    with self.assertRaises(SystemExit):
      bisect_builds.ParseCommandLine(['-a', 'linux64', '--signed', '-g', '1'])
    self.assertIn('unrecognized arguments: --signed', mock_stderr.getvalue())

  @patch('urllib.request.urlopen')
  @patch('builtins.open')
  @patch('sys.stdout', new_callable=io.StringIO)
  def test_update_script(self, mock_stdout, mock_open, mock_urlopen):
    mock_urlopen.return_value = io.BytesIO(
        base64.b64encode('content'.encode('utf-8')))
    with self.assertRaises(SystemExit):
      bisect_builds.ParseCommandLine(['--update-script'])
    mock_urlopen.assert_called_once_with(
        'https://chromium.googlesource.com/chromium/src/+/HEAD/'
        'tools/bisect-builds.py?format=TEXT')
    mock_open.assert_called_once()
    mock_open.return_value.__enter__().write.assert_called_once_with('content')
    self.assertEqual(mock_stdout.getvalue(), 'Update successful!\n')

  @patch(
      "urllib.request.urlopen",
      side_effect=[
          urllib.request.HTTPError('url', 404, 'Not Found', None, None),
          urllib.request.HTTPError('url', 404, 'Not Found', None, None),
          io.BytesIO(b"NOT_A_JSON"),
          io.BytesIO(b'{"chromium_main_branch_position": 123}'),
      ],
  )
  def test_GetRevisionFromVersion(self, mock_urlopen):
    self.assertEqual(123,
                     bisect_builds.GetRevisionFromVersion('127.0.6533.134'))
    mock_urlopen.assert_has_calls([
        call('https://chromiumdash.appspot.com/fetch_version'
             '?version=127.0.6533.134'),
        call(
            'https://chromiumdash.appspot.com/fetch_version?version=127.0.6533.0'
        ),
    ])

  @maybe_patch(
      "urllib.request.urlopen",
      side_effect=[
          io.BytesIO(b'{"chromium_main_branch_position": null}'),
          io.BytesIO(b'{"message": "DEP\\n"}'),
          io.BytesIO(b')]}\'\n{"message": "Cr-Branched-From: '
                     b'3d60439cfb36485e76a1c5bb7f513d3721b20da1-'
                     b'refs/heads/master@{#870763}\\n"}'),
      ],
  )
  def test_GetRevisionFromSourceTag(self, mock_urlopen):
    self.assertEqual(870763,
                     bisect_builds.GetRevisionFromVersion('91.0.4472.38'))
    mock_urlopen.assert_has_calls([
        call(
            'https://chromiumdash.appspot.com/fetch_version?version=91.0.4472.38'
        ),
        call('https://chromium.googlesource.com/chromium/src/'
             '+/refs/tags/91.0.4472.38?format=JSON'),
        call('https://chromium.googlesource.com/chromium/src/'
             '+/refs/tags/91.0.4472.38^?format=JSON'),
    ])

  def test_join_args(self):
    test_data = ['a', 'b c', 'C:\\a b\\c', '/a b/c', '"a"', "'a'"]
    quoted_command = bisect_builds.join_args(
        [sys.executable, '-c', 'import sys, json; print(json.dumps(sys.argv))'
         ] + test_data)

    subproc = subprocess.Popen(quoted_command,
                               shell=True,
                               stdout=subprocess.PIPE)
    stdout, _ = subproc.communicate()
    dumped_argv = json.loads(stdout.decode('utf-8'))

    self.assertListEqual(dumped_argv, ['-c'] + test_data)


class ChromiumVersionTest(BisectTestCase):

  def test_cmpare_version_numbers(self):
    v127_0_6533_74 = bisect_builds.ChromiumVersion('127.0.6533.74')
    v127_0_6533_75 = bisect_builds.ChromiumVersion('127.0.6533.75')
    v127_0_6533_75_with_space = bisect_builds.ChromiumVersion('127.0.6533.75 ')
    v127 = bisect_builds.ChromiumVersion('127')

    self.assertLess(v127_0_6533_74, v127_0_6533_75)
    self.assertLessEqual(v127_0_6533_74, v127_0_6533_75)
    self.assertGreater(v127_0_6533_75, v127_0_6533_74)
    self.assertGreaterEqual(v127_0_6533_75, v127_0_6533_74)
    self.assertEqual(v127_0_6533_75, v127_0_6533_75_with_space)
    self.assertLess(v127, v127_0_6533_74)


class GetEarliestBuildVersionFromRevisionTest(BisectTestCase):

  @patch('bisect-builds.FetchJsonFromURL')
  def test_get_build_versions_from_revision(self, mock_fetch_json):
    mock_fetch_json.return_value = {'commits': [{'earliest': '144.0.7502.0'}]}
    version = bisect_builds.GetEarliestBuildVersionFromRevision(1537356)
    self.assertIsNotNone(version)
    self.assertEqual(str(version), '144.0.7502.0')

  @patch('bisect-builds.FetchJsonFromURL')
  def test_get_build_versions_from_revision_multiple_commits(
      self, mock_fetch_json):
    mock_fetch_json.return_value = {
        'commits': [{
            'earliest': '144.0.7559.3'
        }, {
            'earliest': '144.0.7502.0'
        }]
    }
    with self.assertRaises(bisect_builds.BisectException):
      bisect_builds.GetEarliestBuildVersionFromRevision(1537356)

  @patch('bisect-builds.FetchJsonFromURL')
  def test_get_build_versions_from_revision_no_commits(self, mock_fetch_json):
    mock_fetch_json.return_value = {'commits': []}
    with self.assertRaises(bisect_builds.BisectException):
      bisect_builds.GetEarliestBuildVersionFromRevision(1537356)

  @patch('bisect-builds.FetchJsonFromURL')
  def test_get_build_versions_from_revision_no_earliest(self, mock_fetch_json):
    mock_fetch_json.return_value = {'commits': [{}]}
    with self.assertRaises(bisect_builds.BisectException):
      bisect_builds.GetEarliestBuildVersionFromRevision(1537356)

  @patch('bisect-builds.FetchJsonFromURL')
  def test_get_build_versions_from_revision_non_integer_revision(
      self, mock_fetch_json):
    version = bisect_builds.GetEarliestBuildVersionFromRevision('not-a-number')
    self.assertIsNone(version)
    mock_fetch_json.assert_not_called()


if __name__ == '__main__':
  unittest.main()
