#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"

#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "MediaHal.h"
#include "ES8388_interface.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "ringbuf.h"

#include "../lyraT_audio_provider.h"
#include "../main_functions.h"

static const char *TAG = "lyraT";

#define CHECK(a, b)                                                       \
    do                                                                    \
    {                                                                     \
        if (a)                                                            \
        {                                                                 \
            ESP_LOGE(TAG, "%s:%d: %s fail\n", __FUNCTION__, __LINE__, b); \
            abort();                                                      \
        }                                                                 \
    } while (0)

i2s_port_t i2s_enum = I2S_NUM_0;
unsigned int i2s;

void recsrcTask()
{
    CHECK(MediaHalInit(), "Media hal init");
    CHECK(MediaHalStart(CODEC_MODE_ENCODE), "Media HAL start");
    i2s = MediaHalGetI2sNum();
    // i2s_enum = 0;
    printf("I2S num %d\n", i2s);
    Es8388SetMicGain(MIC_GAIN_21DB);

    const int item_size = 480;
    uint8_t *samp = (uint8_t *)malloc(item_size * 2);
    printf("samp size: %d\n", item_size * 2);

    size_t bytes_read = 0;
    while (1) {
        i2s_read(i2s_enum, (void*)samp, item_size * 2, &bytes_read, portMAX_DELAY);
        printf("Read bytes: %d\n", bytes_read);
        print_int16(1, item_size, (int16_t *)samp);
    }
}
