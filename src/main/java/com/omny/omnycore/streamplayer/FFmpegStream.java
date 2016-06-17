package com.omny.omnycore.streamplayer;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import java.nio.ByteBuffer;

/**
 * OmnyCore
 * <p/>
 * Created by Jin Wang on 14/06/16.
 * Copyright (c) 2016 121cast. All rights reserved.
 */
public class FFmpegStream {
    private static final String TAG = FFmpegStream.class.getSimpleName();

    static {
        System.loadLibrary("OmnyFFmpeg");
    }

    public native ByteBuffer create(String urlPath);
    public native int prepare(ByteBuffer settings);
    public native boolean processNextPacket(ByteBuffer settings);
    public native short[] getNextAudioBuffer(ByteBuffer settings);
    public native void clearPacketQueue(ByteBuffer settings);
}
