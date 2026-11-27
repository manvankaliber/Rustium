"""Protect paired baselines and launch affinity using small test-only children."""

import ctypes
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import run_rust_rewrite_benchmarks as runner


class RangeReferenceTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.http = self.root / "net/http"
        self.http.mkdir(parents=True)

    def source(self, filename, text):
        (self.http / filename).write_text(text, encoding="utf-8")

    def test_migrated_public_value_cannot_be_a_cpp_baseline_dependency(self):
        self.source("http_byte_range.cc", '#include "net/http/http_byte_range_rust_ffi.h"\n')
        self.source("http_range_promote_reference.h", '#include "net/http/http_byte_range.h"\n')
        with self.assertRaisesRegex(ValueError, "before timing"):
            runner.check_range_reference(self.root)

    def test_original_public_value_is_allowed_for_historical_cpp_runs(self):
        self.source("http_byte_range.cc", '#include "net/http/http_byte_range.h"\n')
        self.source("http_range_promote_reference.h", '#include "net/http/http_byte_range.h"\n')
        record = runner.check_range_reference(self.root)
        self.assertFalse(record["public_byte_range_is_rust_backed"])

    def test_independent_value_header_allows_parent_review_to_proceed(self):
        self.source("http_byte_range.cc", '#include "net/http/http_byte_range_rust_ffi.h"\n')
        self.source("http_range_promote_reference.h", '#include "net/http/http_byte_range_rust_reference.h"\n')
        record = runner.check_range_reference(self.root)
        self.assertTrue(record["public_byte_range_is_rust_backed"])
        self.assertIn("Does not replace", record["scope"])


