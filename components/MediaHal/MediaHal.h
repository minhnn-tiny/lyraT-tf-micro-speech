#ifndef _MEDIA_HAL_H_
#define _MEDIA_HAL_H_


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


typedef enum CodecMode {
    CODEC_MODE_UNKNOWN,
    CODEC_MODE_ENCODE ,
    CODEC_MODE_DECODE ,
    CODEC_MODE_LINE_IN,
    CODEC_MODE_DECODE_ENCODE,
    CODEC_MODE_MAX
} CodecMode;

typedef enum CodecMute {
    CODEC_MUTE_DISABLE,
    CODEC_MUTE_ENABLE,
} CodecMute;

/**
 * @brief Initialize media codec driver.
 *
 * @praram void
 *
 * @return  int, 0--success, others--fail
 */
int MediaHalInit(void);

/**
 * @brief Uninitialize media codec driver.
 *
 * @praram void
 *
 * @return  int, 0--success, others--fail
 */
int MediaHalUninit(void);

/**
 * @brief Start codec driver.
 *
 * @param  mode : Refer to enum CodecMode.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalStart(CodecMode mode);

/**
 * @brief Stop codec driver.
 *
 * @param  mode : Refer to enum CodecMode.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalStop(CodecMode mode);

/**
 * @brief Get the codec working mode.
 *
 * @param  mode: Current working mode will be return.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalGetCurrentMode(CodecMode *mode);

/**
 * @brief Set voice volume.
 *
 * @param  volume: Value will be setup.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalSetVolume(int volume);

/**
 * @brief Get voice volume.
 *
 * @param  volume:Current volume will be return.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalGetVolume(int *volume);

/**
 * @brief Set codec driver mute status.
 *
 * @param  mute: 1--Enable mute; 0-- Disable mute.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalSetMute(CodecMute mute);

/**
 * @brief Get codec driver mute status.
 *
 * @param  void
 *
 * @return     int, 0-- Unmute, 1-- Mute, <0 --fail
 */
int MediaHalGetMute(void);

/**
 * @brief Set codec Bits.
 *
 * @param  bitPerSample: see structure BitsLength
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalSetBits(int bitPerSample);

/**
 * @brief Set clock & bit width used for I2S RX and TX.
 *
 * @param i2s_num  I2S_NUM_0, I2S_NUM_1
 *
 * @param rate I2S sample rate (ex: 8000, 44100...)
 *
 * @param bits I2S bit width (16, 24, 32)
 *
 * @return
 *     - 0   Success
 *     - -1  Error
 */
int MediaHalSetClk(int i2s_num, uint32_t rate, uint8_t bits, uint32_t ch);

/**
 * @brief Get i2s configuration.
 *
 * @param info: i2s_config_t information.
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalGetI2sConfig(void *info);

/**
 * @brief Get i2s number
 *
 * @param  None
 *
 * @return int, i2s number
 */
int MediaHalGetI2sNum(void);

/**
 * @brief Get i2s bits
 *
 * @param  None
 *
 * @return int, i2s bits
 */
int MediaHalGetI2sBits(void);

/**
 * @brief get source-music bits for re-bits feature, but only for 16bits to 32bits
 *
 * @param  None
 *
 * @return
 *     - 0   disabled
 *     - 16  enabled
 */
int MediaHalGetSrcBits(void);

/**
 * @brief Power on or not for PA.
 *
 * @param  1: enable; 0: disable
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalPaPwr(int en);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  //__MEDIA_HAL_H__
