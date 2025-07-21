#include "cdc.h"
#include <stdio.h>
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>

#pragma comment(lib, "setupapi.lib")

HANDLE hDevice;

BOOLEAN FindCdcDevice(VOID)
{
    DWORD i = 0;
    BOOL ret = CDC_FAILED;
    char portName[256];
    char portFile[64];
    char hardwareId[1024];
    SP_DEVINFO_DATA devInfoData = {0};
    HDEVINFO deviceInfoSet = NULL;

    deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVCLASS_PORTS,
        NULL,
        NULL,
        DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        printf("Error get list of CDC devices\r\n");
        return ret;
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &devInfoData); i++)
    {
        DWORD size = 0;
        DWORD dataType = 0;
        memset(hardwareId, 0, sizeof(hardwareId));

        // Get hardware id
        if (SetupDiGetDeviceRegistryPropertyA(
                deviceInfoSet,
                &devInfoData,
                SPDRP_HARDWAREID,
                &dataType,
                (PBYTE)hardwareId,
                sizeof(hardwareId),
                &size)
        )
        {
            if (strstr(hardwareId, VENDOR_ID) && strstr(hardwareId, PRODUCT_ID))
            {
                // Open registry to get COM port name
                HKEY hKey = SetupDiOpenDevRegKey(
                    deviceInfoSet,
                    &devInfoData,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DEV,
                    KEY_READ
                );

                if (hKey != INVALID_HANDLE_VALUE)
                {
                    DWORD type = 0;
                    DWORD portNameLen = sizeof(portName);
                    memset(portName, 0, portNameLen);

                    if (RegQueryValueExA(
                        hKey,
                        "PortName",
                        NULL,
                        &type,
                        (LPBYTE)portName,
                        &portNameLen) == ERROR_SUCCESS)
                    {
                        snprintf(portFile, sizeof(portFile), "\\\\.\\%s", portName);
                        
                        hDevice = CreateFileA(
                            portFile,
                            GENERIC_READ | GENERIC_WRITE,
                            0,    // no sharing
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                        );
                        
                        if (hDevice != INVALID_HANDLE_VALUE)
                        {
                            printf("\r\nCom port: %s\r\n", portName);
                            ret = CDC_SUCCESS;
                            break;
                        }

                        RegCloseKey(hKey);
                        SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return ret;
}

BOOL ConfigComState(VOID)
{
    DCB dcbSerialParams = { 0 };
    COMMTIMEOUTS timeouts = { 0 };

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hDevice, &dcbSerialParams)) {
        printf("Error getting COM state\r\n");
        CloseHandle(hDevice);
        return CDC_FAILED;
    }

    dcbSerialParams.BaudRate = BAUD_RATE;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hDevice, &dcbSerialParams)) {
        printf("Error setting COM state\r\n");
        CloseHandle(hDevice);
        return CDC_FAILED;
    }

    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hDevice, &timeouts)) {
        printf("Error setting timeouts\n");
        CloseHandle(hDevice);
        return CDC_FAILED;
    }

    return CDC_SUCCESS;
}