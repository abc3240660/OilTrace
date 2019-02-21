#ifndef __IAP_H__
#define __IAP_H__

#include "delay.h"

#define FLASH_BAK_ADDR		0x0804B000

void download_success(void);
void iap_write_appbin(u32 appxaddr, u8 *appbuf, u32 applen);

#endif







































