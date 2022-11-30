#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define SB16_DATA_LENGTH  0xFFFF

typedef struct {
    uint32_t chunk_id;       // Should be 'RIFF' in big endian
    uint32_t chunk_size;     // File size
    uint32_t format;        // Should be 'WAVE' in big endian
    uint32_t subchunk_1_id;   // Should be 'fmt ' in big endian
    uint32_t subchunk_1_size; // Should be 0x10
    uint16_t audio_format;
    uint16_t num_channels;   // 1 or 2
    uint32_t sample_rate;    // Up to 44100
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bit_per_sample;
    uint32_t subchunk_2_id;   // Should be 'data' in big endian
    uint32_t subchunk_2_size;
} wav_meta_t;



int main(){
    // wav_meta_t wav_meta;
    int32_t sb16_fd, audio_fd, wave_meta_raw;
    int32_t i;
    int32_t data_input_size, data_write_size;
    uint8_t data_input[SB16_DATA_LENGTH];
    uint8_t file_name[1024]; // used for file input, hardcoded file for now

    ece391_strcpy(file_name, "stopandsmell.wav");

    // initialize sb16
    sb16_fd = ece391_open((uint8_t*)"sb16");

    if (sb16_fd == -1){
        ece391_fdputs(1, (uint8_t*)"sb16 not found\n");
        return 2;
    }

    // get file
    audio_fd = ece391_open(file_name);

    if (audio_fd == -1){
        ece391_fdputs(1, (uint8_t*)"file not found\n");
        return 2;
    }

    // read in wave meta data
    ece391_read(audio_fd, wave_meta_raw, sizeof(wav_meta_t));

    // processing to be done

    // load first data chunk
    data_input_size = ece391_read(audio_fd, data_input, SB16_DATA_LENGTH);

    if (data_input_size < 0) {
        ece391_fdputs(1, (uint8_t*)"file read failed\n");
        return 3;
    }

    // if undersized, pad
    if (data_input_size < SB16_DATA_LENGTH) {
        for (i = data_input_size; i < SB16_DATA_LENGTH; i++) {
            data_input[i] = 0;
        }
    }

    // write to sb16
    data_write_size = ece391_write(sb16_fd, data_input, data_input_size);

    if (data_write_size != data_input_size) {
        ece391_fdputs(1, (uint8_t*)"file write failed\n");
        return 4;
    }

        // load first data chunk
    data_input_size = ece391_read(audio_fd, data_input, SB16_DATA_LENGTH);

    if (data_input_size < 0) {
        ece391_fdputs(1, (uint8_t*)"file read failed\n");
        return 3;
    }

    // if undersized, pad
    if (data_input_size < SB16_DATA_LENGTH) {
        for (i = data_input_size; i < SB16_DATA_LENGTH; i++) {
            data_input[i] = 0;
        }
    }

    // write to sb16
    data_write_size = ece391_write(sb16_fd, data_input, data_input_size);

    if (data_write_size != data_input_size) {
        ece391_fdputs(1, (uint8_t*)"file write failed\n");
        return 4;
    }
    

    // while (1) {
    //     // load next data chunk

    // }

    return 0;
}

