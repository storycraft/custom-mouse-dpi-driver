#pragma once

// Use ntddk.h for Kernel mode application
// Use winioctl.h for User mode application

// Retrive config from driver
#define IOCTL_CMMD_GET_CONFIG CTL_CODE(FILE_DEVICE_MOUSE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Update driver config
#define IOCTL_CMMD_SET_CONFIG CTL_CODE(FILE_DEVICE_MOUSE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// DPI Config for driver
typedef struct _CMMD_MOUSE_CONFIG {
	// Original DPI. Must not be zero
	int OriginalDPI;

	// Target DPI for X axis. Must not be zero
	int TargetDPIX;

	// Target DPI for Y axis. Must not be zero
	int TargetDPIY;
} CMMD_MOUSE_CONFIG, *PCMMD_MOUSE_CONFIG;

#define CMMD_DEFAULT_CONFIG_ORIGINAL_DPI 1000
#define CMMD_DEFAULT_CONFIG_TARGET_DPI_X 1000
#define CMMD_DEFAULT_CONFIG_TARGET_DPI_Y 1000
