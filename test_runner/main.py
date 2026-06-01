import os
import subprocess
import sys
import difflib
import argparse
import time


def parse_expected(expected_output):
    # `//` lines are comments (stripped before comparison). A `// xfail: <reason>`
    # directive marks the test as expected to fail, linter-style (cf. `# noqa`).
    xfail = None
    output_lines = []
    for line in expected_output.splitlines():
        stripped = line.strip()
        if stripped.startswith("//"):
            body = stripped[2:].strip()
            if body.lower().startswith("xfail"):
                rest = body[len("xfail") :].lstrip()
                reason = rest[1:].strip() if rest.startswith(":") else rest.strip()
                xfail = reason or "expected failure"
            continue
        output_lines.append(line.rstrip())
    # Drop trailing empty lines for a cleaner comparison
    while output_lines and not output_lines[-1]:
        output_lines.pop()
    return output_lines, xfail


def run_test(binary, test_dir):
    commands_file = os.path.join(test_dir, "commands.txt")
    expected_file = os.path.join(test_dir, "expected.txt")

    if not os.path.exists(commands_file):
        return "skip", f"commands.txt not found in {test_dir}"

    if not os.path.exists(expected_file):
        return "skip", f"expected.txt not found in {test_dir}"

    try:
        with open(expected_file, "r") as f:
            expected_lines, xfail = parse_expected(f.read())

        # Run binary with commands file as argument
        result = subprocess.run(
            [binary, commands_file], capture_output=True, text=True, timeout=10
        )
        actual_lines = [line.rstrip() for line in result.stdout.strip().splitlines()]

        if result.returncode != 0:
            if result.returncode < 0:
                reason = f"Binary terminated by signal {-result.returncode} (e.g. abort/segfault)"
            else:
                reason = f"Binary exited with non-zero code {result.returncode}"
            if xfail is not None:
                if actual_lines == expected_lines:
                    return "xfail", f"Expected failure: {xfail}"
                diff = difflib.unified_diff(
                    expected_lines,
                    actual_lines,
                    fromfile="expected",
                    tofile="actual (before failure)",
                    lineterm="",
                )
                return "fail", (
                    f"Output before expected failure differs ({xfail}):\n"
                    + "\n".join(diff)
                )
            parts = [reason]
            if result.stderr.strip():
                parts.append("--- stderr ---\n" + result.stderr.strip())
            if result.stdout.strip():
                parts.append("--- stdout before exit ---\n" + result.stdout.strip())
            return "fail", "\n".join(parts)

        if xfail is not None:
            # Binary succeeded though failure was expected — flag so the directive gets removed.
            return (
                "xpass",
                f"Unexpectedly passed; remove the `// xfail` directive ({xfail})",
            )

        if actual_lines == expected_lines:
            return "pass", "Passed"
        else:
            diff = difflib.unified_diff(
                expected_lines,
                actual_lines,
                fromfile="expected",
                tofile="actual",
                lineterm="",
            )
            return "fail", "\n".join(diff)
    except subprocess.TimeoutExpired:
        return "fail", "Timeout (10s)"
    except Exception as e:
        return "fail", str(e)


def main():
    parser = argparse.ArgumentParser(
        description="Black-box test runner for sw_battle_test"
    )
    parser.add_argument(
        "--binary",
        help="Path to the binary (searches build/debug or build/release if omitted)",
    )
    parser.add_argument(
        "--tests", default="../tests", help="Path to the tests directory"
    )
    args = parser.parse_args()

    # Resolve paths relative to the script location or use absolute paths if provided
    script_dir = os.path.dirname(os.path.abspath(__file__))

    candidates = [
        os.path.abspath(args.binary) if args.binary else None,
        os.path.abspath(os.path.join(script_dir, "../build/debug/sw_battle_test")),
        os.path.abspath(os.path.join(script_dir, "../build/release/sw_battle_test")),
    ]
    binary_path = candidates[0] or next(
        (c for c in candidates[1:] if os.path.exists(c)), candidates[1]
    )
    tests_root = (
        os.path.abspath(args.tests)
        if os.path.exists(args.tests)
        else os.path.abspath(os.path.join(script_dir, args.tests))
    )

    if not os.path.exists(binary_path):
        print(f"Error: Binary not found at {binary_path}")
        sys.exit(1)

    if not os.path.exists(tests_root):
        print(f"Error: Tests directory not found at {tests_root}")
        sys.exit(1)

    test_dirs = sorted(
        [
            os.path.join(tests_root, d)
            for d in os.listdir(tests_root)
            if os.path.isdir(os.path.join(tests_root, d))
        ]
    )

    if not test_dirs:
        print(f"No test cases found in {tests_root}")
        sys.exit(0)

    def get_relative_time(path):
        m = int((time.time() - os.path.getmtime(path)) / 60)
        if m < 1:
            return "just now"
        if m < 60:
            return f"{m} min ago"
        return f"{m // 60}h {m % 60}m ago" if m < 1440 else f"{m // 1440} days ago"

    print(f"Running tests from: {tests_root}")
    print(f"Using binary: {binary_path} ({get_relative_time(binary_path)})\n")

    passed = 0
    failed = 0
    xfailed = 0

    for test_dir in test_dirs:
        test_name = os.path.basename(test_dir)
        print(f"[{test_name}] ...", end=" ", flush=True)
        status, message = run_test(binary_path, test_dir)

        if status == "pass":
            print("PASSED")
            passed += 1
        elif status == "xfail":
            print(f"XFAIL ({message})")
            xfailed += 1
        elif status == "skip":
            print(f"SKIPPED ({message})")
        else:  # 'fail' or 'xpass'
            print("XPASS" if status == "xpass" else "FAILED")
            print("-" * 40)
            print(message)
            print("-" * 40)
            failed += 1

    summary = f"\nSummary: {passed} passed, {failed} failed"
    if xfailed:
        summary += f", {xfailed} xfailed"
    print(summary + ".")
    if failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
