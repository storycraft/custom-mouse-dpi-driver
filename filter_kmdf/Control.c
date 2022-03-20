#include "Control.h"
#include "Shared.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitControlDevice)
#pragma alloc_text (PAGE, EventControlDeviceShutdownNotification)
#pragma alloc_text (PAGE, ControlEventIoDeviceControl)
#endif

NTSTATUS
InitControlDevice(
    IN WDFDRIVER Driver,
    OUT WDFDEVICE *Device
) {
    // Declare names for control device
    DECLARE_CONST_UNICODE_STRING(CONTROL_DEVICE_NAME, L"\\Device\\CMDD_1");
    DECLARE_CONST_UNICODE_STRING(SYM_NAME, L"\\DosDevices\\CMDD_1");

    NTSTATUS              status = STATUS_SUCCESS;
    PWDFDEVICE_INIT       controlDeviceInit;
    WDF_OBJECT_ATTRIBUTES controlDeviceAttributes;
    WDF_IO_QUEUE_CONFIG   ioQueueConfig;
    PCONTROL_EXTENSION    controlExt;
    WDFDEVICE             device;

    PAGED_CODE();

    controlDeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);
    WdfDeviceInitSetExclusive(controlDeviceInit, FALSE);
    WdfDeviceInitAssignName(controlDeviceInit, &CONTROL_DEVICE_NAME);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&controlDeviceAttributes, CONTROL_EXTENSION);

    WdfControlDeviceInitSetShutdownNotification(controlDeviceInit, EventControlDeviceShutdownNotification, WdfDeviceShutdown);

    status = WdfDeviceCreate(&controlDeviceInit, &controlDeviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreate failed with status code 0x%x\n", status));
        return status;
    }

    WdfDeviceCreateSymbolicLink(device, &SYM_NAME);

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoDeviceControl = ControlEventIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &ioQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    controlExt = ControlGetData(device);

    CMMD_MOUSE_CONFIG config = {
        CMMD_DEFAULT_CONFIG_ORIGINAL_DPI,
        CMMD_DEFAULT_CONFIG_TARGET_DPI_X,
        CMMD_DEFAULT_CONFIG_TARGET_DPI_Y
    };

    status = LoadControlMouseConfig(Driver, &config);
    if (!NT_SUCCESS(status)) {
        KdPrint(("LoadControlMouseConfig failed %x\n", status));
        return status;
    }

    controlExt->Config = config;

    WdfControlFinishInitializing(device);

    *Device = device;

    return status;
}

VOID
EventControlDeviceShutdownNotification(
    IN WDFDEVICE Device
) {
    NTSTATUS              status = STATUS_SUCCESS;
    PCONTROL_EXTENSION    controlExt;

    PAGED_CODE();

    controlExt = ControlGetData(Device);

    status = SaveControlMouseConfig(WdfDeviceGetDriver(Device), controlExt->Config);
    if (!NT_SUCCESS(status)) {
        KdPrint(("SaveControlMouseConfig failed %x\n", status));
        return;
    }
}

VOID
ControlEventIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
) {
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    PCONTROL_EXTENSION  controlExt;
    NTSTATUS           status = STATUS_SUCCESS;
    WDFDEVICE          device;
    PCMMD_MOUSE_CONFIG config;

    PAGED_CODE();

    device = WdfIoQueueGetDevice(Queue);
    controlExt = ControlGetData(device);

    switch (IoControlCode) {
    case IOCTL_CMMD_GET_CONFIG: {
        status = WdfRequestRetrieveOutputBuffer(
            Request,
            sizeof(CMMD_MOUSE_CONFIG),
            &config,
            NULL
        );
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfRequestRetrieveOutputBuffer failed %x\n", status));
            break;
        }

        *config = controlExt->Config;

        WdfRequestCompleteWithInformation(Request, status, sizeof(CMMD_MOUSE_CONFIG));
        return;
    }

    case IOCTL_CMMD_SET_CONFIG: {
        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(CMMD_MOUSE_CONFIG),
            &config,
            NULL
        );
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfRequestRetrieveInputBuffer failed %x\n", status));
            break;
        }

        // Zero DPI config will not be accepted
        if (config->OriginalDPI == 0 || config->TargetDPIX == 0 || config->TargetDPIY == 0) {
            break;
        }

        controlExt->Config = *config;
        SaveControlMouseConfig(WdfDeviceGetDriver(device), controlExt->Config);

        WdfRequestComplete(Request, status);
        return;
    }

    default: break;
    };

    WdfRequestComplete(Request, status);
}

NTSTATUS
LoadControlMouseConfig(
    IN WDFDRIVER Driver,
    OUT CMMD_MOUSE_CONFIG* config
) {
    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY   parameterKey;

    status = WdfDriverOpenParametersRegistryKey(
        Driver,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &parameterKey
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverOpenParametersRegistryKey failed 0x%x\n", status));
        return status;
    }

    ULONG res;
    if (NT_SUCCESS(WdfRegistryQueryULong(parameterKey, &REG_ORIGINAL_DPI_NAME, &res))) {
        config->OriginalDPI = (int)res;
    }

    if (NT_SUCCESS(WdfRegistryQueryULong(parameterKey, &REG_TARGET_DPI_X_NAME, &res))) {
        config->TargetDPIX = (int)res;
    }

    if (NT_SUCCESS(WdfRegistryQueryULong(parameterKey, &REG_TARGET_DPI_Y_NAME, &res))) {
        config->TargetDPIY = (int)res;
    }

    WdfRegistryClose(parameterKey);

    return status;
}

NTSTATUS
SaveControlMouseConfig(
    IN WDFDRIVER Driver,
    IN CMMD_MOUSE_CONFIG config
) {
    NTSTATUS status = STATUS_SUCCESS;
    WDFKEY   parameterKey;

    status = WdfDriverOpenParametersRegistryKey(
        Driver,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &parameterKey
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverOpenParametersRegistryKey failed 0x%x\n", status));
        return status;
    }

    ULONG input;

    input = (ULONG)config.OriginalDPI;
    status = WdfRegistryAssignULong(parameterKey, &REG_ORIGINAL_DPI_NAME, input);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfRegistryAssignULong for REG_ORIGINAL_DPI_NAME failed 0x%x\n", status));
        WdfRegistryClose(parameterKey);
        return status;
    }

    input = (ULONG)config.TargetDPIX;
    status = WdfRegistryAssignULong(parameterKey, &REG_TARGET_DPI_X_NAME, input);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfRegistryAssignULong for REG_TARGET_DPI_X_NAME failed 0x%x\n", status));
        WdfRegistryClose(parameterKey);
        return status;
    }

    input = (ULONG)config.TargetDPIY;
    status = WdfRegistryAssignULong(parameterKey, &REG_TARGET_DPI_Y_NAME, input);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfRegistryAssignULong for REG_TARGET_DPI_Y_NAME failed 0x%x\n", status));
        WdfRegistryClose(parameterKey);
        return status;
    }

    WdfRegistryClose(parameterKey);

    return status;
}
