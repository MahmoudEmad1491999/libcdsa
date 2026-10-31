#!/usr/bin/env python3
"""
Generate a Markdown module table from Doxygen-annotated headers in include/cdsa/.

Each header's @file @brief becomes the row description; rows are sorted
alphabetically by module name.

Usage:
  python3 scripts/gen_module_table.py                 # print table to stdout
  python3 scripts/gen_module_table.py --update-readme # splice into README.md
  python3 scripts/gen_module_table.py --check         # exit 1 if README is stale
"""

import argparse
import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INCLUDE_DIR = os.path.join(REPO_ROOT, "include", "cdsa")
README_PATH = os.path.join(REPO_ROOT, "README.md")

# Matches the ## Modules section body (everything up to next ## heading or EOF).
_SECTION_RE = re.compile(
    r"(^## Modules\n)(.*?)(?=^## |\Z)",
    re.MULTILINE | re.DOTALL,
)


def _extract_file_brief(content: str) -> str:
    """Return the @brief text from the @file Doxygen block, or empty string."""
    block_re = re.compile(r"/\*\*(.*?)\*/", re.DOTALL)
    for block in block_re.finditer(content):
        text = block.group(1)
        if "@file" not in text:
            continue
        lines = text.split("\n")
        in_brief = False
        brief_parts: list[str] = []
        for line in lines:
            # Strip leading " * " Doxygen decoration.
            clean = re.sub(r"^\s*\*\s?", "", line)
            if re.match(r"@brief\s+", clean):
                in_brief = True
                brief_parts.append(clean[len("@brief "):].strip())
            elif in_brief:
                # Stop at a blank Doxygen line or next @tag.
                if not clean.strip() or re.match(r"@\w+", clean.strip()):
                    break
                brief_parts.append(clean.strip())
        if brief_parts:
            return " ".join(brief_parts)
    return ""


def _parse_headers(include_dir: str) -> list[dict]:
    """Return sorted list of module dicts parsed from all *.h files."""
    modules = []
    for fname in sorted(os.listdir(include_dir)):
        if not fname.endswith(".h"):
            continue
        abs_path = os.path.join(include_dir, fname)
        rel_path = os.path.relpath(abs_path, REPO_ROOT)
        with open(abs_path, encoding="utf-8") as f:
            content = f.read()
        brief = _extract_file_brief(content)
        modules.append({
            "name": fname[:-2],  # strip .h
            "header": rel_path,
            "brief": brief,
        })
    return modules


def _generate_table(modules: list[dict]) -> str:
    """Build the Modules section body (table rows only, no heading)."""
    if not modules:
        return "_No modules yet._\n"
    rows = [
        "| Module | Description |",
        "|--------|-------------|",
    ]
    for m in modules:
        name = f"[`{m['name']}`]({m['header']})"
        brief = m["brief"] or "_No description._"
        rows.append(f"| {name} | {brief} |")
    return "\n".join(rows) + "\n"


def _read_readme() -> str:
    with open(README_PATH, encoding="utf-8") as f:
        return f.read()


def _write_readme(content: str) -> None:
    with open(README_PATH, "w", encoding="utf-8") as f:
        f.write(content)


def _splice_section(readme: str, new_body: str) -> str:
    """Replace the ## Modules section body; append the section if absent."""
    replacement = f"## Modules\n\n{new_body}\n"
    updated, n = _SECTION_RE.subn(lambda _m: replacement, readme)
    if n == 0:
        updated = readme.rstrip("\n") + f"\n\n## Modules\n\n{new_body}"
    return updated


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--update-readme", action="store_true",
                    help="Splice the generated table into README.md in place.")
    ap.add_argument("--check", action="store_true",
                    help="Exit 1 if the README Modules section is stale.")
    args = ap.parse_args()

    modules = _parse_headers(INCLUDE_DIR)
    table = _generate_table(modules)

    if args.check:
        readme = _read_readme()
        m = _SECTION_RE.search(readme)
        current = m.group(2).strip() if m else ""
        expected = table.strip()
        if current != expected:
            print(
                "README Modules section is stale.\n"
                "Run: python3 scripts/gen_module_table.py --update-readme",
                file=sys.stderr,
            )
            sys.exit(1)
        print("README Modules section is up to date.")
        return

    if args.update_readme:
        readme = _read_readme()
        updated = _splice_section(readme, table)
        _write_readme(updated)
        print(f"Updated {README_PATH}")
        return

    print(table, end="")


if __name__ == "__main__":
    main()
