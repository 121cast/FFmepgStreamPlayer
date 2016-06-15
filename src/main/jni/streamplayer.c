#include <jni.h>

#include <android/log.h>
#include "include/libavformat/avformat.h"

#define LOG_TAG "libffmpeg_native"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                                             __VA_ARGS__))
JNIEXPORT void JNICALL
Java_com_omny_omnycore_streamplayer_StreamPlayer_prepare(JNIEnv *env, jobject instance) {

    AVFormatContext* context = avformat_alloc_context();

    // TODO
    LOGE("This is called!", context->protocol_whitelist);
}