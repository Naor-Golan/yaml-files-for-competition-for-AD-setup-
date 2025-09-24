#!/usr/bin/env python3

# Naor Golan
# Requires: pysmb, ldap3  (install inside venv if system packages are locked)

from smb.SMBConnection import SMBConnection
from ldap3 import Server, Connection, ALL
import sys

server = "100.65.1.125"    # cortex IP
share = "Public"           # SMB share name
username = "Administrator"
password = "Naor1998"
domain   = "NETRUNNER"     # Your AD domain NetBIOS name
basedn   = "DC=net,DC=runner"  # base DN for bind (adjust if your domain changes)

def check_smb():
    try:
        conn = SMBConnection(username, password, "scorechecker", "cortex",
                             domain=domain, use_ntlm_v2=True, is_direct_tcp=True)
        connected = conn.connect(server, 445, timeout=5)
        conn.close()
        return connected
    except Exception:
        return False

def check_ldap():
    try:
        ldap_server = Server(server, port=389, get_info=ALL)
        # Use UPN style user@domain for LDAP bind
        ldap_user = f"{username}@net.runner"
        conn = Connection(ldap_server, user=ldap_user, password=password, auto_bind=True)
        conn.unbind()
        return True
    except Exception:
        return False

if __name__ == "__main__":
    smb_ok = check_smb()
    ldap_ok = check_ldap()

    if smb_ok and ldap_ok:
        sys.exit(0)   # success if BOTH checks work
    else:
        sys.exit(1)   # fail if either check fails

