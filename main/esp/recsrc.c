#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "xtensa/core-macros.h"
#include "esp_partition.h"
#include "src_if.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "ES8388_interface.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#include "board.h"
#include "audio_common.h"
#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "esp_vad.h"

#define WIN_STEP 8000

#define VAD_SAMPLE_RATE_HZ 16000
#define VAD_FRAME_LENGTH_MS 30
#define VAD_BUFFER_LENGTH (VAD_FRAME_LENGTH_MS * VAD_SAMPLE_RATE_HZ / 1000)

static const char *TAG = "main";

#define CHECK(a, b)                                                       \
    do                                                                    \
    {                                                                     \
        if (a)                                                            \
        {                                                                 \
            ESP_LOGE(TAG, "%s:%d: %s fail\n", __FUNCTION__, __LINE__, b); \
            abort();                                                      \
        }                                                                 \
    } while (0)

int i2s;

void print_int_array(const int16_t *arr, int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

//Enable this for Automatic Gain Control. Current nn doesn't seem to need it that much.
#define DO_ACG
// UBaseType_t queue_len_recsrc;

// #define USING_ADF

void recsrcTask(void *arg)
{
    src_cfg_t *cfg = (src_cfg_t *)arg;
#ifndef USING_ADF
    CHECK(MediaHalInit(), "Media hal init");
    CHECK(MediaHalStart(CODEC_MODE_ENCODE), "Media HAL start");
    i2s = MediaHalGetI2sNum();
    printf("I2S num %d\n", i2s);
    Es8388SetMicGain(MIC_GAIN_21DB);
#else
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 16000;
    i2s_cfg.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_cfg.type = AUDIO_STREAM_READER;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.2] Create filter to resample audio data");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = VAD_SAMPLE_RATE_HZ;
    rsp_cfg.dest_ch = 1;
    filter = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(TAG, "[2.3] Create raw to receive data");
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,
        .type = AUDIO_STREAM_READER,
    };
    raw_read = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[ 3 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    // audio_pipeline_register(pipeline, filter, "filter");
    audio_pipeline_register(pipeline, raw_read, "raw");

    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[VAD]");
    audio_pipeline_link(pipeline, (const char *[]) {"i2s", "raw"}, 2);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    ESP_LOGI(TAG, "[ 6 ] Initialize VAD handle");
    vad_handle_t vad_inst = vad_create(VAD_MODE_2, VAD_SAMPLE_RATE_HZ, VAD_FRAME_LENGTH_MS);
    unsigned long long record_tick = 0;
#endif
    int16_t *samp = malloc(cfg->item_size * 2);
    int16_t *audio = malloc(cfg->item_size);

    printf("samp size: %d\n", cfg->item_size * 2);
#ifndef DO_ACG
    int max;
#endif
    int gain = 512; //128;
    int32_t i;
    while (1) {
#ifndef USING_ADF
        i2s_read_bytes(i2s, (void*)samp, cfg->item_size * 2, portMAX_DELAY);
#else
        raw_stream_read(raw_read, (char *)samp, cfg->item_size * 2);
#endif
        // Feed samples to the VAD process and get the result
#ifdef USING_ADF
        vad_state_t vad_state = vad_process(vad_inst, samp);
        if (vad_state == VAD_SPEECH) {
            ESP_LOGI(TAG, "Speech detected");
            record_tick = xTaskGetTickCount();
        } else {
            if ((xTaskGetTickCount() - record_tick) * portTICK_PERIOD_MS > 1000) {
                // ESP_LOGI(TAG, "Stop record");
                continue;
            }
        }
#endif
#ifndef DO_ACG
        max = 0;
#endif
        for (int x = 0; x < cfg->item_size / 2; x++)
        { // 800
            i = (int)samp[x * 2] + (int)samp[x * 2 + 1];

#ifdef DO_ACG
            i = (i * gain) / 32;
            if (i < -32768)
                i = 32768;
            if (i > 32767)
                i = 32767;
            // audio[x] = audio[x + WIN_STEP];
            // audio[x + WIN_STEP] = i;
            audio[x] = i;
            // if (max<i) max=i;
#else
            samp[x] = i / 2;
#endif
        }
#ifndef DO_ACG
        ++t;
        if ((t & 7) == 0)
        {
            if (max < 27000)
                gain += (gain / 128) + 1;
        }
        if (max > 32000)
            gain -= (gain / 2);
        if (gain < 2)
            gain = 2;
        if (gain > 4096)
            gain = 4096;
#endif
        //  printf("Max % 6d gain % 6d\n", max, gain);
        xQueueSend(*cfg->queue, audio, portMAX_DELAY);
    }
    vTaskDelete(NULL);
}
