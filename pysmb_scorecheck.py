#!/usr/bin/env python3

#Naor Golan
#requires the pysmb library!!!

from smb.SMBConnection import SMBConnection
import sys

server = "100.65.1.125"    # cortex IP
share = "Public"           # SMB share name
username = "Administrator"
password = "Naor1998"
domain   = "NETRUNNER"     # Your AD domain NetBIOS name

try:
    conn = SMBConnection(username, password, "scorechecker", "cortex",
                         domain=domain, use_ntlm_v2=True, is_direct_tcp=True)
    connected = conn.connect(server, 445, timeout=5)
    conn.close()
    if connected:
        #print("Code 0")
        sys.exit(0)
except Exception as e:
    # Optional: print debug info
    #print("Code 1")
    print("Error:", e)
    sys.exit(1)

sys.exit(1)
