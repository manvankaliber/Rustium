#!/usr/bin/env python3
"""Local, single-director coordination. JSON is durable; OS locks are local.

No agent launcher, remote service, daemon, automatic recovery or build-tree sync.
All linked worktrees resolve the same canonical queue through git-common-dir.
"""
import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import hashlib
import json
import os
import re
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid

ACTIVE = {"assigned", "implementing", "needs-repair"}
PENDING = {"submitted", "validating", "merge-ready"}
TRANSITIONS = {
    "ready": {"assigned", "blocked"},
    "assigned": {"implementing", "blocked", "ready"},
    "implementing": {"submitted", "blocked", "needs-repair"},
    "submitted": {"validating", "needs-repair", "blocked"},
    "validating": {"merge-ready", "needs-repair", "blocked"},
    "needs-repair": {"assigned", "blocked"},
    "merge-ready": {"integrated", "needs-repair", "blocked"},
    "blocked": {"ready", "needs-repair"},
    "integrated": set(),
}


def now():
    return datetime.now(timezone.utc).isoformat()


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


def canonical(root):
    common = Path(git(root, "rev-parse", "--path-format=absolute", "--git-common-dir"))
    if common.name != ".git":
        raise ValueError("Expected a non-bare primary checkout with a .git directory")
    return common.parent.resolve()


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def atomic(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as f:
            json.dump(value, f, indent=2, ensure_ascii=False)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


@contextmanager
def lock(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "a+b") as f:
        if path.stat().st_size == 0:
            f.write(b"0")
            f.flush()
        f.seek(0)
        try:
            if os.name == "nt":
                import msvcrt
                msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(f.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as e:
            raise ValueError(f"Busy lock: {path}; retry after the current operation") from e
        try:
            yield
        finally:
            f.seek(0)
            if os.name == "nt":
                msvcrt.locking(f.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(f.fileno(), fcntl.LOCK_UN)


def overlap(a, b):
    a, b = a.rstrip("/").lower(), b.rstrip("/").lower()
    return a == b or a.startswith(b + "/") or b.startswith(a + "/")


def worker_limit(q):
    # Historical queue receipts used a model-specific budget key.
    return q["limits"].get("coding_workers", q["limits"].get("luna_workers"))


class Coordinator:
    def __init__(self, root):
        self.root = canonical(root)
        self.state = self.root / "docs/rust/migration/queue.json"
        self.runtime = self.root / ".rustium-runtime"

    @contextmanager
    def transaction(self, token):
        with lock(self.runtime / "queue.lock"):
            lease = read(self.runtime / "director.json")
            if lease["token"] != token:
                raise ValueError("Director token mismatch")
            q = read(self.state)
            yield q
            self.check(q)
            q["revision"] += 1
            q["updated_utc"] = now()
            atomic(self.state, q)

    def acquire(self, session):
        with lock(self.runtime / "queue.lock"):
            path = self.runtime / "director.json"
            if path.exists():
                raise ValueError("Director lease exists; inspect session/process state, then recover explicitly")
            data = {"session": session, "token": uuid.uuid4().hex, "created_utc": now()}
            atomic(path, data)
            return data

    def release(self, token):
        with lock(self.runtime / "queue.lock"):
            path = self.runtime / "director.json"
            if read(path)["token"] != token:
                raise ValueError("Director token mismatch")
            path.unlink()

    def recover(self, kind, evidence):
        """Explicit human/agent reconciliation attestation; never infer from age/PID."""
        if not evidence.is_file() or not evidence.read_text(encoding="utf-8").strip():
            raise ValueError("A nonempty reconciliation evidence file is required")
        gate = "queue.lock" if kind == "director" else "validation.lock"
        with lock(self.runtime / gate):
            path = self.runtime / ("director.json" if kind == "director" else "job-active.json")
            receipt = {"recovered_utc": now(), "previous": read(path),
                       "evidence": str(evidence.resolve()), "sha256": hashlib.sha256(evidence.read_bytes()).hexdigest()}
            atomic(self.runtime / "recovery" / (uuid.uuid4().hex + ".json"), receipt)
            path.unlink()

    @staticmethod
    def task(q, task_id):
        return next(t for t in q["tasks"] if t["id"] == task_id)

    @staticmethod
    def event(t, action, note):
        t["history"].append({"utc": now(), "action": action, "note": note})

    def dispatch_hold(self, focus, task):
        if not focus:
            return None
        if (task["subsystem"] != focus["subsystem"]
                and task["id"] not in focus.get("allowed_dependency_tasks", [])):
            return "Task outside active subsystem focus; preserve it as held work"
        if focus.get("single_network_family"):
            family = read(self.root / "tools/ci/network-focus.json")["family"]
            if task.get("network_family") != family:
                return f"Dispatch is limited to the active Network family: {family}"
        return None

    def claim(self, token, task_id, session):
        with self.transaction(token) as q:
            t = self.task(q, task_id)
            hold = self.dispatch_hold(q.get("active_focus"), t)
            if hold:
                raise ValueError(hold)
            if t["state"] not in {"ready", "needs-repair"} or t["assignment"].get("active"):
                raise ValueError("Task is not available")
            if t["blockers"] or any(self.task(q, d)["state"] != "integrated" for d in t["dependencies"]):
                raise ValueError("Task dependencies/blockers are unresolved")
            active = [x for x in q["tasks"] if x["assignment"].get("active")]
            if len(active) >= worker_limit(q) + q["limits"]["auxiliary_children"]:
                raise ValueError("Project agent capacity is full; release completed auxiliary work")
            if t["role"] == "coding_worker":
                if sum(x["role"] == "coding_worker" for x in active) >= worker_limit(q):
                    raise ValueError("Global eight-worker pool is full")
                if t["state"] != "needs-repair" and sum(x["state"] in PENDING for x in q["tasks"]) >= q["limits"]["pending_validation"]:
                    raise ValueError("Validation backlog full: repair or integrate first")
            if any(overlap(a, b) for x in active for a in t["owned_files"] for b in x["owned_files"]):
                raise ValueError("Source ownership overlaps an active task")
            t["state"] = "assigned"
            t["assignment"] = {"active": True, "session": session, "worktree": None,
                               "branch": None, "model": q["roles"][t["role"]]["model"],
                               "effort": q["roles"][t["role"]]["effort"]}
            self.event(t, "assigned", session)
            return t

    def bind(self, token, task_id, workspace):
        workspace = workspace.resolve()
        with self.transaction(token) as q:
            t = self.task(q, task_id)
            if not t["assignment"].get("active") or t["state"] != "assigned":
                raise ValueError("Reserve task before binding workspace")
            if canonical(workspace) != self.root or workspace == self.root:
                raise ValueError("Use a separate linked task worktree of this repository")
            if Path(git(workspace, "rev-parse", "--show-toplevel")).resolve() != workspace:
                raise ValueError("Bind the worktree root, not a subdirectory")
            branch = git(workspace, "branch", "--show-current")
            if not branch.startswith("rust/") or git(workspace, "status", "--porcelain"):
                raise ValueError("Require a clean rust/* task branch")
            expected = t.get("delivery_commit") or t["base_revision"]
            if git(workspace, "rev-parse", "HEAD") != expected:
                raise ValueError("Workspace HEAD differs from task base or exact repair delivery; reconcile first")
            for x in q["tasks"]:
                other = x["assignment"].get("worktree")
                if x["id"] != task_id and x["assignment"].get("active") and other and Path(other).resolve() == workspace:
                    raise ValueError("Workspace already owned")
            t["assignment"].update(worktree=str(workspace), branch=branch)
            self.event(t, "workspace-bound", str(workspace))

    def workspace(self, token, task_id, destination):
        """Native Codex route only. T3 delegates use isolation=worktree instead."""
        with self.transaction(token) as q:
            t = self.task(q, task_id)
            if t["state"] != "assigned" or not t["assignment"].get("active") or t["assignment"].get("worktree"):
                raise ValueError("Task must be reserved and unbound")
            if destination.exists():
                raise ValueError("Destination exists; inspect and bind explicitly")
            listed = git(self.root, "worktree", "list", "--porcelain")
            worker_trees = 0
            for block in listed.split("\n\n"):
                lines = block.splitlines()
                paths = [line.removeprefix("worktree ") for line in lines if line.startswith("worktree ")]
                if paths and Path(paths[0]).resolve() != self.root and any(line.startswith("branch refs/heads/rust/") for line in lines):
                    worker_trees += 1
            if worker_trees >= q["limits"]["editing_workspaces"]:
                raise ValueError("Editing workspace limit reached; safely release a submitted clean slot")
            ancestor = destination.resolve().parent
            while not ancestor.exists():
                ancestor = ancestor.parent
            if shutil.disk_usage(ancestor).free < q["limits"].get("minimum_free_disk_bytes", 8 * 1024**3):
                raise ValueError("Low free disk; inspect existing copies and release safe slots before allocating")
            branch = "rust/" + task_id.lower() + "-" + uuid.uuid4().hex[:8]
            git(self.root, "worktree", "add", "--no-checkout", "-b", branch, str(destination), t["base_revision"])
            # Sparse cone includes root instructions; add broader context as needed.
            dirs = {".codex", "docs/rust/migration"}
            dirs.update(str(Path(p).parent).replace("\\", "/") for p in t["owned_files"] if Path(p).parent != Path("."))
            dirs.update(t.get("context_directories", []))
            git(destination, "sparse-checkout", "set", "--cone", *sorted(dirs))
            git(destination, "checkout", branch)
            t["assignment"].update(worktree=str(destination.resolve()), branch=branch)
            self.event(t, "workspace-created", str(destination))
            return t["assignment"]

    def transition(self, token, task_id, state, note, evidence=None, commit=None):
        with self.transaction(token) as q:
            t = self.task(q, task_id)
            if state not in TRANSITIONS[t["state"]]:
                raise ValueError(f"Invalid transition {t['state']} -> {state}")
            if state == "implementing" and not t["assignment"].get("worktree"):
                raise ValueError("Bind an isolated workspace first")
            if state == "submitted":
                workspace = t["assignment"].get("worktree")
                if not workspace or not commit or git(workspace, "rev-parse", "HEAD") != commit or git(workspace, "status", "--porcelain"):
                    raise ValueError("Submission needs a clean workspace and its exact committed HEAD")
                changed = git(workspace, "diff", "--name-only", t["base_revision"], commit).splitlines()
                if not changed:
                    raise ValueError("Delivery has no changes from its base")
                if any(not any(p == owned or p.startswith(owned.rstrip("/") + "/") for owned in t["owned_files"]) for p in changed):
                    raise ValueError("Delivery contains changes outside task ownership")
                t["delivery_commit"] = commit
            if state in {"submitted", "merge-ready", "integrated"}:
                if evidence is None or not evidence.is_file():
                    raise ValueError("Durable evidence file required")
                ev = read(evidence)
                if ev.get("task_id") != task_id or ev.get("candidate_commit") != t["delivery_commit"]:
                    raise ValueError("Evidence must identify task and delivery commit")
                if state == "merge-ready":
                    if (not isinstance(ev.get("tests"), dict) or ev["tests"].get("status") != "passed"
                            or not isinstance(ev.get("review"), dict) or ev["review"].get("status") != "accepted"
                            or not ev.get("source_identity") or "platform_limits" not in ev):
                        raise ValueError("Acceptance evidence lacks passed tests, accepted review, identity or platform limits")
                if state == "integrated":
                    accepted = ev.get("integration_commit")
                    if not accepted:
                        raise ValueError("Integration commit required")
                    git(self.root, "merge-base", "--is-ancestor", accepted, "HEAD")
                    t["integration_commit"] = accepted
                t["evidence"].append({"path": str(evidence.resolve()), "sha256": hashlib.sha256(evidence.read_bytes()).hexdigest()})
            t["state"], t["next_action"] = state, note
            # Requires director to confirm the worker has ended before this call.
            if state in {"submitted", "blocked", "ready", "integrated", "needs-repair"}:
                t["assignment"]["active"] = False
            self.event(t, state, note)

    def amend(self, token, task_id, patch):
        allowed = {"base_revision", "dependencies", "blockers", "next_action", "acceptance_tests",
                   "build_targets", "performance", "stages", "evidence", "original_cpp_reference",
                   "network_family"}
        if set(patch) - allowed:
            raise ValueError("Patch may only amend planning/evidence fields; use state commands for lifecycle")
        with self.transaction(token) as q:
            t = self.task(q, task_id)
            if "base_revision" in patch:
                if t["assignment"].get("active") or t["delivery_commit"]:
                    raise ValueError("Do not repin an active or delivered task")
                patch["base_revision"] = git(self.root, "rev-parse", patch["base_revision"] + "^{commit}")
            if "dependencies" in patch and any(d == task_id or d not in {x["id"] for x in q["tasks"]} for d in patch["dependencies"]):
                raise ValueError("Unknown/self dependency")
            t.update(patch)
            self.event(t, "amended", ", ".join(sorted(patch)))

    def enqueue(self, token, task):
        with self.transaction(token) as q:
            if task["id"] in {t["id"] for t in q["tasks"]}:
                raise ValueError("Task ID already exists")
            required = set(q["tasks"][0]) - {"integration_commit", "context_directories", "network_family"}
            if required - task.keys() or task["state"] not in {"ready", "blocked"} or task["assignment"].get("active"):
                raise ValueError("New task needs complete schema and inactive ready/blocked state")
            if task["role"] not in q["roles"] or any(d not in {t["id"] for t in q["tasks"]} for d in task["dependencies"]):
                raise ValueError("Unknown role or dependency")
            q["tasks"].append(task)
            self.event(task, "enqueued", task["objective"])

    def check(self, q=None):
        q = read(self.state) if q is None else q
        ids = [t["id"] for t in q["tasks"]]
        if len(set(ids)) != len(ids):
            raise ValueError("Duplicate task IDs")
        required = {"id", "subsystem", "objective", "behavioral_contract", "original_cpp_reference", "owned_files",
                    "shared_interfaces", "dependencies", "blockers", "role", "assignment", "base_revision",
                    "delivery_commit", "acceptance_tests", "build_targets", "performance", "state", "next_action", "evidence", "stages", "history"}
        for t in q["tasks"]:
            if required - t.keys() or t["state"] not in TRANSITIONS or t["role"] not in q["roles"] or any(d not in ids for d in t["dependencies"]):
                raise ValueError("Invalid task schema/dependencies: " + t["id"])
            for p in t["owned_files"]:
                if Path(p).is_absolute() or ".." in Path(p).parts or "*" in p:
                    raise ValueError("Ownership must use relative paths/prefixes")
        def visit(task_id, ancestors):
            if task_id in ancestors:
                raise ValueError("Dependency cycle: " + task_id)
            for dep in self.task(q, task_id)["dependencies"]:
                visit(dep, ancestors | {task_id})
        for task_id in ids:
            visit(task_id, set())
        active = [t for t in q["tasks"] if t["assignment"].get("active") and t["role"] == "coding_worker"]
        if len(active) > worker_limit(q):
            raise ValueError("Global coding worker budget exceeded")
        return {"valid": True, "root": str(self.root), "tasks": len(ids), "active_workers": len(active),
                "ready": [t["id"] for t in q["tasks"] if t["state"] == "ready" and not t["blockers"]
                          and not self.dispatch_hold(q.get("active_focus"), t)
                          and all(self.task(q, d)["state"] == "integrated" for d in t["dependencies"])]}

    def job(self, token, manifest, command):
        """One gate for configured sync, build, substantial test and quiet timing.

        Persistent marker survives an interrupted launcher; no automatic clearing.
        The director checks external processes before this, including old campaigns.
        """
        spec = read(manifest)
        for k in ["task_id", "source_identity", "toolchain", "build_configuration", "cwd", "kind", "process_reconciliation"]:
            if not spec.get(k):
                raise ValueError("Missing job provenance: " + k)
        if spec["kind"] not in {"sync", "build", "test", "benchmark"} or not command:
            raise ValueError("Invalid job kind/command")
        if spec["kind"] == "build":
            jobs = [int(m.group(1)) for arg in command if (m := re.fullmatch(r"-j(\d+)", arg))]
            if not jobs or any(n < 4 or n > 8 for n in jobs) or "-j" in command:
                raise ValueError("Substantial build must explicitly use -j4 through -j8")
            if any("siso" in arg.lower() or "reclient" in arg.lower() for arg in command):
                raise ValueError("Use verified local Ninja; remote execution is not configured")
        with lock(self.runtime / "validation.lock"):
            marker = self.runtime / "job-active.json"
            if marker.exists():
                raise ValueError("Unreconciled prior job; inspect child processes and result before recovery")
            with self.transaction(token) as q:
                self.task(q, spec["task_id"])
            receipt = dict(spec, command=command, started_utc=now(), status="running", launcher_pid=os.getpid())
            run = self.runtime / "jobs" / uuid.uuid4().hex
            run.mkdir(parents=True)
            atomic(marker, dict(receipt, result_directory=str(run)))
            atomic(run / "result.json", receipt)
            # Leave marker on interruption. Child may still own compiler outputs.
            with (run / "output.log").open("wb") as output:
                child = subprocess.Popen(command, cwd=spec["cwd"], stdout=output, stderr=subprocess.STDOUT)
                receipt["child_pid"] = child.pid
                atomic(marker, dict(receipt, result_directory=str(run)))
                code = child.wait()
            receipt.update(status="passed" if code == 0 else "failed", exit_code=code, finished_utc=now())
            atomic(run / "result.json", receipt)
            marker.unlink()
            return {"result": str(run / "result.json"), "exit_code": code}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, default=Path.cwd())
    sub = p.add_subparsers(dest="action", required=True)
    sub.add_parser("check")
    sub.add_parser("status")
    x = sub.add_parser("acquire"); x.add_argument("--session", required=True)
    x = sub.add_parser("release"); x.add_argument("--token", required=True)
    x = sub.add_parser("recover"); x.add_argument("kind", choices=["director", "job"]); x.add_argument("--evidence", type=Path, required=True)
    for verb in ["claim", "workspace", "bind", "transition"]:
        x = sub.add_parser(verb); x.add_argument("task"); x.add_argument("--token", required=True)
        if verb == "claim": x.add_argument("--session", required=True)
        elif verb in {"workspace", "bind"}: x.add_argument("path", type=Path)
        else:
            x.add_argument("state", choices=TRANSITIONS); x.add_argument("--note", required=True)
            x.add_argument("--evidence", type=Path); x.add_argument("--commit")
    x = sub.add_parser("amend"); x.add_argument("task"); x.add_argument("--token", required=True); x.add_argument("--patch", type=Path, required=True)
    x = sub.add_parser("enqueue"); x.add_argument("--token", required=True); x.add_argument("--task-file", type=Path, required=True)
    x = sub.add_parser("job"); x.add_argument("--token", required=True); x.add_argument("--manifest", type=Path, required=True); x.add_argument("command", nargs=argparse.REMAINDER)
    a = p.parse_args()
    c = Coordinator(a.root)
    if a.action == "check": result = c.check()
    elif a.action == "status":
        result = c.check()
        result["git_head"] = git(c.root, "rev-parse", "HEAD")
        result["worktrees"] = git(c.root, "worktree", "list", "--porcelain")
        result["runtime"] = {n: read(c.runtime / n) for n in ["director.json", "job-active.json"] if (c.runtime / n).exists()}
    elif a.action == "acquire": result = c.acquire(a.session)
    elif a.action == "release": result = c.release(a.token)
    elif a.action == "recover": result = c.recover(a.kind, a.evidence)
    elif a.action == "claim": result = c.claim(a.token, a.task, a.session)
    elif a.action in {"workspace", "bind"}: result = getattr(c, a.action)(a.token, a.task, a.path)
    elif a.action == "transition": result = c.transition(a.token, a.task, a.state, a.note, a.evidence, a.commit)
    elif a.action == "amend": result = c.amend(a.token, a.task, read(a.patch))
    elif a.action == "enqueue": result = c.enqueue(a.token, read(a.task_file))
    else:
        command = a.command[1:] if a.command[:1] == ["--"] else a.command
        result = c.job(a.token, a.manifest, command)
    print(json.dumps(result, indent=2))
    return result.get("exit_code", 0) if isinstance(result, dict) else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, StopIteration, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
