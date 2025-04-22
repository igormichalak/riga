#!/usr/bin/env python3

import sys
from pathlib import Path

def usage() -> None:
    exe = Path(sys.argv[0]).name
    print(f"Usage: {exe} <binary_file>", file=sys.stderr)

def main() -> None:
    if len(sys.argv) != 2:
        usage()
        sys.exit(1)

    path = Path(sys.argv[1])
    try:
        with path.open("rb") as f:
            for b in iter(lambda: f.read(1), b""):
                print(f"{b[0]:02x}")
    except FileNotFoundError:
        print(f"Error: Cannot open '{path}'.")
        sys.exit(1)

if __name__ == "__main__":
    main()
