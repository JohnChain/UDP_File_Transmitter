package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
public class SynAck{
    int downloadable = 0;
    int threadNum = 0;
    int port[] = new int[Values.THREAD_NUMBER];
    LocalFile fileInfo = new LocalFile();

    public class LocalFile{
        //	    byte[] filename = new byte[Values.LEN_FILENAME];
        String fileName = "";
        int fileSize = 0;

        int breakPoint = 0;
        int endPoint = 0;

        int blockSize = Values.BODYLEN;
        int blockNum = 0;
        int downloadedBlock = 0;
    };
};
