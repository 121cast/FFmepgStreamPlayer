//
// Created by Jin Wang on 16/6/16.
//

extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

}

#include <string>
#include <queue>
using namespace std;

#ifndef OMNYCORE_STREAMSETTINGS_H
#define OMNYCORE_STREAMSETTINGS_H


class StreamSettings {
public:
    StreamSettings();
    AVFormatContext* context;
    char* urlPath;
    AVStream* stream;
    AVCodecContext* codecContext;
    queue<AVPacket*> packetQueue;
    bool isReleased;
};


#endif //OMNYCORE_STREAMSETTINGS_H
