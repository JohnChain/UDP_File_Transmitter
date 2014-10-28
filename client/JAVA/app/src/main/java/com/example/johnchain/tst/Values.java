package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
import java.net.InetAddress;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.locks.ReentrantLock;

public class Values {
    public final static int MT_SYN 		= 1;
    public final static int MT_PROG 	= 2;
    public final static int MT_RESER 	= 3;

    public final static String HEAD 	= "SZTEAM";

    public final static int TYPE_NUM 	= 20;

    public final static int SYN 		= 1;
    public final static int ACK 		= 2;
    public final static int SYN_ACK 	= 3;
    public final static int FIN 		= 4;
    public final static int DATA 		= 10;

    public final static int YES 		= 1;
    public final static int NO 			= 0;

    public final static int RST_OK		= 0;
    public final static int RST_ERROR	= -1;
    public final static int RST_TIMEOUT = -2;

    public static Queue<String> fileQueue = new LinkedList<String>();
    public static int[] taskArray;
    public static ReentrantLock[] lockArray;
    public final static int NEW 		= 0;
    public final static int PENDING		= 1;
    public final static int ABANDON		= 2;
    public final static int DONE		= 3;

    public final static int THREAD_NUMBER   = 8;

    public final static int BODYLEN   		= 1400;
    public final static int SYNACKLEN		= 64 + THREAD_NUMBER * 4;
    public final static int MAXLEN    		= BODYLEN + 24;

    public final static int LEN_FILENAME 	= 32;
    public final static int LEN_HEAD 		= 8;

    public static InetAddress SERV_ADDRESS 	= null;
    public static int SERV_PORT 			= 8888;
    public final static int LOCAL_PORT 		= 10001;

    public final static int SOCKET_TIMEOUT	= 2000;
    public final static int timeoutTimes 	= 20;

    public static int downloadedBlock 	= 0;
    public static String fileName		= "";
    public static int fileSize 			= 0;
    public static int blockNum 			= 0;
    public static String fileDict		= "";

    public static boolean isNewTask 	= true;
    public static boolean commanderLive = true;
    public static boolean cookLive		= true;
    public static boolean soldierLive 	= true;

    public static long timeCost = 0;

    public static ReentrantLock lockDownloadedBlock = new ReentrantLock();
    public static ReentrantLock lockState 			= new ReentrantLock();
    public static ReentrantLock lockAbandon			= new ReentrantLock();

    public static ReentrantLock lock_cost_readFile = new ReentrantLock();
    public static ReentrantLock lock_cost_packPack = new ReentrantLock();
    public static ReentrantLock lock_cost_recvPack = new ReentrantLock();
    public static ReentrantLock lock_cost_depack   = new ReentrantLock();


    public static int forSpeed = 0;
    public static int taskIndex = 0;
    public static int abandonNum = 0;

    public static long cost_readFile = 0;
    public static long cost_packPack = 0;
    public static long cost_recvPack = 0;
    public static long cost_depack   = 0;

    //public static String path = "/data/media/diag_logs/QXDM_logs/2014_09_28_13_09_59/";
//    public static String path = "/data/media/diag_logs/QXDM_logs/2014_09_29_16_10_59/";
    public static String path = "/sdcard/Download/";
    //public static String path = "/Removable/MicroSD/diag_logs/QXDM_logs/2014_09_28_16_48_19/";

}

