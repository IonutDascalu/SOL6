#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LIMITA 1000

int main(void)
{
    DWORD pid = GetCurrentProcessId();
    srand((unsigned)time(NULL) ^ pid);

    /* Memoria partajata (initializata automat cu 0 de sistem) */
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                     0, sizeof(LONG), "Local\\NumaratorMem");
    if (!hMap) { printf("CreateFileMapping a esuat: %lu\n", GetLastError()); return 1; }

    volatile LONG *mem = (volatile LONG *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS,
                                                        0, 0, sizeof(LONG));
    if (!mem) { printf("MapViewOfFile a esuat: %lu\n", GetLastError()); return 1; }

    /* Semafor binar (initial liber, maxim 1) - acelasi nume in ambele procese */
    HANDLE hSem = CreateSemaphoreA(NULL, 1, 1, "Local\\NumaratorSem");
    if (!hSem) { printf("CreateSemaphore a esuat: %lu\n", GetLastError()); return 1; }

    for (;;)
    {
        WaitForSingleObject(hSem, INFINITE);      /* rezerv semaforul */

        LONG n = *mem;                            /* citesc memoria */

        if (n >= LIMITA) {                        /* gata, am ajuns la 1000 */
            ReleaseSemaphore(hSem, 1, NULL);
            break;
        }

        /* dau cu banul; cat timp pica "2" (valoarea 1), scriu urmatorul numar */
        while (n < LIMITA && rand() % 2 == 1) {
            n++;
            *mem = n;
            printf("[PID %lu] a scris %ld\n", pid, n);
        }

        ReleaseSemaphore(hSem, 1, NULL);          /* eliberez semaforul */
        Sleep(1);                                 /* las si celuilalt proces o sansa */
    }

    printf("[PID %lu] terminat.\n", pid);
    UnmapViewOfFile((LPCVOID)mem);
    CloseHandle(hMap);
    CloseHandle(hSem);
    return 0;
}