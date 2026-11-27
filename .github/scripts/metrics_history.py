#!/usr/bin/env python3
"""Maintain chart-ready daily metrics history without rescanning source."""

from __future__ import annotations

from datetime import date, datetime, timezone
import json
import os
from pathlib import Path
import tempfile


# Bump when counting semantics change, not for UI, persistence or cache changes.
COUNTING_RULES_VERSION = 1
HISTORY_SCHEMA_VERSION = 1
CATEGORY_KEYS = ("actual_loc", "source_bytes", "files", "lines_of_code",
                 "lines_without_comment_only_lines", "comment_lines",
                 "google_mentions", "obsolete_functions")
CATEGORIES = ("maintained", "tests", "vendored")


def daily_record(metrics: dict) -> dict:
    updated = datetime.fromisoformat(metrics["updated"].replace("Z", "+00:00"))
    if updated.tzinfo is None:
        raise ValueError("Metrics timestamp must include a timezone")
    return {
        "date": updated.astimezone(timezone.utc).date().isoformat(),
        "updated": metrics["updated"], "commit": metrics["commit"],
        "counting_rules_version": metrics["counting_rules_version"],
        "categories": {
            category: {
                **{key: metrics["categories"][category][key] for key in CATEGORY_KEYS},
                "languages": {item["name"]: item["lines"]
                              for item in metrics["categories"][category]["languages"]},
            } for category in CATEGORIES
        },
        "configuration_files": metrics["configuration_files"],
        "root_folder_count": metrics["root_folder_count"],
        "excluded_files": metrics["excluded_files"],
    }


def subtract(current: dict, previous: dict) -> dict:
    """Difference numeric trees; missing language/exclusion keys count as zero."""
    result = {}
    for key in sorted(current.keys() | previous.keys()):
        value = current.get(key, 0)
        before = previous.get(key, 0)
        if isinstance(value, dict) or isinstance(before, dict):
            result[key] = subtract(value if isinstance(value, dict) else {},
                                   before if isinstance(before, dict) else {})
        else:
            result[key] = value - before
    return result


def update_history(path: Path, metrics: dict) -> dict:
    history = {"schema_version": HISTORY_SCHEMA_VERSION, "timezone": "UTC", "records": []}
    if path.exists():
        history = json.loads(path.read_text(encoding="utf-8"))
        if history["schema_version"] != HISTORY_SCHEMA_VERSION or history["timezone"] != "UTC":
            raise ValueError("Unsupported metrics history schema or timezone")
    record = daily_record(metrics)
    records = {}
    for existing in history["records"]:
        date.fromisoformat(existing["date"])
        if existing["date"] in records:
            raise ValueError("Duplicate date in metrics history")
        records[existing["date"]] = existing
    # A stale rerun must not overwrite a more recently measured snapshot.
    existing = records.get(record["date"])
    if existing is None or datetime.fromisoformat(record["updated"].replace("Z", "+00:00")) >= datetime.fromisoformat(existing["updated"].replace("Z", "+00:00")):
        records[record["date"]] = record
    ordered = [records[day] for day in sorted(records)]
    previous = None
    for item in ordered:
        comparable = previous is not None and previous["counting_rules_version"] == item["counting_rules_version"]
        item["previous_date"] = previous["date"] if comparable else None
        item["days_since_previous"] = ((date.fromisoformat(item["date"]) - date.fromisoformat(previous["date"])).days
                                       if comparable else None)
        item["delta"] = subtract(
            {key: item[key] for key in ("categories", "configuration_files", "root_folder_count", "excluded_files")},
            {key: previous[key] for key in ("categories", "configuration_files", "root_folder_count", "excluded_files")},
        ) if comparable else None
        previous = item
    history["records"] = ordered
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=path.name + ".", suffix=".tmp", delete=False) as output:
            temporary = Path(output.name)
            json.dump(history, output, ensure_ascii=False, indent=2)
            output.write("\n")
        os.replace(temporary, path)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()
    return history
