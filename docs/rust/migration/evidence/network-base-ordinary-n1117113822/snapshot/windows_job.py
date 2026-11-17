"""Own each command tree before allowing the actual command to start (Windows)."""
import ctypes
from ctypes import wintypes
import json
import os
import subprocess
import sys
import time


class Limits(ctypes.Structure):
    _fields_ = [('process_time', ctypes.c_int64), ('job_time', ctypes.c_int64),
        ('flags', wintypes.DWORD), ('minimum_ws', ctypes.c_size_t),
        ('maximum_ws', ctypes.c_size_t), ('active_limit', wintypes.DWORD),
        ('affinity', ctypes.c_size_t), ('priority', wintypes.DWORD),
        ('scheduling', wintypes.DWORD)]


class ExtendedLimits(ctypes.Structure):
    _fields_ = [('basic', Limits), ('io', ctypes.c_uint64 * 6),
        ('process_memory', ctypes.c_size_t), ('job_memory', ctypes.c_size_t),
        ('peak_process_memory', ctypes.c_size_t), ('peak_job_memory', ctypes.c_size_t)]


class Accounting(ctypes.Structure):
    _fields_ = [('times', ctypes.c_int64 * 4), ('faults', wintypes.DWORD),
        ('total', wintypes.DWORD), ('active', wintypes.DWORD),
        ('terminated', wintypes.DWORD)]


def api():
    if os.name != 'nt':
        raise RuntimeError('Windows job ownership required')
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    signatures = {
        'CreateJobObjectW': ([ctypes.c_void_p, wintypes.LPCWSTR], wintypes.HANDLE),
        'SetInformationJobObject': ([wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD], wintypes.BOOL),
        'AssignProcessToJobObject': ([wintypes.HANDLE, wintypes.HANDLE], wintypes.BOOL),
        'QueryInformationJobObject': ([wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p], wintypes.BOOL),
        'TerminateJobObject': ([wintypes.HANDLE, wintypes.UINT], wintypes.BOOL),
        'CloseHandle': ([wintypes.HANDLE], wintypes.BOOL),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(kernel, name)
        function.argtypes = arguments
        function.restype = result
    return kernel


def checked(value):
    if not value:
        raise ctypes.WinError(ctypes.get_last_error())
    return value


# The wrapper cannot launch the command until its director has assigned it to
# the job. Descendants inherit that job; breakaway permission is never enabled.
WRAPPER = "import json,subprocess,sys; token=sys.stdin.buffer.read(1); sys.exit(subprocess.call(json.loads(sys.argv[1]),stdin=subprocess.DEVNULL) if token==b'1' else 125)"


class ProcessTree:
    def __init__(self, command, **kwargs):
        self.kernel = api()
        self.job = checked(self.kernel.CreateJobObjectW(None, None))
        self.process = None
        try:
            limits = ExtendedLimits()
            limits.basic.flags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            checked(self.kernel.SetInformationJobObject(self.job, 9, ctypes.byref(limits), ctypes.sizeof(limits)))
            self.process = subprocess.Popen(
                [sys.executable, '-B', '-c', WRAPPER, json.dumps([str(x) for x in command])],
                stdin=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW, **kwargs)
            checked(self.kernel.AssignProcessToJobObject(self.job, wintypes.HANDLE(int(self.process._handle))))
            self.process.stdin.write(b'1')
            self.process.stdin.flush()
            self.process.stdin.close()
            self.process.stdin = None
        except BaseException:
            # Before assignment/handshake this root is blocked and cannot have
            # command descendants. Closing an assigned job also kills its tree.
            self.kernel.CloseHandle(self.job)
            self.job = None
            if self.process is not None:
                self.process.kill()
                self.process.wait(timeout=15)
            raise

    @property
    def returncode(self):
        return self.process.returncode

    def active(self):
        accounting = Accounting()
        checked(self.kernel.QueryInformationJobObject(self.job, 1, ctypes.byref(accounting), ctypes.sizeof(accounting), None))
        return accounting.active

    def empty(self, timeout):
        deadline = time.monotonic() + max(0, timeout)
        while self.active():
            if time.monotonic() >= deadline:
                raise subprocess.TimeoutExpired(self.process.args, timeout)
            time.sleep(0.01)

    def wait(self, timeout):
        deadline = time.monotonic() + timeout
        code = self.process.wait(timeout=timeout)
        self.empty(max(0, deadline - time.monotonic()))
        return code

    def communicate(self, timeout):
        deadline = time.monotonic() + timeout
        result = self.process.communicate(timeout=timeout)
        self.empty(max(0, deadline - time.monotonic()))
        return result

    def terminate_tree(self, timeout=15):
        required = bool(self.active())
        if required:
            checked(self.kernel.TerminateJobObject(self.job, 1))
        self.empty(timeout)
        self.process.wait(timeout=timeout)
        return {'required': required, 'owned_job_empty': True, 'reaped': True}

    def close(self):
        if self.job is not None:
            checked(self.kernel.CloseHandle(self.job))
            self.job = None
