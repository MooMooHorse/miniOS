#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define SB16_PAGE_ADDRESS       0x120000    // memory address for SB16 DMA
#define SB16_CHUNK_LENGTH       0x10000      // size of data buffer for SB16 DMA
#define SB16_IRQ                5           // IRQ number for SB16

#define SB16_MIXER_PORT         0x224
#define SB16_MIXER_DATA_PORT    0x225
#define SB16_RESET_PORT         0x226
#define SB16_READ_PORT          0x22A
#define SB16_WRITE_PORT         0x22C
#define SB16_READ_STATUS_PORT   0x22E
#define SB16_INT_ACK_PORT       0x22F       // for DSP v4.0+, check necessity

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

#define SB16_READY              0xAA        // status code for DSP ready

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

typedef struct {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t format;
    uint32_t subchunk_1_id;
    uint32_t subchunk_1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t subchunk_2_id;
    uint32_t subchunk_2_size;
} wav_meta_t;



int main() {
    wav_meta_t wav_meta;
    int32_t sb16_fd, audio_fd;
    int32_t data_input_size;
    int32_t i;
    char data_input[SB16_CHUNK_LENGTH];
    char file_name[1024]; // used for file input, hardcoded file for now
    char debug_buf[1024];

    // obtain filename
    // if (0 != ece391_getargs((uint8_t)file_name, 1024)) {
    //     ece391_fdputs(1, (uint8_t*)"Error: no file name provided\n");
    //     return 2;
    // }

    //ece391_strcpy((uint8_t*)file_name, (uint8_t*)"stopandsmell8.wav");

    // obtain filename
    if (0 != ece391_getargs ((uint8_t)file_name, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 2;
    }
    // get file
    audio_fd = ece391_open((uint8_t*)file_name);

    if (audio_fd == -1) {
        ece391_fdputs(1, (uint8_t*)"file not found\n");
        return 2;
    }

    // initialize sb16
    sb16_fd = ece391_open((uint8_t*)"sb16");

    if (sb16_fd == -1) {
        ece391_fdputs(1, (uint8_t*)"sb16 not found\n");
        return 2;
    }

    // read in wave meta data
    ece391_read(audio_fd, &wav_meta, (int32_t)sizeof(wav_meta_t));

    // verify wave metadata
    if (0x46464952 != wav_meta.chunk_id // RIFF
        || 0x45564157 != wav_meta.format // WAVE
        || 0x20746d66 != wav_meta.subchunk_1_id // fmt
        || 0x10 != wav_meta.subchunk_1_size
        || 1 != wav_meta.audio_format
        || 0 == wav_meta.num_channels
        || 0x61746164 != wav_meta.subchunk_2_id // data
    ) {
        ece391_fdputs(1, (uint8_t*) "invalid wav file\n");
        return 2;
    }

    // HARDWARE LIMITATION CHECKS

    // ensure no more than 2 channels
    if (wav_meta.num_channels > 2 || wav_meta.num_channels < 1) {
        ece391_fdputs(1, (uint8_t*) "wav file has unplayable number of channels\n");
        return 2;
    }

    // sample rate upper bound check
    if (wav_meta.sample_rate > 44100 || wav_meta.sample_rate < 5000) {
        ece391_fdputs(1, (uint8_t*) "wav file has unplayable sample rate\n");
        return 2;
    }

    // bits per sample check
    if (wav_meta.bits_per_sample != 8 && wav_meta.bits_per_sample != 16) {
        ece391_fdputs(1, (uint8_t*) "wav file has unplayable bits depth\n");
        return 2;
    }

    ece391_fdputs(1, (uint8_t*)"Sample Rate: ");
    ece391_itoa(wav_meta.sample_rate, (uint8_t*)debug_buf, 10);
    ece391_fdputs(1, (uint8_t*)debug_buf);
    ece391_fdputs(1, (uint8_t*)"\n");

    ece391_fdputs(1, (uint8_t*)"Bits per Sample: ");
    ece391_itoa(wav_meta.bits_per_sample, (uint8_t*)debug_buf, 10);
    ece391_fdputs(1, (uint8_t*)debug_buf);
    ece391_fdputs(1, (uint8_t*)"\n");

    ece391_fdputs(1, (uint8_t*)"Number of Channels: ");
    ece391_itoa(wav_meta.num_channels,(uint8_t*)debug_buf, 10);
    ece391_fdputs(1, (uint8_t*)debug_buf);
    ece391_fdputs(1, (uint8_t*)"\n");

    // Load first data chunk
    data_input_size = ece391_read(audio_fd, data_input, SB16_CHUNK_LENGTH);

    // Check for read failure
    if (data_input_size <= 0) {
        ece391_fdputs(1, (uint8_t*)"file read failed\n");
        return 3;
    }

    // Pad as necessary
    for (i = data_input_size; i < SB16_CHUNK_LENGTH; i++) {
        data_input[i] = 0;
    }

    // Write to SB16
    if (data_input_size != ece391_write(sb16_fd, data_input, SB16_CHUNK_LENGTH)) {
        ece391_fdputs(1, (uint8_t*)"sb16 write failed\n");
        return 3;
    }

    // default sample rate and stuff for testing
    outb(SB16_SET_SAMPLE_RATE, SB16_WRITE_PORT);
    outb((uint8_t) (wav_meta.sample_rate >> 8) & 0xFF, SB16_WRITE_PORT);
    outb((uint8_t) wav_meta.sample_rate & 0xFF, SB16_WRITE_PORT);

    // causes syserr in qemu
    // "sb16: warning: command 0x42,2 is not truly understood yet"
    // outb(0x42, SB16_WRITE_PORT);
    // outb((uint8_t) (wav_meta.sample_rate >> 8) & 0xFF, SB16_WRITE_PORT);
    // outb((uint8_t) wav_meta.sample_rate & 0xFF, SB16_WRITE_PORT);

    // outb(0xB0, SB16_WRITE_PORT);
    // outb(0x10, SB16_WRITE_PORT);
    outb(0xC6, SB16_WRITE_PORT);
    outb(0x00, SB16_WRITE_PORT);
    outb((uint8_t) (SB16_CHUNK_LENGTH - 1) & 0xFF, SB16_WRITE_PORT); // L
    outb((uint8_t) ((SB16_CHUNK_LENGTH - 1) >> 8) & 0xFF, SB16_WRITE_PORT); // H
    outb(0xD4, SB16_WRITE_PORT); // Continue 8-bit DMA mode digitized sound I/O paused using command D0.
    
    while (1) {
        ece391_fdputs(1, (uint8_t*)"READY TO LOAD NEW CHUNK...\n");

        // load next data chunk
        if (ece391_read(sb16_fd, 0, 0) == -1) {
            return 5;
        }
        

        ece391_fdputs(1, (uint8_t*)"LOADING NEW CHUNK...\n");

        outb(0xD0, SB16_WRITE_PORT);

        data_input_size = ece391_read(audio_fd, data_input, SB16_CHUNK_LENGTH);

        if (data_input_size < 0) {
            ece391_fdputs(1, (uint8_t*)"file read failed\n");
            return 3;
        }

        if (data_input_size == 0) {
            break;
        }

        // Pad as necessary
        for (i = data_input_size; i < SB16_CHUNK_LENGTH; i++) {
            data_input[i] = 0;
        }

        if (data_input_size != ece391_write(sb16_fd, data_input, data_input_size)) {
            ece391_fdputs(1, (uint8_t*)"file write failed\n");
            return 4;
        }

        outb(0xD4, SB16_WRITE_PORT);

        if (data_input_size < SB16_CHUNK_LENGTH) {
            if (ece391_read(sb16_fd, 0, 0) == -1) {
                return 5;
            }

            outb(0xDA, SB16_WRITE_PORT);
            break;
        }
    }

    audio_fd = ece391_close(audio_fd);
    if (audio_fd != 0) {
        ece391_fdputs(1, (uint8_t*)"file close failed\n");
        return 5;
    }

    sb16_fd = ece391_close(sb16_fd);
    if (sb16_fd != 0) {
        ece391_fdputs(1, (uint8_t*)"sb16 close failed\n");
        return 5;
    }

    return 0;
}

