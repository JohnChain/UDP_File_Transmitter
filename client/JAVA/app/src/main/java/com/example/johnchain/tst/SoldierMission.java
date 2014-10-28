package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

import android.util.Log;

public class SoldierMission {
    private int threadId;
    private MyMessage myMsg;
    private DatagramSocket socket;
    int serverPort;
    InetAddress serverAddress;

    public SoldierMission(int threadId, MyMessage myMsg){
        Log.d("johnchain","thread: " + threadId + " in");
        this.threadId = threadId;
        this.myMsg = myMsg;
        this.serverPort = myMsg.synackPack.port[threadId - 1];
        this.serverAddress = Values.SERV_ADDRESS;

        try {
            socket = new DatagramSocket(serverPort);
        } catch (SocketException e) {
            e.printStackTrace();
        }
    }

    /*
     * 找出index之后第一个状态为 fromState的数据块编号，并将该数据块状态改为 toState
     */
    public int popTask(int fromState, int toState, int cycle)
    {
        int index = 0;
        Values.lockState.lock();
        if(cycle == 1){
            if(Values.taskIndex >= Values.blockNum)
                index = Values.taskIndex;
            for(; Values.taskIndex < Values.blockNum;){
                if(Values.taskArray[Values.taskIndex] <= fromState){
                    Values.taskArray[Values.taskIndex] = toState;
                    Log.d("johnchain" ,"thread: " + threadId + "[first cycle] change taskArray[" + Values.taskIndex +"] to " + toState);
                    index = Values.taskIndex;
                    Values.taskIndex ++;
                    break;
                }
                Values.taskIndex ++;
            }
        }else if(cycle == 2){
            if(Values.taskIndex >= Values.blockNum)
                Values.taskIndex = 0;
            for(; Values.taskIndex < Values.blockNum;){
                if(Values.taskArray[Values.taskIndex] <= fromState){
//					Values.taskArray[Values.taskIndex] = toState;
                    Log.d("johnchain" ,"thread: " + threadId + "[second cycle] find taskArray[" + Values.taskIndex +"] state " + Values.taskArray[Values.taskIndex]);
                    index = Values.taskIndex;
                    Values.taskIndex ++;
                    break;
                }
                Values.taskIndex ++;
            }
        }

        if(Values.taskIndex > Values.blockNum){
            Log.d("johnchain", ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
            index = Values.taskIndex;
        }
        Values.lockState.unlock();
        return index;
    }

    public MyMessage waitReply(int replyType, DatagramPacket resendPacket){
		/* 等待请求回应 */
        int times = 0;
        MyMessage myMsg = null;
        while(Values.soldierLive && times < Values.timeoutTimes){
            final byte[] buf = new byte[Values.MAXLEN];
            DatagramPacket recvPacket = new DatagramPacket(buf, buf.length);
            try{
                socket.receive(recvPacket);
            } catch (IOException e) {
                Log.e("johnchain", "thread: " + threadId + " receive SYN_ACK timeout");
                try{
                    socket.send(resendPacket);
                    times ++;
                    continue;
                }catch (IOException e2){
                    Log.e("johnchain", "thread: " + threadId + " send SYN failed, transmition aborted");
                    return null;
                }
            }
            myMsg = PacketManager.depack2(recvPacket.getData());
            if(myMsg.type == replyType){
                Log.d("johnchain","thread: " + threadId + " received SYN_ACK | " + Utils.stringMessage(myMsg));
                break;
            }else{
                Log.d("johnchain", "thread: " + threadId + " received type: " + myMsg.type + " not what we wanted, receive again");
                times ++;
            }
        }
        if(times >= Values.timeoutTimes)
            return null;
        else
            return myMsg;
    }

    public int synackLink(SynAck synPack){
		/* 请求建立连接 */
        PacketManager mag = new PacketManager(Values.HEAD, Values.SYN, 0, 104, null, synPack);
        DatagramPacket sendPacket = new DatagramPacket(mag.getBuf(), mag.getBuf().length, serverAddress, serverPort);
        try {
            socket.send(sendPacket);
            Log.d("johnchain", "thread: " + threadId + " send SYN to server | " + Utils.stringMessage(myMsg));
        } catch (IOException e) {
            Log.e("johnchain", "thread: " + threadId + " send SYN failed, transmition aborted");
            return Values.RST_ERROR;
        }

        try {
            socket.setSoTimeout(Values.SOCKET_TIMEOUT * 2);
        } catch (SocketException e1) {
            Log.e("johnchain", "thread: " + threadId + " set timeout failed");
        }

        MyMessage myMsg = waitReply(Values.SYN_ACK, sendPacket);
        if(myMsg != null)
            return Values.RST_OK;
        else
            return Values.RST_TIMEOUT;
    }

    public void writeCompare(RandomAccessFile temp_handler, byte[] tempBuf, int index, int length){
		/* 原始数据写入对照文件 */
        String file = ("thread: " + threadId + " packNo: " + index);
        try {
            temp_handler.seek(index * Values.BODYLEN);
            temp_handler.write(tempBuf, 0, length);
            Log.e("johnchain", "thread: " + threadId + " write to comp file suc");
        } catch (IOException e2) {
            Log.e("johnchain", "thread: " + threadId + " write to comp file error");
        }

    }

    public void processMission(){
        String target_name = "sdcard/Download/" + this.myMsg.synackPack.fileInfo.fileName;
        RandomAccessFile fos_in = null;
//        String tempTarget = "sdcard/Download/" + this.myMsg.synackPack.fileInfo.fileName + "_comp";
        RandomAccessFile temp_handler = null;
        try {
			/* 获取 target 文件操作句柄 */
            fos_in = new RandomAccessFile(target_name, "r");
            Log.d("johnchain", "thread: " + threadId + " get part target handler: " + target_name);

			/* 获取对照文件操作句柄 */
//            File cmp_file = new File(tempTarget);
//            if(!cmp_file.exists())
//                cmp_file.createNewFile();
//            temp_handler = new RandomAccessFile(tempTarget, "rw");
//            Log.d("johnchain", "thread: " + threadId + " get part target handler: " + tempTarget);
        } catch (IOException e2) {
            Log.e("johnchain", "thread: " + threadId + " failed open target file, abort upload");
            return ;
        }

        if(socket == null){
            Log.e("johnchain","thread: " + threadId + " socket is null, abort transmition");
            return ;
        }
		/* 请求建立连接 */
        int linkRst = synackLink(this.myMsg.synackPack);
        if(linkRst == Values.RST_ERROR || linkRst == Values.RST_TIMEOUT)
            return ;

        try {
            socket.setSoTimeout(Values.SOCKET_TIMEOUT);
        } catch (SocketException e1) {
            Log.e("johnchain", "thread: " + threadId + " set timeout failed");
        }

		/* 第一轮上传数据 */
        int index = 0;
        Values.cost_readFile = 0;
        Values.cost_packPack = 0;
        Values.cost_recvPack = 0;
        Values.cost_depack   = 0;
        long t1 = 0;
        boolean selfEnd   = false;
        Log.d("johnchain", "thread: " + threadId + " soldierLive = " + Values.soldierLive);
        while(Values.soldierLive && !selfEnd){

            t1 = System.currentTimeMillis();
			/* 找到下一个未发送的数据块 */
            index = popTask(Values.NEW, Values.PENDING, 1);
            if(index >= Values.blockNum){
                break;
            }

            byte[] tempBuf = new byte[Values.BODYLEN];
            int bytesRead;
            int seekNum = index *  Values.BODYLEN;
            try {
                fos_in.seek(seekNum);
                bytesRead = fos_in.read(tempBuf);
                Log.d("johnchain", "thread: " + threadId + "[first cycle] will send packNo: " + index + " seekNum: " + seekNum + " bytesRead: " + bytesRead);
            } catch (IOException e2) {
//				e2.printStackTrace();
                Log.e("johnchain", "thread: " + threadId + "[first cycle] operate file error, transmition aborted");
                return ;
            }
            Values.lock_cost_readFile.lock();
            Values.cost_readFile += System.currentTimeMillis() - t1;
            t1 = System.currentTimeMillis();
            Values.lock_cost_readFile.unlock();


            PacketManager ackPackMag = new PacketManager(Values.HEAD, Values.DATA, index, bytesRead, tempBuf, null);
            DatagramPacket sendDataPack = new DatagramPacket(ackPackMag.getBuf(), ackPackMag.getBuf().length, serverAddress, serverPort);
            Values.lock_cost_packPack.lock();
            Values.cost_packPack += System.currentTimeMillis() - t1;
            t1 = System.currentTimeMillis();
            Values.lock_cost_packPack.unlock();

            try {
                socket.send(sendDataPack);
            } catch (IOException e) {
                Log.e("johnchain", "thread: " + threadId + "[first cycle] failed to upload, please check the network");
                break;
            }

//			/* 原始数据写入对照文件 */
//			writeCompare(temp_handler, tempBuf, index, bytesRead);

			/* 等待数据确认 */
            int times = 0;
            while(!selfEnd && Values.soldierLive && times < Values.timeoutTimes){
                final byte[] buf = new byte[Values.MAXLEN];
                DatagramPacket recvPacket = new DatagramPacket(buf, buf.length);
                try {
                    socket.receive(recvPacket);
                }catch (IOException e) {
//					e.printStackTrace();
                    times ++;
                    Log.w("johnchain", "thread: " + threadId + "[first cycle] receive data timeout :" + times + "resend packNo: " + index);
                    try {
                        socket.send(sendDataPack);
                    } catch (IOException e1) {
                        Log.e("johnchain", "thread: " + threadId + "[first cycle] failed to upload, please check the network");
                        break;
                    }
                    if(times >= Values.timeoutTimes){
                        Values.lockState.lock();
                        Log.d("johnchain" ,"thread: " + threadId + "[first cycle] change packNo[" + index +"] to abandon");
                        Values.taskArray[index] = Values.ABANDON;
                        Values.lockState.unlock();

                        Values.lockAbandon.lock();
                        Values.abandonNum ++;
                        Log.d("johnchain" ,"thread: " + threadId + "[first cycle] abandon number = " + Values.abandonNum);
                        Values.lockAbandon.unlock();

                        index ++;
                        break;
                    }
                    continue;
                }

                Values.lock_cost_recvPack.lock();
                Values.cost_recvPack += System.currentTimeMillis() - t1;
                t1 = System.currentTimeMillis();
                Values.lock_cost_recvPack.unlock();


				/* 收到数据确认，修改任务状态 */
                MyMessage myMsg = PacketManager.depack2(recvPacket.getData());
                if(myMsg.type == Values.ACK){
					/* 修改全局变量：总下载数据块数 */
                    Log.d("johnchain", "thread: " + threadId + "[first cycle] wait for " + index + " , received: " + myMsg.packNo);
                    if(myMsg.packNo == index){
                        Values.lockDownloadedBlock.lock();
                        Values.downloadedBlock++;
                        Log.d("johnchain" ,"thread: " + threadId + "[first cycle] change downloadedBlock to:" + Values.downloadedBlock);
                        Values.lockDownloadedBlock.unlock();

                        Values.lockState.lock();
                        Log.d("johnchain" ,"thread: " + threadId + "[first cycle] change taskArray[" + index +"] to DONE");
                        Values.taskArray[index] = Values.DONE;
                        Values.lockState.unlock();
                        index ++;
                        break;
                    }else{
                        Log.d("johnchain", "thread: " + threadId + "[first cycle] not what I wanted, wait again");
                        continue;
                    }
                }else if(myMsg.type == Values.FIN){
                    Log.d("johnchain" ,"thread: " + threadId + "[first cycle] received FIN | " + Utils.stringMessage(myMsg));
                    selfEnd = true;
                    break;
                }else{
                    Log.d("johnchain" ,"thread: " + threadId + "[first cycle] received unkonwn commande type: " + myMsg.type +" ignored");
                }

                Values.lock_cost_depack.lock();
                Values.cost_depack += System.currentTimeMillis() - t1;
                t1 = System.currentTimeMillis();
                Values.lock_cost_depack.unlock();
            }
        }

//        String result = Arrays.toString(Values.taskArray);
//        Log.d("johnchain", "thread: " + threadId + " " + result);

		/* 第二轮上传数据 */
        index = 0;
        Log.d("johnchain", "thread: " + threadId + " enter second circle, Abandoned Block: " + Values.abandonNum);
        while(Values.soldierLive && !selfEnd && Values.abandonNum != 0){

            t1 = System.currentTimeMillis();
			/* 找到下一个未发送的数据块 */
            index = popTask(Values.ABANDON, Values.ABANDON, 2);
            if(Values.abandonNum == 0){
                Log.d("johnchain", "thread: " + threadId + " abandon is zero, send done");
                break;
            }else if(index >= Values.blockNum){
                Log.d("johnchain", "thread: " + threadId + "[second cycle] one cirle end, still exists abandoned block, once more");
                index = 0; // 尚有未被确认的，第一轮中被放弃的数据
                continue;
            }

            byte[] tempBuf = new byte[Values.BODYLEN];
            int bytesRead;
            int seekNum = index *  Values.BODYLEN;
            try {
                fos_in.seek(seekNum);
                bytesRead = fos_in.read(tempBuf);
                Log.d("johnchain", "thread: " + threadId + "[second cycle] will send packNo: " + index +
                        " seekNum: " + seekNum + " bytesRead: " + bytesRead);
            } catch (IOException e2) {
//				e2.printStackTrace();
                Log.e("johnchain", "thread: " + threadId + "[second cycle] operate file error, transmition aborted");
                return ;
            }
            Values.lock_cost_readFile.lock();
            Values.cost_readFile += System.currentTimeMillis() - t1;
            t1 = System.currentTimeMillis();
            Values.lock_cost_readFile.unlock();

            Log.d("johnchain", "thread: " + threadId + " pack " + index + " DATA ready");
            PacketManager ackPackMag = new PacketManager(Values.HEAD, Values.DATA, index, bytesRead, tempBuf, null);
            DatagramPacket sendDataPack = new DatagramPacket(ackPackMag.getBuf(), ackPackMag.getBuf().length, serverAddress, serverPort);
            Values.lock_cost_packPack.lock();
            Values.cost_packPack += System.currentTimeMillis() - t1;
            t1 = System.currentTimeMillis();
            Values.lock_cost_packPack.unlock();

            try {
                socket.send(sendDataPack);
                Log.d("johnchain", "thread: " + threadId + "[second cycle] send packNo: " + index + " bytes: " + sendDataPack.getLength());
            } catch (IOException e) {
                Log.e("johnchain", "thread: " + threadId + "[second cycle] failed to upload, please check the network");
                break;
            }

			/* 等待数据确认 */
            int times = 0;
            while(!selfEnd && Values.soldierLive && times < Values.timeoutTimes){
                final byte[] buf = new byte[Values.MAXLEN];
                DatagramPacket recvPacket = new DatagramPacket(buf, buf.length);
                try {
                    socket.receive(recvPacket);
                }catch (IOException e) {
                    times ++;
                    try {
                        socket.send(sendDataPack);
                        Log.w("johnchain", "thread: " + threadId + "[second cycle] receive data timeout :" + times +
                                "resend packNo: " + index + "bytes: " + sendDataPack.getLength());
                    } catch (IOException e1) {
                        Log.e("johnchain", "thread: " + threadId + "[second cycle] failed to upload, please check the network");
                        return;
                    }
                    if(times >= Values.timeoutTimes){
                        index ++;
                        break;
                    }
                    continue;
                }

                if(times >= Values.timeoutTimes) continue; //一个数据块接收超时

                Values.lock_cost_recvPack.lock();
                Values.cost_recvPack += System.currentTimeMillis() - t1;
                t1 = System.currentTimeMillis();
                Values.lock_cost_recvPack.unlock();

				/* 收到数据确认，修改任务状态 */
                MyMessage myMsg = PacketManager.depack2(recvPacket.getData());
                if(myMsg.type == Values.ACK){
					/* 修改全局变量：总下载数据块数 */
                    Log.d("johnchain", "thread: " + threadId + "[second cycle] wait for " + index + " , received: " + myMsg.packNo);
                    if(myMsg.packNo == index){

                        Values.lockDownloadedBlock.lock();
                        Values.downloadedBlock++;
                        Log.d("johnchain" ,"thread: " + threadId + "[second cycle] change downloadedBlock to:" + Values.downloadedBlock);
                        Values.lockDownloadedBlock.unlock();

//						Values.lockArray[index].lock();
                        Values.lockState.lock();
                        Log.d("johnchain" ,"thread: " + threadId + "[second cycle] change taskArray[" + index +"] to DONE");
                        Values.taskArray[index] = Values.DONE;
//						Values.lockArray[index].unlock();
                        Values.lockState.lock();

                        Values.lockAbandon.lock();
                        Values.abandonNum --;
                        Log.d("johnchain" ,"thread: " + threadId + "[second cycle] abandon number = " + Values.abandonNum);
                        Values.lockAbandon.unlock();
                        index ++;
                        break;
                    }else{
                        Log.d("johnchain", "thread: " + threadId + "[second cycle] not what I wanted, wait again");
                        continue;
                    }
                }else if(myMsg.type == Values.FIN){
                    Log.d("johnchain","thread: " + threadId + "[second cycle] received FIN | " + Utils.stringMessage(myMsg));
                    selfEnd = true;
                    break;
                }else{
                    Log.d("johnchain" ,"thread: " + threadId + "[second cycle] received unkonwn commande type: " + myMsg.type +" ignored");
                }
                Values.lock_cost_depack.lock();
                Values.cost_depack += System.currentTimeMillis() - t1;
                t1 = System.currentTimeMillis();
                Values.lock_cost_depack.unlock();
            }
        }

        SynAck finPack = new SynAck();
        PacketManager finPackMag = new PacketManager(Values.HEAD, Values.FIN, 0, Values.SYNACKLEN, null, finPack);
        DatagramPacket sendFinPack = new DatagramPacket(finPackMag.getBuf(), finPackMag.getBuf().length, serverAddress, serverPort);
        try {
            Log.d("johnchain", "thread: " + threadId + " send FIN to client and close self socket, send bytes: " + sendFinPack.getLength());
            socket.send(sendFinPack);
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            Log.d("johnchain", "thread: " + threadId + " close partFileHandler");
            fos_in.close();
//            temp_handler.close();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (NullPointerException e){
            e.printStackTrace();
        }

        Log.e("johnchain", "thread: " + threadId + " exit");
    }
}
