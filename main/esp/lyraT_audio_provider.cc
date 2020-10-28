#include <stdio.h>
#include "esp_log.h"
#include "MediaHal.h"
#include "ES8388_interface.h"

#include "../lyraT_audio_provider.h"

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

int i2s;

void recsrcTask(){
    CHECK(MediaHalInit(), "Media hal init");
    CHECK(MediaHalStart(CODEC_MODE_ENCODE), "Media HAL start");
    i2s = MediaHalGetI2sNum();
    printf("I2S num %d\n", i2s);
    Es8388SetMicGain(MIC_GAIN_21DB);
}
