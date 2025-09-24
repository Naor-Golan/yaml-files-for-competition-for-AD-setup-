// MonitorApp.c (fixed)
// background watchdog (no console). launches TargetApp.exe, restarts if it exits,
// listens on \\.\pipe\LabMonitorPipe_v1 for "STOP".
// IMPORTANT CHANGE: when STOP is received, the monitor no longer forcibly terminates
// the target. It sets g_stop and waits for the target to exit naturally (with a grace).

#include <windows.h>
#include <stdio.h>

volatile LONG g_stop = 0;

// simple log to C:\Temp\monitor.log (best-effort)
static void log_line(const char *line) {
    FILE *f = fopen("C:\\Temp\\monitor.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetSystemTime(&st);
        fprintf(f, "%04d-%02d-%02dT%02d:%02d:%02dZ %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, line);
        fclose(f);
    }
}

DWORD WINAPI PipeThreadProc(LPVOID lpv) {
    (void)lpv;
    const char *pipename = "\\\\.\\pipe\\LabMonitorPipe_v1";
    while (!g_stop) {
        HANDLE hPipe = CreateNamedPipeA(
            pipename,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            512,
            512,
            0,
            NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            log_line("CreateNamedPipe failed.");
            Sleep(1000);
            continue;
        }

        log_line("Pipe server waiting for connection...");
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            CloseHandle(hPipe);
            continue;
        }

        log_line("Pipe client connected. Reading...");
        char buf[128] = {0};
        DWORD readBytes = 0;
        BOOL ok = ReadFile(hPipe, buf, sizeof(buf)-1, &readBytes, NULL);
        if (ok && readBytes > 0) {
            buf[readBytes] = '\0';
            for (DWORD i=0;i<readBytes;i++) if (buf[i]=='\r' || buf[i]=='\n') { buf[i]='\0'; break; }
            if (_stricmp(buf, "STOP") == 0) {
                log_line("Received STOP command on pipe.");
                InterlockedExchange(&g_stop, 1);
                // Do NOT terminate the child here; let main loop handle graceful shutdown.
                FlushFileBuffers(hPipe);
                DisconnectNamedPipe(hPipe);
                CloseHandle(hPipe);
                break;
            } else {
                log_line("Received unknown pipe command.");
            }
        } else {
            log_line("Pipe read failed or empty.");
        }

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    log_line("Pipe thread exiting.");
    return 0;
}

int start_target_and_wait(void) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const char *target = "TargetApp.exe";

    DWORD attrs = GetFileAttributesA(target);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        log_line("TargetApp.exe not found.");
        return -1;
    }

    char cmdline[512];
    snprintf(cmdline, sizeof(cmdline), "\"%s\"", target);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        log_line("CreateProcess failed for TargetApp.exe.");
        return -1;
    }

    char logbuf[128];
    snprintf(logbuf, sizeof(logbuf), "Started TargetApp (PID %lu).", (unsigned long)pi.dwProcessId);
    log_line(logbuf);

    // Wait for the child to exit OR if g_stop is set: wait a short grace period for child to exit naturally.
    while (1) {
        DWORD wr = WaitForSingleObject(pi.hProcess, 500);
        if (wr == WAIT_OBJECT_0) {
            DWORD ec = 0;
            GetExitCodeProcess(pi.hProcess, &ec);
            snprintf(logbuf, sizeof(logbuf), "TargetApp exited (code %lu).", (unsigned long)ec);
            log_line(logbuf);
            break;
        }
        if (g_stop) {
            // STOP received: give the child a grace period to exit on its own (e.g., 10 seconds),
            // so the user can read the message and press ENTER.
            log_line("g_stop set: giving target a grace period to exit naturally.");
            DWORD graceMs = 10000; // 10 seconds
            DWORD waited = 0;
            DWORD slice = 200;
            while (waited < graceMs) {
                DWORD wr2 = WaitForSingleObject(pi.hProcess, slice);
                if (wr2 == WAIT_OBJECT_0) {
                    DWORD ec2 = 0;
                    GetExitCodeProcess(pi.hProcess, &ec2);
                    snprintf(logbuf, sizeof(logbuf), "TargetApp exited during grace (code %lu).", (unsigned long)ec2);
                    log_line(logbuf);
                    break;
                }
                waited += slice;
            }
            // After grace period, if the process still exists, do NOT force-kill it.
            // We'll just close handles and let OS/Instructor decide. This prevents abrupt termination.
            if (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
                log_line("Grace expired; target still running. Leaving it running and exiting monitor.");
            }
            // close handles and return - this causes the monitor main loop to exit (since g_stop is set).
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
        // else continue waiting
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow) {
    (void)hInst; (void)hPrev; (void)lpCmd; (void)nCmdShow;
    CreateDirectoryA("C:\\Temp", NULL);
    log_line("Monitor starting.");

    HANDLE hPipeThread = CreateThread(NULL, 0, PipeThreadProc, NULL, 0, NULL);
    if (!hPipeThread) log_line("Failed to create pipe thread.");

    while (!g_stop) {
        int res = start_target_and_wait();
        if (g_stop) break;
        if (res == -1) {
            log_line("start_target_and_wait returned error; sleeping 5s before retry.");
            Sleep(5000);
        } else {
            Sleep(1000);
        }
    }

    log_line("Monitor exiting main loop.");
    if (hPipeThread) {
        // ensure pipe thread exits (it should when g_stop set or after brief wait)
        WaitForSingleObject(hPipeThread, 3000);
        CloseHandle(hPipeThread);
    }

    log_line("Monitor shutting down.");
    return 0;
}
