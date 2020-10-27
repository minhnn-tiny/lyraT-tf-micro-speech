#include <stdlib.h>
#include "esp_system.h"
#include "string.h"
#include "userconfig.h"
#include "esp_heap_alloc_caps.h"

//#include "esp_log.h"

void *EspAudioAlloc(int n, int size)
{
    void *data =  NULL;

#if IDF_3_0
    #ifdef CONFIG_SPIRAM_BOOT_INIT
//    ESP_LOGI("ALLOC", "PSRAM %d", n * size);
    data = heap_caps_malloc(n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (data) {
        memset(data, 0, n * size);
    }
    #else
//    ESP_LOGI("ALLOC", "D-RAM %d", n * size);
    data = calloc(n, size);
    #endif
#else
    #ifdef CONFIG_MEMMAP_SPIRAM_ENABLE_MALLOC
//    ESP_LOGI("ALLOC", "PSRAM %d", n * size);
    data = pvPortMallocCaps(n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (data) {
        memset(data, 0, n * size);
    }
    #else
//    ESP_LOGI("ALLOC", "D-RAM %d", n * size);
    data = calloc(n, size);
    #endif
#endif

    return data;
}

void *EspAudioAllocInner(int n, int size)
{
    void *data =  NULL;
//    ESP_LOGI("ALLOC", "INTERNAL %d", n * size);
#if IDF_3_0
    data = heap_caps_malloc(n * size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
#else
    data = pvPortMallocCaps(n * size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
#endif
    if (data) {
        memset(data, 0, n * size);
    }

    return data;
}

