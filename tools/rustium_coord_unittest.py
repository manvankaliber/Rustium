#!/usr/bin/env python3
"""Focused coordination checks, using disposable miniature Git repositories."""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import rustium_coord as rc


class CoordinationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "repo"
        self.root.mkdir()
        rc.git(self.root, "init", "-b", "main")
        rc.git(self.root, "config", "user.name", "Coordination Test")
        rc.git(self.root, "config", "user.email", "local-test@example.invalid")
        (self.root / ".gitignore").write_text(".rustium-runtime/\n")
        (self.root / "AGENTS.md").write_text("Fixture only\n")
        (self.root / "source").mkdir()
        (self.root / "source/a.cc").write_text("original\n")
        rc.git(self.root, "add", ".")
        rc.git(self.root, "commit", "-qm", "fixture")
        self.head = rc.git(self.root, "rev-parse", "HEAD")
        self.c = rc.Coordinator(self.root)
        seed = rc.read(Path(__file__).resolve().parents[1] / "docs/rust/migration/queue.json")
        sample = next(t for t in seed["tasks"] if t["role"] == "coding_worker")
        seed["tasks"] = []
        seed["limits"]["minimum_free_disk_bytes"] = 0
        for i in range(10):
            t = copy.deepcopy(sample)
            t.update(id=f"T{i}", base_revision=self.head, owned_files=[f"drafts/t{i}"], context_directories=["source"])
            seed["tasks"].append(t)
        rc.atomic(self.c.state, seed)
        self.token = self.c.acquire("test-director")["token"]

    def claim(self, n=0):
        return self.c.claim(self.token, f"T{n}", f"worker-{n}")

    def workspace(self, n=0):
        dest = Path(self.temp.name) / f"slot-{n}"
        self.c.workspace(self.token, f"T{n}", dest)
        return dest

    def test_startup_and_eight_global_slots(self):
        self.assertEqual(len(self.c.check()["ready"]), 10)
        for n in range(8):
            self.claim(n)
        with self.assertRaisesRegex(ValueError, "pool is full"):
            self.claim(8)
        self.assertEqual(self.c.check()["active_luna"], 8)
        with self.assertRaises(ValueError):
            self.c.claim("different-director", "T8", "bad")

    def test_single_director_and_explicit_recovery(self):
        with self.assertRaisesRegex(ValueError, "lease exists"):
            self.c.acquire("second")
        self.claim()
        note = self.root / "reconciled.md"
        note.write_text("Fixture: previous director stopped; worker retained, outcome unconfirmed.")
        self.c.recover("director", note)
        token = self.c.acquire("replacement")["token"]
        self.assertNotEqual(token, self.token)
        self.assertTrue(rc.read(self.c.state)["tasks"][0]["assignment"]["active"])
        with self.assertRaisesRegex(ValueError, "not available"):
            self.c.claim(token, "T0", "duplicate")

    def test_overlap_and_dependencies(self):
        q = rc.read(self.c.state)
        q["tasks"][1]["owned_files"] = ["drafts/t0/child"]
        q["tasks"][2]["dependencies"] = ["T9"]
        rc.atomic(self.c.state, q)
        self.claim()
        with self.assertRaisesRegex(ValueError, "overlaps"):
            self.claim(1)
        with self.assertRaisesRegex(ValueError, "dependencies"):
            self.claim(2)

    def test_sparse_isolation_and_shared_queue(self):
        self.claim()
        self.claim(1)
        a, b = self.workspace(), self.workspace(1)
        self.assertNotEqual(rc.git(a, "branch", "--show-current"), rc.git(b, "branch", "--show-current"))
        (a / "source/a.cc").write_text("worker edit\n")
        self.assertEqual((b / "source/a.cc").read_text(), "original\n")
        self.assertEqual((self.root / "source/a.cc").read_text(), "original\n")
        self.assertEqual(rc.Coordinator(a).state, self.c.state)
        self.assertEqual(rc.git(a, "config", "--get", "core.sparseCheckout"), "true")

    def test_submission_guards_and_slot_release(self):
        self.claim()
        a = self.workspace()
        self.c.transition(self.token, "T0", "implementing", "bound")
        draft = a / "drafts/t0"
        draft.mkdir(parents=True)
        (draft / "core.rs").write_text("// candidate fixture\n")
        rc.git(a, "add", "--sparse", "drafts/t0/core.rs")
        rc.git(a, "commit", "-qm", "candidate")
        sha = rc.git(a, "rev-parse", "HEAD")
        ev = self.root / "evidence.json"
        rc.atomic(ev, {"task_id": "T0", "candidate_commit": sha})
        with self.assertRaisesRegex(ValueError, "exact committed HEAD"):
            self.c.transition(self.token, "T0", "submitted", "wrong SHA", ev, self.head)
        self.c.transition(self.token, "T0", "submitted", "worker ended", ev, sha)
        self.assertEqual(self.c.check()["active_luna"], 0)
        self.c.transition(self.token, "T0", "validating", "review")
        with self.assertRaisesRegex(ValueError, "Acceptance evidence lacks"):
            self.c.transition(self.token, "T0", "merge-ready", "missing checks", ev)

    def test_backlog_bound_and_repair_priority(self):
        q = rc.read(self.c.state)
        for t in q["tasks"][:8]:
            t["state"] = "submitted"
        q["tasks"][9]["state"] = "needs-repair"
        rc.atomic(self.c.state, q)
        with self.assertRaisesRegex(ValueError, "backlog full"):
            self.claim(8)
        self.claim(9)

    def test_real_cross_process_lock(self):
        path = self.c.runtime / "validation.lock"
        code = "import sys; from pathlib import Path; import rustium_coord as r;\nwith r.lock(Path(sys.argv[1])): print('acquired')"
        with rc.lock(path):
            child = subprocess.run([sys.executable, "-c", code, str(path)], cwd=Path(__file__).parent, capture_output=True, text=True)
            self.assertNotEqual(child.returncode, 0)
            self.assertIn("Busy lock", child.stderr)
        with rc.lock(path):
            pass

    def spec(self, kind="test"):
        p = self.root / "request.json"
        rc.atomic(p, {"task_id": "T0", "kind": kind, "source_identity": self.head,
                      "toolchain": sys.version, "build_configuration": "disposable fixture; no Chromium build",
                      "cwd": str(self.root), "process_reconciliation": "Only fixture process; no external job"})
        return p

    def test_job_failure_and_interrupted_marker(self):
        result = self.c.job(self.token, self.spec(), [sys.executable, "-c", "raise SystemExit(7)"])
        self.assertEqual(result["exit_code"], 7)
        self.assertEqual(rc.read(Path(result["result"]))["status"], "failed")
        marker = self.c.runtime / "job-active.json"
        rc.atomic(marker, {"status": "unknown after interruption", "child_pid": 123})
        with self.assertRaisesRegex(ValueError, "Unreconciled"):
            self.c.job(self.token, self.spec(), [sys.executable, "-c", "pass"])
        note = self.root / "job-reconciled.md"
        note.write_text("Fixture only: no actual child; preserve unknown outcome.")
        self.c.recover("job", note)
        self.assertFalse(marker.exists())

    def test_build_ceiling_and_workspace_ceiling(self):
        rc.git(self.root, "branch", "-m", "rust/director")
        with self.assertRaisesRegex(ValueError, "-j1 or -j2"):
            self.c.job(self.token, self.spec("build"), ["ninja", "-j3"])
        self.claim()
        self.workspace()
        q = rc.read(self.c.state)
        q["limits"]["editing_workspaces"] = 1
        rc.atomic(self.c.state, q)
        self.claim(1)
        with self.assertRaisesRegex(ValueError, "workspace limit"):
            self.workspace(1)

    def test_atomic_failure_does_not_change_queue(self):
        before = self.c.state.read_bytes()
        with self.assertRaisesRegex(ValueError, "Invalid transition"):
            self.c.transition(self.token, "T0", "integrated", "not validated")
        self.assertEqual(before, self.c.state.read_bytes())

    def test_new_task_and_cycle_rejection(self):
        t = copy.deepcopy(rc.read(self.c.state)["tasks"][0])
        t.update(id="FOLLOWUP", owned_files=["drafts/followup"], dependencies=["T0"])
        self.c.enqueue(self.token, t)
        self.assertEqual(self.c.check()["tasks"], 11)
        before = self.c.state.read_bytes()
        with self.assertRaisesRegex(ValueError, "cycle"):
            self.c.amend(self.token, "T0", {"dependencies": ["FOLLOWUP"]})
        self.assertEqual(before, self.c.state.read_bytes())

    def test_bind_rejects_subdirectory_alias(self):
        self.claim()
        a = self.workspace()
        self.claim(1)
        with self.assertRaisesRegex(ValueError, "worktree root"):
            self.c.bind(self.token, "T1", a / "source")


if __name__ == "__main__":
    unittest.main()
