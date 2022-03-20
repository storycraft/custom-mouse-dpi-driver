#include <stdio.h>
#include <Windows.h>
#include <winioctl.h>
#include <ioapiset.h>
#include "Shared.h"
#include <string.h>

int main(int argc, char **argv) {
	HANDLE device = CreateFile(
		L"\\\\.\\CMDD_1",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD error = GetLastError();
	if (error) {
		printf("Cannot open CMDD Control Device. status: 0x%x\n", error);
		return 1;
	}

	CMDD_MOUSE_CONFIG config;
	if (argc >= 4) {
		config.OriginalDPI = atoi(argv[1]);
		config.TargetDPIX = atoi(argv[2]);
		config.TargetDPIY = atoi(argv[3]);

		if (!DeviceIoControl(device, IOCTL_CMDD_SET_CONFIG, &config, sizeof(CMDD_MOUSE_CONFIG), NULL, NULL, NULL, NULL)) {
			printf("DeviceIoControl failed. status: 0x%x\n", GetLastError());
			return 1;
		}

		printf("Mouse Settings Updated.\n\n");
	}

	if (!DeviceIoControl(device, IOCTL_CMDD_GET_CONFIG, NULL, NULL, &config, sizeof(CMDD_MOUSE_CONFIG), NULL, NULL)) {
		printf("DeviceIoControl failed. status: 0x%x\n", GetLastError());
		return 1;
	}

	printf("==== Current Mouse Settings ====\n");
	printf("OriginalDPI: %d\n", config.OriginalDPI);
	printf("TargetDPIX: %d\n", config.TargetDPIX);
	printf("TargetDPIY: %d\n", config.TargetDPIY);

	return 0;
}
