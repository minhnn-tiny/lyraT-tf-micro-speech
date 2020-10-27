/*
*
* Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "ES8388_interface.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "lock.h"
#include "userconfig.h"
#define HAL_TAG "MEDIA_HAL"

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

static CodecMode _currentMode;
static xSemaphoreHandle _halLock;

const i2s_pin_config_t i2s_pin = {
    .bck_io_num = IIS_SCLK,
    .ws_io_num = 25,
    .data_out_num = IIS_DSIN,
    .data_in_num = 35
};

#define SUPPOERTED_BITS 16
static int I2S_NUM = 0;//only support 16 now and i2s0 or i2s1
static char MUSIC_BITS = 0; //for re-bits feature, but only for 16 to 32

i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX,
    .sample_rate = 16000,
    .bits_per_sample = SUPPOERTED_BITS,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    //when dma_buf_count = 3 and dma_buf_len = 300, then 3 * 4 * 300 * 2 Bytes internal RAM will be used. The multiplier 2 is for Rx buffer and Tx buffer together.
    .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
    .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .use_apll = 1,
};

// Set ES8388
Es8388Config initConf = {
    .esMode = ES_MODE_SLAVE,
    .i2c_port_num = I2C_NUM_0,
    .i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = IIC_DATA,
        .scl_io_num = IIC_CLK,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    },
#if DIFFERENTIAL_MIC
    .adcInput = ADC_INPUT_DIFFERENCE,
#else
    .adcInput = ADC_INPUT_LINPUT1_RINPUT1,
#endif
    .dacOutput = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2,
};
int I2cInit(i2c_config_t *conf, int i2cMasterPort);
int MediaHalInit(void)
{
    int ret  = 0;
    // Set I2S pins
    ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    ret |= i2s_set_pin(I2S_NUM, &i2s_pin); // I2S0 default
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    if (I2S_NUM == 0) {
        SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    } else if (I2S_NUM > 1) {
        ESP_LOGE(HAL_TAG, "I2S_NUM must < 2");
        return -1;
    }

//    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD_CLK_OUT2);
//    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
//    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT2, 0, CLK_OUT2_S);

//    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_CLK_OUT3);
//    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
//    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT3, 0, CLK_OUT3_S);

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO35_U, FUNC_GPIO35_GPIO35);
    SET_PERI_REG_MASK(PERIPHS_IO_MUX_GPIO35_U, FUN_IE | FUN_PU);
    PIN_INPUT_ENABLE(PERIPHS_IO_MUX_GPIO35_U);

    ret |= Es8388Init(&initConf);
    ret |= Es8388ConfigFmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);
    _currentMode = CODEC_MODE_UNKNOWN;
    if (_halLock) {
        mutex_destroy(_halLock);
    }
    _halLock = mutex_init();

#if WROAR_BOARD
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_SEL_PA_EN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);
#endif
    return ret;
}

int MediaHalUninit(void)
{
    mutex_destroy(_halLock);
    Es8388Uninit(&initConf);
    _halLock = NULL;
    MUSIC_BITS = 0;
    _currentMode = CODEC_MODE_UNKNOWN;
    return 0;
}

int MediaHalStart(CodecMode mode)
{
    int ret ;
    int esMode = 0;
    mutex_lock(_halLock);
    switch (mode) {
    case CODEC_MODE_ENCODE:
        esMode  = ES_MODULE_ADC;
        break;
    case CODEC_MODE_LINE_IN:
        esMode  = ES_MODULE_LINE;
        break;
    case CODEC_MODE_DECODE:
        esMode  = ES_MODULE_DAC;
        break;
    case CODEC_MODE_DECODE_ENCODE:
        esMode  = ES_MODULE_ADC_DAC;
        break;
    default:
        esMode = ES_MODULE_DAC;
        ESP_LOGW(HAL_TAG, "Codec mode not support, default is decode mode");
        break;
    }
    ESP_LOGI(HAL_TAG, "Codec mode is %d", esMode);
    int inputConfig;
    if (esMode == ES_MODULE_LINE) {
        inputConfig = ADC_INPUT_LINPUT2_RINPUT2;
    } else {
#if DIFFERENTIAL_MIC
        inputConfig = ADC_INPUT_DIFFERENCE;
#else
        inputConfig = ADC_INPUT_LINPUT1_RINPUT1;
#endif
    }
    ESP_LOGI(HAL_TAG, "Codec inputConfig is %2X", inputConfig);
    ret = Es8388ConfigAdcInput(inputConfig);
    ret |= Es8388Start(esMode);
    _currentMode = mode;
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalStop(CodecMode mode)
{
    int ret ;
    int esMode = 0;
    mutex_lock(_halLock);
    switch (mode) {
    case CODEC_MODE_ENCODE:
        esMode  = ES_MODULE_ADC;
        break;
    case CODEC_MODE_LINE_IN:
        esMode  = ES_MODULE_LINE;
        break;
    case CODEC_MODE_DECODE:
        esMode  = ES_MODULE_DAC;
        break;
    default:
        esMode = ES_MODULE_DAC;
        ESP_LOGI(HAL_TAG, "Codec mode not support");
        break;
    }
    ret = Es8388Stop(esMode);
    _currentMode = CODEC_MODE_UNKNOWN;
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalGetCurrentMode(CodecMode *mode)
{
    MEDIA_HAL_CHECK_NULL(mode, "Get current mode para is null", -1);
    *mode = _currentMode;
    return 0;
}

int MediaHalSetVolume(int volume)
{
    int ret = 0;

    if (volume < 3 ) {
        if (0 == MediaHalGetMute()) {
            MediaHalSetMute(CODEC_MUTE_ENABLE);
        }
    } else {
        if ((1 == MediaHalGetMute())) {
            MediaHalSetMute(CODEC_MUTE_DISABLE);
        }
    }
    mutex_lock(_halLock);
    ret = Es8388SetVoiceVolume(volume);
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalGetVolume(int *volume)
{
    int ret ;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
    mutex_lock(_halLock);
    ret = Es8388GetVoiceVolume(volume);
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalSetMute(CodecMute mute)
{
    int ret ;
    mutex_lock(_halLock);
    ret = Es8388SetVoiceMute(mute);
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalGetMute(void)
{
    mutex_lock(_halLock);
    int res = Es8388GetVoiceMute();
    mutex_unlock(_halLock);
    return res;
}

int MediaHalSetBits(int bitPerSample)
{
    int ret = 0;
    if (bitPerSample <= BIT_LENGTH_MIN || bitPerSample >= BIT_LENGTH_MAX) {
        ESP_LOGE(HAL_TAG, "bitPerSample: wrong param");
        return -1;
    }
    mutex_lock(_halLock);
    ret = Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, (BitsLength)bitPerSample);
    mutex_unlock(_halLock);
    return ret;
}

int MediaHalSetClk(int i2s_num, uint32_t rate, uint8_t bits, uint32_t ch)
{
    int ret;
    if (bits != 16) {
        ESP_LOGE(HAL_TAG, "bits should be 16 Bits:%d", bits);
        return -1;
    }
    if (ch != 1 && ch != 2) {
        ESP_LOGE(HAL_TAG, "channel should be 1 or 2 %d", ch);
        return -1;
    }
    if (bits > SUPPOERTED_BITS) {
        ESP_LOGE(HAL_TAG, "Bits:%d, bits must be smaller than %d", bits, SUPPOERTED_BITS);
        return -1;
    }

    MUSIC_BITS = bits;
    ret = i2s_set_clk((i2s_port_t)i2s_num, rate, SUPPOERTED_BITS, ch);

    return ret;
}

int MediaHalGetI2sConfig(void *info)
{
    if (info) {
        memcpy(info, &i2s_config, sizeof(i2s_config_t));
    }
    return 0;
}

int MediaHalGetI2sNum(void)
{
    return I2S_NUM;
}

int MediaHalGetI2sBits(void)
{
    return SUPPOERTED_BITS;
}

int MediaHalGetSrcBits(void)
{
    return MUSIC_BITS;
}

int MediaHalPaPwr(int en)
{

#if WROAR_BOARD
    if (en) {
        gpio_set_level(GPIO_PA_EN, 1);
    } else {
        gpio_set_level(GPIO_PA_EN, 0);
    }
#endif
    return 0;
}