@unittest.skipUnless(os.name == "nt", "Windows suspended-launch integration tests")
class WindowsAffinityTest(unittest.TestCase):
    # This is the first application action after Python imports/ctypes setup,
    # not a claim to observe the Windows loader or Python interpreter startup.
    PROBE = r'''
import ctypes, json, os, sys
api = ctypes.WinDLL("kernel32", use_last_error=True)
api.GetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]
api.GetProcessAffinityMask.restype = ctypes.c_int
process_mask, system_mask = ctypes.c_size_t(), ctypes.c_size_t()
if not api.GetProcessAffinityMask(ctypes.c_void_p(-1), ctypes.byref(process_mask), ctypes.byref(system_mask)):
    raise ctypes.WinError(ctypes.get_last_error())
print(json.dumps({"mask": process_mask.value, "pid": os.getpid(), "argv": sys.argv[1:],
                  "cwd": os.getcwd(), "environment": os.environ.get("AFFINITY_TEST_VALUE")}), flush=True)
sys.stderr.buffer.write(b"test stderr\x00\xff\r\n")
sys.stderr.buffer.flush()
if os.environ.get("AFFINITY_TEST_WAIT"):
    import time
    time.sleep(60)  # The test kills this owned child immediately after its ready line.
sys.exit(int(os.environ.get("AFFINITY_TEST_EXIT", "0")))
'''

    def setUp(self):
        import _winapi

        self.winapi = _winapi
        self.api = runner._WindowsAffinityAPI()
        self.parent_mask, _ = self.api.get_affinity(-1)
        # Count does not establish mask membership. Leave CPU4 free, prefer CPU6.
        supported = [cpu for cpu in range(ctypes.sizeof(ctypes.c_size_t) * 8)
                     if self.parent_mask & (1 << cpu) and cpu != 4]
        if not supported:
            self.skipTest("No available CPU other than reserved CPU4")
        self.cpu = 6 if 6 in supported else supported[0]
        self.directory = tempfile.TemporaryDirectory(prefix="benchmark affinity ")
        self.addCleanup(self.directory.cleanup)
        self.addCleanup(self.assert_parent_unchanged)
        self.root = Path(self.directory.name)
        self.probe = self.root / "child probe.py"
        self.probe.write_text(self.PROBE, encoding="utf-8")
        self.stdout = self.root / "stdout.jsonl"
        self.command = [sys.executable, "-I", "-u", str(self.probe),
                        "--timing", "space argument", 'quote"and\\slash', ""]
        self.env = os.environ.copy()
        self.env["AFFINITY_TEST_VALUE"] = "preserved value"

    def assert_parent_unchanged(self):
        self.assertEqual(self.api.get_affinity(-1)[0], self.parent_mask)

    def run_child(self):
        with self.stdout.open("wb") as stream:
            return runner.run_benchmark(self.command, cwd=self.root, env=self.env,
                                        stdout=stream, cpu=self.cpu)

    def retain_owned_handle(self, process_handle):
        current = self.winapi.GetCurrentProcess()
        handle = self.winapi.DuplicateHandle(current, process_handle, current, 0, False,
                                             self.winapi.DUPLICATE_SAME_ACCESS)
        self.addCleanup(self.winapi.CloseHandle, handle)
        return handle

    def assert_reaped(self, handle):
        self.assertEqual(self.winapi.WaitForSingleObject(handle, 0), self.winapi.WAIT_OBJECT_0)
        self.assertNotEqual(self.winapi.GetExitCodeProcess(handle), 259)  # STILL_ACTIVE

    def test_first_action_affinity_exact_arguments_io_cwd_and_environment(self):
        code, errors, affinity = self.run_child()
        child = json.loads(self.stdout.read_bytes())
        self.assertEqual(code, 0)
        self.assertEqual(errors, b"test stderr\x00\xff\r\n")
        self.assertEqual(child["mask"], 1 << self.cpu)
        self.assertEqual(child["argv"], self.command[4:])
        self.assertEqual(Path(child["cwd"]), self.root)
        self.assertEqual(child["environment"], "preserved value")
        self.assertEqual(affinity["requested_mask"], child["mask"])
        self.assertEqual(affinity["verified_process_mask"], child["mask"])
        self.assertTrue(affinity["verified_before_resume"])
        self.assertEqual(affinity["previous_suspend_count"], 1)
        self.assertGreater(affinity["primary_thread_id"], 0)
        self.assertEqual(affinity["process_id"], child["pid"])
        self.assertIn("GetProcessAffinityMask", affinity["method"])

    def test_fast_nonzero_exit_is_propagated_with_stdout_and_stderr(self):
        self.env["AFFINITY_TEST_EXIT"] = "23"
        code, errors, _ = self.run_child()
        self.assertEqual(code, 23)
        self.assertEqual(json.loads(self.stdout.read_bytes())["mask"], 1 << self.cpu)
        self.assertEqual(errors, b"test stderr\x00\xff\r\n")

    def check_setup_failure(self, method, side_effect, error_type, pattern=None):
        handles = []
        real_set = runner._WindowsAffinityAPI.set_affinity

        def observe(api, handle, mask):
            handles.append(self.retain_owned_handle(handle))
            return real_set(api, handle, mask)

        with mock.patch.object(runner._WindowsAffinityAPI, "set_affinity", observe):
            with mock.patch.object(runner._WindowsAffinityAPI, method, side_effect=side_effect):
                if pattern:
                    with self.assertRaisesRegex(error_type, pattern):
                        self.run_child()
                else:
                    with self.assertRaises(error_type):
                        self.run_child()
        self.assertEqual(len(handles), 1)
        self.assert_reaped(handles[0])
        self.assertEqual(self.stdout.read_bytes(), b"")

    def test_real_set_failure_reaps_suspended_child_without_running_it(self):
        handles = []
        real_set = runner._WindowsAffinityAPI.set_affinity

        def invalid_mask(api, handle, mask):
            handles.append(self.retain_owned_handle(handle))
            return real_set(api, handle, 0)  # Actual Win32 error on this owned child.

        with mock.patch.object(runner._WindowsAffinityAPI, "set_affinity", invalid_mask):
            with self.assertRaises(OSError):
                self.run_child()
        self.assertEqual(len(handles), 1)
        self.assert_reaped(handles[0])
        self.assertEqual(self.stdout.read_bytes(), b"")

    def test_query_failure_reaps_suspended_child(self):
        real_query = runner._WindowsAffinityAPI.get_affinity
        self.check_setup_failure("get_affinity", lambda handle: real_query(self.api, 0), OSError)

    def test_observed_mismatch_reaps_without_resuming(self):
        self.check_setup_failure("get_affinity", lambda handle: (0, self.parent_mask),
                                 RuntimeError, "affinity mismatch")

    def test_resume_failure_reaps_suspended_child(self):
        real_resume = runner._WindowsAffinityAPI.resume_thread
        self.check_setup_failure("resume_thread", lambda handle: real_resume(self.api, 0), OSError)

    def test_unexpected_resume_count_is_rejected(self):
        for count in (0, 2):
            with self.subTest(count=count):
                api = runner._WindowsAffinityAPI.__new__(runner._WindowsAffinityAPI)
                api.kernel32 = mock.Mock()
                api.kernel32.ResumeThread.return_value = count
                with self.assertRaisesRegex(RuntimeError, "unexpected suspend count"):
                    api.resume_thread(123)

    def test_cancellation_during_setup_reaps_suspended_child(self):
        self.check_setup_failure("get_affinity", KeyboardInterrupt(), KeyboardInterrupt)

    def test_cancellation_before_handle_adoption_reaps_suspended_child(self):
        handles = []
        real_create = runner._WindowsAffinityAPI.create_process
        real_close_pipes = runner._WindowsAffinityPopen._close_pipe_fds

        def create(api, *args):
            real_create(api, *args)
            handles.append(self.retain_owned_handle(args[-1].hProcess))

        def close_then_cancel(process, *handles):
            real_close_pipes(process, *handles)
            raise KeyboardInterrupt()

        with mock.patch.object(runner._WindowsAffinityAPI, "create_process", create), \
                mock.patch.object(runner._WindowsAffinityPopen, "_close_pipe_fds", close_then_cancel):
            with self.assertRaises(KeyboardInterrupt):
                self.run_child()
        self.assertEqual(len(handles), 1)
        self.assert_reaped(handles[0])
        self.assertEqual(self.stdout.read_bytes(), b"")

    def test_cancellation_at_create_return_reaps_suspended_child(self):
        handles = []
        real_create = runner._WindowsAffinityAPI.create_process

        def create_then_cancel(api, *args):
            real_create(api, *args)
            handles.append(self.retain_owned_handle(args[-1].hProcess))
            raise KeyboardInterrupt()

        with mock.patch.object(runner._WindowsAffinityAPI, "create_process", create_then_cancel):
            with self.assertRaises(KeyboardInterrupt):
                self.run_child()
        self.assertEqual(len(handles), 1)
        self.assert_reaped(handles[0])
        self.assertEqual(self.stdout.read_bytes(), b"")

    def test_cancellation_at_constructor_return_reaps_running_child(self):
        children = []
        handles = []
        real_init = runner._WindowsAffinityPopen.__init__

        def init_then_cancel(process, *args, **kwargs):
            real_init(process, *args, **kwargs)
            children.append(process)
            handles.append(self.retain_owned_handle(process._handle))
            raise KeyboardInterrupt()

        self.env["AFFINITY_TEST_WAIT"] = "1"
        with mock.patch.object(runner._WindowsAffinityPopen, "__init__", init_then_cancel):
            with self.assertRaises(KeyboardInterrupt):
                self.run_child()
        self.assertEqual(len(children), 1)
        self.assert_reaped(handles[0])
        self.assertIsNone(children[0]._handle)
        self.assertTrue(children[0].stderr.closed)

    def test_cancellation_immediately_after_resume_reaps_running_child(self):
        real_resume = runner._WindowsAffinityAPI.resume_thread

        def resume_then_cancel(api, handle):
            real_resume(api, handle)
            raise KeyboardInterrupt()

        handles = []
        real_set = runner._WindowsAffinityAPI.set_affinity

        def observe(api, handle, mask):
            handles.append(self.retain_owned_handle(handle))
            real_set(api, handle, mask)

        self.env["AFFINITY_TEST_WAIT"] = "1"
        with mock.patch.object(runner._WindowsAffinityAPI, "set_affinity", observe):
            with mock.patch.object(runner._WindowsAffinityAPI, "resume_thread", resume_then_cancel):
                with self.assertRaises(KeyboardInterrupt):
                    self.run_child()
        self.assertEqual(len(handles), 1)
        self.assert_reaped(handles[0])

    def test_cancellation_while_communicating_reaps_running_child(self):
        children = []
        handles = []

        def cancel(process):
            children.append(process)
            handles.append(self.retain_owned_handle(process._handle))
            # Wait for application readiness, not an arbitrary delay.
            self.assertEqual(process.stderr.readline(), b"test stderr\x00\xff\r\n")
            raise KeyboardInterrupt()

        self.env["AFFINITY_TEST_WAIT"] = "1"
        with mock.patch.object(runner._WindowsAffinityPopen, "communicate", cancel):
            with self.assertRaises(KeyboardInterrupt):
                self.run_child()
        self.assertEqual(len(children), 1)
        self.assert_reaped(handles[0])
        self.assertIsNone(children[0]._handle)
        self.assertIsNotNone(children[0].returncode)
        self.assertTrue(children[0].stderr.closed)
        self.assertEqual(json.loads(self.stdout.read_bytes())["mask"], 1 << self.cpu)

    def test_create_failure_is_reported_without_stdout(self):
        self.command[0] = str(self.root / "missing.exe")
        with self.assertRaises(FileNotFoundError):
            self.run_child()
        self.assertEqual(self.stdout.read_bytes(), b"")


