#ifndef _ESP_AUDIO_SYS_H_
#define _ESP_AUDIO_SYS_H_

#include "esp_system.h"

//#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    ESP_AUDIO_ALLOC only for structure
    EspAudioAlloc only for buffer
*/
#define ESP_AUDIO_ALLOC(n, size) \
EspAudioAlloc(n, size);\
//ESP_LOGI("", "FUNC:%s LINE:%d\n", __func__, __LINE__);

void *EspAudioAlloc(int n, int size);
void *EspAudioAllocInner(int n, int size);


#ifdef __cplusplus
}
#endif

#endif  //_ESP_AUDIO_SYS_H_
