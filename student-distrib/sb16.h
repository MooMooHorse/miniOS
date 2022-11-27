#ifndef _SB16_H
#define _SB16_H

#define SB16_PAGE_ADDRESS        0x10000
#define SB16_DATA_LENGTH         0x10000
#define SB16_IRQ                6

#define SB16_MIXER_PORT         0x224
#define SB16_MIXER_DATA_PORT    0x225
#define SB16_RESET_PORT         0x226
#define SB16_READ_PORT          0x22A
#define SB16_WRITE_PORT         0x22C
#define SB16_READ_STATUS_PORT   0x22E
#define SB16_INT_ACK_PORT       0x22F // for DSP v4.0+, check necessity

#define SB16_SET_TIME_CONST     0x40
#define SB16_SET_SAMPLE_RATE    0x41
#define SB16_SPEAKER_ENABLE     0xD1
#define SB16_SPEAKER_DISABLE    0xD3
#define SB16_8_BIT_PAUSE        0xD0
#define SB16_8_BIT_RESUME       0xD4
#define SB16_16_BIT_PAUSE       0xD5
#define SB16_16_BIT_RESUME      0xD6
#define SB16_DSP_VERSION        0xE1
#define SB16_MASTER_VOLUME      0x22

#define SB16_READY              0xAA