class PosixLaunchContractTest(unittest.TestCase):
    def test_existing_post_spawn_placement_and_cancellation_are_preserved(self):
        # This checks control flow on Windows; it does not replace a POSIX run.
        for cancelled in (False, True):
            with self.subTest(cancelled=cancelled):
                process = mock.Mock(pid=123, returncode=23)
                process.communicate.side_effect = (KeyboardInterrupt() if cancelled
                                                   else [(None, b"stderr")])
                with mock.patch.object(runner.os, "name", "posix"), \
                        mock.patch.object(runner.os, "sched_setaffinity", create=True) as pin, \
                        mock.patch.object(runner.subprocess, "Popen", return_value=process):
                    arguments = dict(cwd="/test", env={}, stdout=io.BytesIO(), cpu=6)
                    if cancelled:
                        with self.assertRaises(KeyboardInterrupt):
                            runner.run_benchmark(["/test/probe"], **arguments)
                        process.kill.assert_called_once_with()
                        process.wait.assert_called_once_with()
                    else:
                        code, errors, affinity = runner.run_benchmark(["/test/probe"], **arguments)
                        self.assertEqual((code, errors), (23, b"stderr"))
                        self.assertIsNone(affinity["verified_process_mask"])
                        self.assertFalse(affinity["verified_before_resume"])
                        process.kill.assert_not_called()
                    pin.assert_called_once_with(123, {6})
                    process.stderr.close.assert_called_once_with()


