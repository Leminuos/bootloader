#ifndef __HID_H__
#define __HID_H__

#include <windows.h>

#define VENDOR_ID                   0x1234
#define PRODUCT_ID                  0x5678

#define HID_SUCCESS					0x00
#define HID_FAILED					0x01

extern HANDLE hDevice;
extern BOOLEAN FindHidDevice(VOID);

#endif /* __HID_H__ */

