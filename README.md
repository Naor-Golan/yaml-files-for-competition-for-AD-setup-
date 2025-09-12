## CSEC473 — Ansible (Active Directory + SMB Share)

**Name:** Naor Golan  
**Team & Semester:** Delta (Gray Team) / Fall 2025  
**Date:** September 11, 2025

---

### What This Does
Three tiny playbooks to spin up a Windows lab:

1. Promote a Windows Server to a **Domain Controller** (new forest; no explicit DNS role here).  
2. Create an **SMB share**.  
3. Populate the share with a **randomized text file** to confirm automation works.

---

### Files
- `install_dc.yml` – install AD DS and promote to a new forest.  
- `create_SMB_share.yml` – ensure `C:\Cyber_Shard` exists and publish **CyberShard**.  
- `populate_share.yml` – write a randomized `file.txt` into `C:\Cyber_Shard`.  
- `win_inventory.ini` – WinRM inventory (edit IP/creds).

---

### Requirements
•	Control host: Ansible (2.15+ recommended) 

•	Windows target: Windows Server reachable over WinRM (TCP 5985) with supplied local Administrator credentials (REDACTED).

•	Windows target: disabling firewall options. 

•	Privileges: Ability to install roles/features and create shares on the Windows host

