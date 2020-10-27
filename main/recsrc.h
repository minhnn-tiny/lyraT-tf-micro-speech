#pragma once


/**
 * @brief Microphone source (read from the I2S codec on a LyraT board) task fuction
 *
 * @param arg Pointer to an src_cfg_t struct
*/

void recsrcTask(void *arg);
