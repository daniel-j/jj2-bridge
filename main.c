
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <tlhelp32.h>
#include <time.h>
#include <unistd.h> // for sleep()
#include <string.h>
//#include <locale.h>

#define JAZZ2123 0x004cd9f8
#define JAZZ2124 0x004c9b98

#define JAZZ2_VALUE_VERSION123 0x33322E31
#define JAZZ2_VALUE_VERSION124 0x34322E31

#define V123TOV124OFS 0x00023620

// const unsigned long GLOBAL_GAME_LOOP_RUNNING = 0x52A522;
#define JAZZ2LOCALPLAYERS 0x5A4D00

unsigned long vOffset = 0;
bool isTSF = false;
bool gameRunning = false;
HWND hWnd = 0;
DWORD pid = 0;
DWORD plusOffset = 0;
HANDLE phandle = 0;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

char* getline() {
	char *line = NULL, *tmp = NULL;
	size_t size = 0, index = 0;
	int ch = EOF;

	while (ch) {
		ch = getc(stdin);

		/* Check if we need to stop. */
		if (ch == EOF || ch == '\n')
			ch = 0;

		/* Check if we need to expand. */
		if (size <= index) {
			size += 20;
			tmp = realloc(line, size);
			if (!tmp) {
				free(line);
				line = NULL;
				break;
			}
			line = tmp;
		}

		/* Actually store the thing. */
		line[index++] = ch;
	}

	return line;
}

bool prefix(const char *pre, const char *str) {
	return strncmp(pre, str, strlen(pre)) == 0;
}

void checkRunning() {
	char v;
	gameRunning = false;
	unsigned long addr = vOffset + JAZZ2LOCALPLAYERS;
	ReadProcessMemory(phandle, (LPCVOID)addr, &v, 1, 0);
	if (v > 0) {
		gameRunning = true;
	}
}

//Get Remote Module Handle
//Example:GetRemoteModuleHandle(PID,"Game.dll");
HMODULE GetRemoteModuleHandle(unsigned long pId, char *module) {
	MODULEENTRY32 modEntry;
	HANDLE tlh = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pId);

	modEntry.dwSize = sizeof(MODULEENTRY32);
	Module32First(tlh, &modEntry);

	do {
		if(!stricmp(modEntry.szModule, module))
			return modEntry.hModule;
		modEntry.dwSize = sizeof(MODULEENTRY32);
	}
	while (Module32Next(tlh, &modEntry));

	return NULL;
}

bool openJJ2(HWND hwnd) {

	GetWindowThreadProcessId(hwnd, &pid);
	plusOffset = (DWORD)GetRemoteModuleHandle(pid, "plus.dll");
	//printf("plusOffset: %#08x\r\n", (unsigned int)plusOffset);
	phandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (!phandle) {
		printf("BRIDGE: Could not get process handle!\r\n");
		return false;
	}
	else {
		int v;

		ReadProcessMemory(phandle, (LPCVOID)JAZZ2123, (LPVOID)&v, 4, 0);
		if (v == JAZZ2_VALUE_VERSION123) {
			vOffset = 0;
			isTSF = false;
			printf("BRIDGE: Found JJ2 1.23!\r\n");
			checkRunning();
			return true;
		}

		ReadProcessMemory(phandle, (LPCVOID)JAZZ2124, (LPVOID)&v, 4, 0);
		if (v == JAZZ2_VALUE_VERSION124) {
			vOffset = V123TOV124OFS;
			isTSF = true;
			printf("BRIDGE: Found JJ2 TSF!\r\n");
			checkRunning();
			return true;
		}

		return false;
	}
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {

	char class_name[11];
	char title[18];
	GetClassName(hwnd,class_name, sizeof(class_name));
	GetWindowText(hwnd,title,sizeof(title));
	// printf("Class name: %s\r\n", class_name);
	// printf("Window title: %s\r\n", title);

	if (strcmp(class_name, "DDWndClass") == 0 && strcmp(title, "Jazz Jackrabbit 2") == 0) {
		bool success = openJJ2(hwnd);
		if (success) {
			hWnd = hwnd;
			return false;
		}
	}

	return TRUE;
}

void sendChat(const char *message) {
	if (!phandle) return;
	char s[64];
	sprintf(s, "\x20%.62s", message);
	int dataLength = strlen(s);

	// printf("send chat:%s\r\n", s);

	LPVOID p = VirtualAllocEx(phandle, 0, dataLength+1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(phandle, p, s, dataLength+1, 0);

	int adr = vOffset==0 ? 0x483DE0 : 0x4833A0;

	HANDLE t = CreateRemoteThread(phandle, 0, 0, (LPVOID)adr, p, 0, 0);

	WaitForSingleObject(t, INFINITE);

	VirtualFreeEx(phandle, p, 0, MEM_RELEASE);
}

void closeWindow() {
	PostMessage(hWnd, WM_CLOSE, 0, 0);
}

int main (int argc, char *argv[]) {
	// setlocale(LC_ALL, "");
	// setlocale(LC_ALL, "en_US.utf8");

	printf("BRIDGE: Connecting to JJ2 process...\r\n");
	fflush(stdout);
	while (hWnd == 0) {
		EnumWindows(EnumWindowsProc, (LPARAM)NULL);
		fflush(stdout);
		sleep(1);
	}
	printf("BRIDGE: Connected, waiting for game to start...\r\n");
	fflush(stdout);

	do {
		sleep(1);
		checkRunning();
	} while (!gameRunning);
	sleep(2);
	printf("BRIDGE: Ready!\r\n");
	fflush(stdout);

	while (true) {
		char* s = getline();
		if (strlen(s) == 0) {
			continue;
		}
		/*const unsigned char* p = s;
		for (; *p; ++p) {
			printf("%02x %d %02x\r\n", *p, *p, *p & 255);
		}
		printf("%s\r\n", s);*/
		if (prefix("/close", (char*)s)) {
			printf("BRIDGE: Closing JJ2 process...\r\n");
			fflush(stdout);
			closeWindow();
			break;
		}
		sendChat((char*)s);
		fflush(stdout);
	}
	fflush(stdout);

	return 0;
}
