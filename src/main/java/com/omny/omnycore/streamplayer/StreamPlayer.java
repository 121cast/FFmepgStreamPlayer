package com.omny.omnycore.streamplayer;

/**
 * OmnyCore
 * <p/>
 * Created by Jin Wang on 14/06/16.
 * Copyright (c) 2016 121cast. All rights reserved.
 */
public class StreamPlayer {
    static {
        System.loadLibrary("libffmpeg");
    }

    public native void prepare();
}