@unittest.skipUnless(os.name == "nt", "Windows execution metadata")
class ExecutionMetadataTest(unittest.TestCase):
    def test_verified_launch_evidence_and_nonzero_exit_reach_execution_record(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            out = root / "out/Default"
            out.mkdir(parents=True)
            (root / "net/base").mkdir(parents=True)
            (root / "net/BUILD.gn").write_text("# Test fixture only\n")
            (root / "net/base/BUILD.gn").write_text("# Test fixture only\n")
            executable = out / "http_status_code_perftests.exe"
            executable.write_bytes(b"test fixture; never executed")
            evidence = root / "evidence"
            affinity = {"requested_mask": 64, "verified_process_mask": 64,
                        "verified_before_resume": True, "method": "test fixture"}
            args = ["runner", "--module", "http-status-code", "--executable", str(executable),
                    "--build-root", str(root), "--output-dir", str(evidence),
                    "--run", "metadata-test", "--cpu", "0",
                    "--dependency-inspector", str(root / "test-dumpbin.exe")]
            clean = subprocess.CompletedProcess([], 0, "ninja: no work to do.\n", "")
            with mock.patch.object(runner, "ROOT", root), mock.patch.object(sys, "argv", args), \
                    mock.patch.object(runner.subprocess, "run", return_value=clean), \
                    mock.patch.object(runner, "local_dll_dependencies", return_value=({}, [])), \
                    mock.patch.object(runner, "run_benchmark", return_value=(23, b"test stderr", affinity)), \
                    contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as exit_status:
                    runner.main()
            self.assertEqual(exit_status.exception.code, 23)
            record = json.loads((evidence / "metadata-test-execution.json").read_text())
            self.assertEqual(record["command"], [str(executable.resolve())])
            self.assertEqual(record["executable"], str(executable.resolve()))
            self.assertEqual(record["windows_command_line"], subprocess.list2cmdline(record["command"]))
            self.assertEqual(record["affinity"], affinity)
            self.assertEqual(record["affinity_mask"], affinity["verified_process_mask"])
            self.assertTrue(record["affinity_mask_is_verified"])
            self.assertEqual(record["exit_code"], 23)
            self.assertEqual(record["stderr"], "test stderr")
            self.assertEqual(record["raw_report_sha256"], runner.digest(evidence / "metadata-test-benchmark.jsonl"))
            self.assertEqual(record["source_hashes_file_sha256"], runner.digest(evidence / "metadata-test-source-hashes.json"))


if __name__ == "__main__":
    unittest.main()
