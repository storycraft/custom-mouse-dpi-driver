#pragma once

#include <ntddk.h>
#include <wdf.h>

#include "Shared.h"

typedef struct _CONTROL_EXTENSION
{
    CMDD_MOUSE_CONFIG Config;
    // ...
} CONTROL_EXTENSION, *PCONTROL_EXTENSION;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ControlEventIoDeviceControl;
EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION EventControlDeviceShutdownNotification;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_EXTENSION, ControlGetData)

DECLARE_GLOBAL_CONST_UNICODE_STRING(REG_ORIGINAL_DPI_NAME, L"OriginalDPI");
DECLARE_GLOBAL_CONST_UNICODE_STRING(REG_TARGET_DPI_X_NAME, L"TargetDPIX");
DECLARE_GLOBAL_CONST_UNICODE_STRING(REG_TARGET_DPI_Y_NAME, L"TargetDPIY");

NTSTATUS
InitControlDevice(
    IN WDFDRIVER Driver,
    OUT WDFDEVICE *Device
);

// Try loading config from driver parameters registry.
// If some keys are not present, values will not be updated.
NTSTATUS
LoadControlMouseConfig(
    IN WDFDRIVER Driver,
    OUT CMDD_MOUSE_CONFIG *config
);

// Try saving config to driver parameters registry
NTSTATUS
SaveControlMouseConfig(
    IN WDFDRIVER Driver,
    IN CMDD_MOUSE_CONFIG config
);
