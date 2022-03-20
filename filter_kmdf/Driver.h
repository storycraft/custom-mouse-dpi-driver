#pragma once

#include <ntddk.h>
#include <wdf.h>

#include <ntstatus.h>

#include "Shared.h"
#include "Filter.h"
#include "Control.h"

WDFDEVICE ControlDevice;

DRIVER_INITIALIZE DriverEntry;

VOID
CleanupControlDevice();
