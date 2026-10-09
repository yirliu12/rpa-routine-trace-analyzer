#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/main.c"
TESTS = ROOT / "tests"

with tempfile.TemporaryDirectory() as temp:
    exe = Path(temp) / "boolean-trace-analyzer-reference"
    subprocess.run([
        "gcc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-O2",
        str(SOURCE), "-o", str(exe),
    ], check=True)
    failed = []
    for case in sorted(TESTS.glob("case*")):
        run = subprocess.run(
            [str(exe)],
            input=(case / "input.txt").read_bytes(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        expected = (case / "expected.txt").read_bytes().replace(b"\r\n", b"\n")
        actual = run.stdout.replace(b"\r\n", b"\n")
        if run.returncode != 0 or actual != expected:
            failed.append(case.name)
            print(f"FAIL {case.name} (exit {run.returncode})")
        else:
            print(f"PASS {case.name}")
    if failed:
        raise SystemExit(f"{len(failed)} test case(s) failed")
