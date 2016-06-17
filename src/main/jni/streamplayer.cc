#include <jni.h>

#include <android/log.h>
#include "StreamSettings.h"

#define LOG_TAG "libffmpeg_native"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                                             __VA_ARGS__))
jboolean checkAVError(string ctx, int errnum) {
    if (errnum < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(errnum, str, AV_ERROR_MAX_STRING_SIZE);
        LOGE("%s: %s", ctx.c_str(), str);
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

jboolean prepareReader(StreamSettings* settings) {
    // Open the stream. The path must have a proper scheme that ffmpeg can suppport.
    AVFormatContext* context = avformat_alloc_context();

    if (checkAVError("Connection failed", avformat_open_input(&context, settings->urlPath, NULL, NULL))) {
        return JNI_FALSE;
    }

    // Fetch the stream info if possible.
    if (checkAVError("Stream info load failed.", avformat_find_stream_info(context, NULL))) {
        return JNI_FALSE;
    }

    // Print detailed information about the stream. For debug purpose.
    av_dump_format(context, 0, settings->urlPath, 0);

    // Looks all good. Store it.
    settings->context = context;

    return JNI_TRUE;
}

jboolean prepareCodec(StreamSettings* settings) {
    AVCodec* codec = NULL;
    int streamIndex = av_find_best_stream(settings->context, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

    if (checkAVError("Cannot find best stream.", streamIndex)) {
        return JNI_FALSE;
    }

    settings->stream = settings->context->streams[streamIndex];
    settings->codecContext = settings->stream->codec;

    if (checkAVError("Codec open failed.", avcodec_open2(settings->codecContext, codec, NULL))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

int convert(AVFrame* frame, short* outPtr) {
    int sampleSize = frame->nb_samples;
    int channels = frame->channels;

    for (int i = 0; i < sampleSize; ++i) {
        for (int j = 0; j < channels; ++j) {
            float* extended_data = (float *) frame->extended_data[j];
            float sample = extended_data[i];

            if (sample < -1.0) {
                sample = -1.0f;
            } else if (sample > 1.0) {
                sample = 1.0f;
            }

            outPtr[i * channels + j] = static_cast<short>(round(sample * 32767.0));
        }
    }

    return sampleSize * channels;
}

extern "C" {

JNIEXPORT jobject JNICALL
Java_com_omny_omnycore_streamplayer_FFmpegStream_create(JNIEnv *env, jobject instance, jstring urlPath) {
    StreamSettings* settings = new StreamSettings();

    settings->urlPath = (char *) env->GetStringUTFChars(urlPath, 0);

    // Register the protocol we want to support. For now, just register all of them.
    av_register_all();

    // For network streaming, this must be called.
    avformat_network_init();

    return env->NewDirectByteBuffer(settings, 0);
}

JNIEXPORT jint JNICALL
Java_com_omny_omnycore_streamplayer_FFmpegStream_prepare(JNIEnv *env, jobject instance, jobject handle) {
    StreamSettings* settings = (StreamSettings *) env->GetDirectBufferAddress(handle);

    if (!prepareReader(settings)) return -1;
    if (!prepareCodec(settings)) return -1;

    return settings->codecContext->sample_rate;
}

JNIEXPORT jboolean JNICALL
Java_com_omny_omnycore_streamplayer_FFmpegStream_processNextPacket(JNIEnv *env, jobject instance, jobject handle) {
    StreamSettings* settings = (StreamSettings *) env->GetDirectBufferAddress(handle);

    AVPacket* packet = av_packet_alloc();
    int ret = av_read_frame(settings->context, packet);

    if (ret != 0) {
        // Whoops, something wrong happend.
        if (ret == AVERROR_EOF) {
            // End of the file.

        } else {
            // Something else wrong.
        }
        return JNI_FALSE;
    } else {
        // Add the packet to the queue.
        if (packet->stream_index == settings->stream->index) {
            settings->packetQueue.push(packet);
        }
        return JNI_TRUE;
    }
}

JNIEXPORT jshortArray JNICALL
Java_com_omny_omnycore_streamplayer_FFmpegStream_getNextAudioBuffer(JNIEnv *env, jobject instance, jobject handle) {
    StreamSettings* settings = (StreamSettings *) env->GetDirectBufferAddress(handle);

    if (settings->packetQueue.empty())
        return NULL;

    AVPacket *packet = settings->packetQueue.front();
    settings->packetQueue.pop();

    //
    int gotFrame = 0;
    AVFrame* frame = av_frame_alloc();
    int decodedLength = avcodec_decode_audio4(settings->codecContext, frame, &gotFrame, packet);

    if (decodedLength < packet->size) {
        LOGE("HOLY, the decode size is too small!");
    }

    av_packet_free(&packet); // Don't forget this one.

    if (checkAVError("Decode packet failed", decodedLength)) {
        return NULL;
    } else if (gotFrame == 0) {
        return Java_com_omny_omnycore_streamplayer_FFmpegStream_getNextAudioBuffer(env, instance, handle);
    } else {
        // We got a new decoded frame, yay!

        // No data available at all? Skip this packet.
        if (frame->data == NULL) return NULL;

        // The size is critical, if we use wrong size then there will be either noisy or trimming.
        int dataSize = frame->channels * frame->nb_samples;
        short outPtr[dataSize]; // This means we'll alloc dataSize number of shorts.
        convert(frame, outPtr);

        av_frame_free(&frame);

        jshortArray retval = env->NewShortArray(dataSize); // This means we'll have dataSize number of shorts.
        env->SetShortArrayRegion(retval, 0, dataSize, outPtr); // This means we'll insert dataSize number of shorts into the region.

        // All the above methods use dataSize as count instead of the byte size. Please be warned.

        return retval;
    }
}

};

