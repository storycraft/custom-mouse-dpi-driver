#pragma once

#include <ntddk.h>
#include <wdf.h>

#include <ntstatus.h>
#include <kbdmou.h>
#include <ntddmou.h>

#include "Shared.h"

typedef struct _FILTER_EXTENSION
{
    CONNECT_DATA UpperConnectData;
    // ...
} FILTER_EXTENSION, *PFILTER_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_EXTENSION, FilterGetData)

EVT_WDF_DRIVER_DEVICE_ADD FilterEventDeviceAdd;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL FilterEventInternalIoDeviceControl;
EVT_WDF_DEVICE_CONTEXT_CLEANUP FilterEventDeviceCleanup;

VOID
DispatchPassThrough(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
);

VOID
FilterServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
);
