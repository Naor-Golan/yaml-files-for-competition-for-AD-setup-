// TargetApp.c  (minimal)
// embed the secret string in the binary but do NOT print it.
// accepts "4E 41 4F 52" or "4E414F52"; on success sends STOP to the monitor pipe
// and prints a success message then waits for ENTER.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

volatile const char secret_hidden[] = "secret unlock code: 4E 41 4F 52"; // embedded but not printed

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) { s[--n] = '\0'; }
}

static void remove_spaces_and_upper(char *dst, const char *src, size_t dstsz) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 1 < dstsz; ++i) {
        if (!isspace((unsigned char)src[i])) {
            dst[j++] = (char)toupper((unsigned char)src[i]);
        }
    }
    dst[j] = '\0';
}

int main(void) {
    char buf[512];

    (void)secret_hidden; // keep the literal in the binary

    puts("========================================");
    puts(" The cortex has been hacked by a netrunner");
    puts(" Provide the code matrix");
    puts("========================================");
    puts("");
    printf("Enter code (type the hex bytes, spaces allowed):\n> ");

    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    trim_newline(buf);

    char compact[512];
    remove_spaces_and_upper(compact, buf, sizeof(compact));

    // expected compact hex
    if (strcmp(compact, "4E414F52") == 0) {
        puts("Cortex control regained, hack stopped.");
        // attempt to contact monitor via named pipe
        const char *pipename = "\\\\.\\pipe\\LabMonitorPipe_v1";
        if (WaitNamedPipeA(pipename, 1500)) {
            HANDLE h = CreateFileA(pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                const char msg[] = "STOP";
                DWORD wrote = 0;
                WriteFile(h, msg, (DWORD)strlen(msg), &wrote, NULL);
                CloseHandle(h);
                puts("Sent STOP to monitor.");
            } else {
                puts("Couldn't open monitor pipe. Monitor might not be running.");
            }
        } else {
            puts("Monitor pipe not available (timed out).");
        }
    } else {
        puts("Access denied. Try again.");
    }

    puts("Press ENTER to exit.");
    getchar();
    return 0;
}
