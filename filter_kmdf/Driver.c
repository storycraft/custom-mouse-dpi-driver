#include "Driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, CleanupControlDevice)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
) {
    WDF_DRIVER_CONFIG     config;
    NTSTATUS              status;
    WDFDRIVER             driver;

    KdPrint(("Custom Mouse Filter driver\n"));
    KdPrint(("Built %s %s\n", __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT(&config, FilterEventDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        &driver
    );
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed with status 0x%x\n", status));
    }

    status = InitControlDevice(driver, &ControlDevice);
    if (!NT_SUCCESS(status)) {
        KdPrint(("InitControlDevice failed with status 0x%x\n", status));
    }

    return status;
}

// Remove Control device on cleanup
VOID
CleanupControlDevice() {
    WdfObjectDelete(ControlDevice);
    ControlDevice = NULL;
}
