#include "types.h"

#define RTC_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x0C
#define RTC_REG_D 0x8D
#define RTC_IRQ 8

/* RTC init and handler */
extern void rtc_init(void);
extern void rtc_handler(void);
