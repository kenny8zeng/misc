#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "remoter_service.h"
#include "keymap.hpp"

static const char *TAG = "remoter_service";

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

JNIEXPORT void JNICALL Java_cn_hudplay_remoter_service_RemoterNative_init(JNIEnv *env, jobject thiz)
{
    LOGE( "%s start", __func__ );

	key_loop();
}
