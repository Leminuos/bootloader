#define _CRT_SECURE_NO_WARNINGS

#include "hid.h"
#include <cfgmgr32.h>
#include <hidsdi.h>
#include <time.h>
#include <setupapi.h>
#include <string.h>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

HANDLE hDevice;

BOOLEAN FindHidDevice(VOID)
{
    GUID hidguid;
    DWORD index = 0;
    DWORD requiredSize = 0;
    HANDLE FindDevice = NULL;
    LPCWSTR filePath = NULL;
    HDEVINFO deviceInfoSet = NULL;
    HIDD_ATTRIBUTES attributes = { 0 };
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetailData = NULL;

    attributes.Size = sizeof(HIDD_ATTRIBUTES);
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    HidD_GetHidGuid(&hidguid);

    // Lấy danh sách thiết bị HID hiện có
    deviceInfoSet = SetupDiGetClassDevs(
        &hidguid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("Lỗi khi lấy danh sách thiết bị HID.\n");
        return 1;
    }

    // Duyệt qua từng thiết bị
    while (SetupDiEnumDeviceInterfaces(
        deviceInfoSet,
        NULL,
        &hidguid,
        index,
        &deviceInterfaceData)
        )
    {
        // Lấy kích thước cần thiết cho chi tiết giao diện
        SetupDiGetDeviceInterfaceDetail(
            deviceInfoSet,
            &deviceInterfaceData,
            NULL,
            0,
            &requiredSize,
            NULL
        );

        deviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);

        if (deviceDetailData == NULL)
        {
            printf("Allocate failed\n");
            return 1;
        }

        deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(
            deviceInfoSet,
            &deviceInterfaceData,
            deviceDetailData,
            requiredSize,
            NULL,
            NULL)
            )
        {
            // Mở thiết bị
            FindDevice = CreateFile(
                deviceDetailData->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );

            if (FindDevice != INVALID_HANDLE_VALUE) {
                if (HidD_GetAttributes(FindDevice, &attributes)) {
                    if (attributes.VendorID == VENDOR_ID && attributes.ProductID == PRODUCT_ID)
                    {
                        filePath = (LPCWSTR)malloc(wcslen(deviceDetailData->DevicePath) * sizeof(wchar_t));
                        if (filePath) wcscpy(filePath, deviceDetailData->DevicePath);
                        hDevice = FindDevice;
                        break;
                    }
                }

                CloseHandle(FindDevice);
            }
        }

        free(deviceDetailData);
        index++;
    }

    // Giải phóng tài nguyên
    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return 0;
}
