#ifndef __CDC_H__
#define __CDC_H__

#include <windows.h>

#define VENDOR_ID                   "VID_0483"
#define PRODUCT_ID                  "PID_5740"

#define CDC_SUCCESS                 0x00
#define CDC_FAILED                  0x01
#define BAUD_RATE                   CBR_115200

extern HANDLE hDevice;
extern BOOL ConfigComState(VOID);
extern BOOLEAN FindCdcDevice(VOID);

#endif /* __CDC_H__ */


