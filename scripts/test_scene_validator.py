#!/usr/bin/env python3
"""Smoke tests for the scene validator CLI."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = REPO_ROOT / "scripts" / "tiny_hippie_validate.py"


def run_validator(scene: dict) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory() as tmp:
        scene_path = Path(tmp) / "scene.scene.json"
        scene_path.write_text(json.dumps(scene), encoding="utf-8")
        return subprocess.run(
            [sys.executable, str(VALIDATOR), str(scene_path), "--repo-root", str(REPO_ROOT)],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )


def expect_pass(scene: dict) -> None:
    result = run_validator(scene)
    if result.returncode != 0:
        raise AssertionError(f"expected validator success, got:\n{result.stderr}")


def expect_fail(scene: dict, expected: str) -> None:
    result = run_validator(scene)
    if result.returncode == 0:
        raise AssertionError("expected validator failure")
    if expected not in result.stderr:
        raise AssertionError(f"expected {expected!r} in:\n{result.stderr}")


def main() -> int:
    expect_pass({"name": "ok", "nodes": [{"name": "root", "type": "SceneNode"}]})
    expect_fail(
        {"name": "bad", "nodes": [{"type": "SceneNode", "surprise": True}]},
        "unknown field",
    )
    expect_fail(
        {"name": "bad", "nodes": [{"type": "SpriteNode", "image": "missing.png"}]},
        "asset does not exist",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
