#ifndef _LOCK_H_
#define _LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


typedef void * xSemaphoreHandle;

xSemaphoreHandle mutex_init(void);

void mutex_lock(xSemaphoreHandle pxMutex);

void mutex_unlock(xSemaphoreHandle pxMutex);

void mutex_destroy(xSemaphoreHandle pxMutex);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif
