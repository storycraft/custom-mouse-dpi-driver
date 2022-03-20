#include "Filter.h"
#include "Driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, FilterEventDeviceAdd)
#pragma alloc_text (PAGE, FilterEventDeviceCleanup)
#pragma alloc_text (PAGE, FilterEventInternalIoDeviceControl)
#endif


// Device COUNTER
int filterDevices = 0;
WDFWAITLOCK FilterDeviceCounterLock = NULL;

#pragma warning(push)
#pragma warning(disable:4055) // type case from PVOID to PSERVICE_CALLBACK_ROUTINE
#pragma warning(disable:4152) // function/data pointer conversion in expression
NTSTATUS
FilterEventDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
) {
    UNREFERENCED_PARAMETER(Driver);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    NTSTATUS              status;
    WDFDEVICE             device;
    WDF_IO_QUEUE_CONFIG   ioQueueConfig;

    PAGED_CODE();

    WdfFdoInitSetFilter(DeviceInit);

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_MOUSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, FILTER_EXTENSION);
    deviceAttributes.EvtCleanupCallback = FilterEventDeviceCleanup;

    if (FilterDeviceCounterLock == NULL) {
        status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &FilterDeviceCounterLock);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfWaitLockCreate failed with status code 0x%x\n", status));
            return status;
        }
    }

    WdfWaitLockAcquire(FilterDeviceCounterLock, NULL);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreate failed with status code 0x%x\n", status));
        return status;
    }

    filterDevices++;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoInternalDeviceControl = FilterEventInternalIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &ioQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE // Default queue
    );
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    WdfWaitLockRelease(FilterDeviceCounterLock);

    return status;
}

VOID
FilterEventDeviceCleanup(
    WDFOBJECT Device
) {
    UNREFERENCED_PARAMETER(Device);

    if (FilterDeviceCounterLock == NULL) return;

    WdfWaitLockAcquire(FilterDeviceCounterLock, NULL);

    // If all filter devices cleaned up It is safe to cleanup control
    if (--filterDevices <= 0) {
        CleanupControlDevice();
    }

    WdfWaitLockRelease(FilterDeviceCounterLock);
}

VOID
DispatchPassThrough(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
) {
    WDF_REQUEST_SEND_OPTIONS options;
    BOOLEAN ret;
    NTSTATUS status = STATUS_SUCCESS;

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    ret = WdfRequestSend(Request, Target, &options);
    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        KdPrint(("WdfRequestSend failed: 0x%x\n", status));
        WdfRequestComplete(Request, status);
    }
}

VOID
FilterEventInternalIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
) {
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    PFILTER_EXTENSION filterExt;
    NTSTATUS          status = STATUS_SUCCESS;
    WDFDEVICE         device;
    PCONNECT_DATA     connectData;
    size_t            length;

    PAGED_CODE();

    device = WdfIoQueueGetDevice(Queue);
    filterExt = FilterGetData(device);

    switch (IoControlCode) {
        // Install filter when new mouse device connected
        case IOCTL_INTERNAL_MOUSE_CONNECT: {
            if (filterExt->UpperConnectData.ClassService != NULL) {
                status = STATUS_SHARING_VIOLATION;
                break;
            }

            status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(CONNECT_DATA),
                &connectData,
                &length
            );
            if (!NT_SUCCESS(status)) {
                KdPrint(("WdfRequestRetrieveInputBuffer failed %x\n", status));
                break;
            }

            filterExt->UpperConnectData = *connectData;

            connectData->ClassDeviceObject = WdfDeviceWdmGetDeviceObject(device);
            connectData->ClassService = FilterServiceCallback;

            break;
        }

        case IOCTL_INTERNAL_MOUSE_DISCONNECT: {
            status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        default: break;
    }

    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    DispatchPassThrough(Request, WdfDeviceGetIoTarget(device));
}

VOID
FilterServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
) {

    PFILTER_EXTENSION   filterExt;
    PCONTROL_EXTENSION  controlExt;
    WDFDEVICE           device;
    CMDD_MOUSE_CONFIG   config;

    static int xLeft = 0;
    static int yLeft = 0;

    device = WdfWdmDeviceGetWdfDeviceHandle(DeviceObject);
    filterExt = FilterGetData(device);
    controlExt = ControlGetData(ControlDevice);
    config = controlExt->Config;

    int lastX, lastY;
    for (PMOUSE_INPUT_DATA input = InputDataStart; input < InputDataEnd; ++input) {
        lastX = input->LastX;
        lastY = input->LastY;

        lastX = lastX * config.TargetDPIX + xLeft;
        lastY = lastY * config.TargetDPIY + yLeft;

        xLeft = lastX % config.OriginalDPI;
        yLeft = lastY % config.OriginalDPI;

        input->LastX = lastX / config.OriginalDPI;
        input->LastY = lastY / config.OriginalDPI;
    }

    // Pass modified data to original routine
    (*(PSERVICE_CALLBACK_ROUTINE) filterExt->UpperConnectData.ClassService)(
        filterExt->UpperConnectData.ClassDeviceObject,
        InputDataStart,
        InputDataEnd,
        InputDataConsumed
        );
}

#pragma warning(pop)