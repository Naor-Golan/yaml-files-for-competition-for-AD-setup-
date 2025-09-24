#!/usr/bin/env python3
import socket
import sys

server = "100.65.1.125"
port = 445  # SMB port

try:
    sock = socket.create_connection((server, port), timeout=5)
    sock.close()
    print("Scorecheck PASSED: SMB port is open and reachable")
    sys.exit(0)
except Exception as e:
    print(f"Scorecheck FAILED: {e}")
    sys.exit(1)
