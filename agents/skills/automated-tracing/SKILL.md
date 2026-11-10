---
name: automated-tracing
description: >-
  Automated Tracing & Performance Telemetry in Chromium using Perfetto and
  Telemetry benchmarks. Use when you need to launch the browser binary,
  execute a specific scenario/story, and collect Perfetto traces. Don't use
  for trace analysis (use analyzing-sql-traces).
---

# Automated Tracing & Performance Telemetry

This skill guides the agent through the process of automated tracing in Chromium
to capture performance profiles on Desktop.

## 1. Prerequisites

### For Desktop (Linux/macOS/Windows)

- A compiled Chrome executable (typically `out/Default/chrome`).
- A targeted **Scenario/Story** (e.g., `omnibox:search` inside the `desktop_ui`
  benchmark).

______________________________________________________________________

## 2. Safety & Sandbox Compliance (Zero-Grant Rule)

To prevent triggering user permission/access grant prompts during automated
execution:

- **ALWAYS** redirect Telemetry output artifacts to the parent E2E session's
  unified capture directory inside the workspace:
  `out/e2e_nla_run_{parent_session_id}/capture/` (where `{parent_session_id}` is
  passed by the Orchestrator).
- **NEVER** call command-line utilities like `mkdir`, `ls`, `touch`, or `rm` via
  shell commands.
- **NEVER** run terminal verification commands (such as
  `python3 -c "import os; ..."` or `ls -lh` or `test -f`) to check file
  existence or size.
- **ALWAYS** rely on standard python script APIs or built-in benchmark tool
  logic to create directories programmatically.
- **ALWAYS** let down-stream tools (like the trace analyzer script) perform
  validation internally. If the trace is invalid or empty, the analyzer will
  fail loudly and safely.
- **NEVER** let Telemetry write to the default `tools/perf/artifacts/` workspace
  directory.

______________________________________________________________________

## 3. Execution & Capture Workflow

### Step A: Execute Telemetry Benchmark

Select either **Cold Run** (default) or **Warm Run** based on the investigation
requirements.

#### 1. For Cold Runs (Default)

No pre-warmup is needed. Telemetry will automatically start with a clean
profile.

**For Desktop:** Run the `run_benchmark` command directly using the `xvfb.py`
virtual display wrapper:

```bash
./testing/xvfb.py vpython3 tools/perf/run_benchmark run desktop_ui \
  --story={story_name} \
  --browser=exact \
  --browser-executable={chrome_binary} \
  --extra-chrome-categories=omnibox,navigation,blink,cc,gpu,toplevel \
  --output-dir=out/e2e_nla_run_{parent_session_id}/capture/
```

#### 2. For Warm Runs (GPU & HTTP Caches Warmed)

To capture a warm run (bypassing GPU shader compilation and static network
loading), you should use the automated wrapper script `run_warm_benchmark.py`.

The script automatically coordinates the warmup run, profile migration/cleanup,
final warm run, and temporary directory deletion.

*For Desktop:*

```bash
vpython3 .agents/skills/automated-tracing/scripts/run_warm_benchmark.py \
  --benchmark=desktop_ui \
  --story={story_name} \
  --browser-executable=out/Default/chrome \
  --output-dir=out/e2e_nla_run_{parent_session_id}/capture/ \
  [--delete-state] \
  --extra-chrome-categories=omnibox,navigation,blink,cc,gpu,toplevel,net \
  [extra_args...]
```

*(Note: The script automatically detects if a display is present on Linux and
wraps the desktop Chrome execution with `xvfb.py` if needed. Any unrecognized
extra arguments `[extra_args...]` are forwarded directly to the underlying
`run_benchmark` commands.)*

### Step C: Locate the Captured Trace

Once the benchmark completes successfully, find the Perfetto trace file inside
the redirected folder. The path structure is:
`out/e2e_nla_run_{parent_session_id}/capture/artifacts/run_{timestamp}/{story_sanitized}_1/trace/trace.pb`

Verify the trace exists and is non-empty.

______________________________________________________________________

## 4. Output Contract

Report back to the caller with the absolute path of the captured trace:

```json
{
  "status": "SUCCESS",
  "trace_file_path": "out/e2e_nla_run_{parent_session_id}/capture/artifacts/run_{timestamp}/{story_sanitized}_1/trace/trace.pb",
  "build_dir": "out/Default/"
}
```
