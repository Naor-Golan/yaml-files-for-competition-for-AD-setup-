#!/usr/bin/env python3
import subprocess
import sys

server = "100.65.1.125"
share = "Public"
username = "Administrator"
password = "Naor1998"

cmd = [
    "smbclient",
    f"//{server}/{share}",
    "-U", username + "%" + password,
    "-c", "ls"
]

try:
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
    if result.returncode == 0:
        print("Scorecheck PASSED: SMB share is accessible")
        sys.exit(0)
    else:
        print("Scorecheck FAILED:")
        print(result.stderr or result.stdout)
        sys.exit(1)
except Exception as e:
    print(f"Scorecheck FAILED: {e}")
    sys.exit(1)

