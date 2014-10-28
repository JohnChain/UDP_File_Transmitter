package com.example.johnchain.tst;

import android.util.Log;
import java.net.*;
import java.util.Arrays;

/**
 * Created by johnchain on 14-9-29.
 */
public class PacketManager {
    private byte[] buf = null;

    /**
     * 将int转为低字节在前，高字节在后的byte数组
     */
    public static byte[] toLH(int n) {
        byte[] b = new byte[4];
        b[0] = (byte) (n & 0xff);
        b[1] = (byte) (n >> 8 & 0xff);
        b[2] = (byte) (n >> 16 & 0xff);
        b[3] = (byte) (n >> 24 & 0xff);
        return b;
    }

    /**
     * 将int转为低字节在前，高字节在后的byte数组
     */
    public static byte[] longtoLH(long n) {
        byte[] b = new byte[8];
        b[0] = (byte) (n & 0xffff);
        b[1] = (byte) (n >> 8 & 0xffff);
        b[2] = (byte) (n >> 16 & 0xffff);
        b[3] = (byte) (n >> 24 & 0xffff);
        b[4] = (byte) (n >> 32 & 0xffff);
        b[5] = (byte) (n >> 40 & 0xffff);
        b[6] = (byte) (n >> 48 & 0xffff);
        b[7] = (byte) (n >> 56 & 0xffff);
        return b;
    }

    /**
     * 将float转为低字节在前，高字节在后的byte数组
     */
    public static byte[] toLH(float f) {
        return toLH(Float.floatToRawIntBits(f));
    }

    /**
     * 构造并转换
     */
    public PacketManager(String head, int type, int packetID, int packetLen, byte[] packetBody, SynAck synack) {
        byte[] temp = null;
        int seekPos = 0;

//		Log.e("in message pack func", "bef pack synack Pos = " + seekPos);

//		temp = packetBody.getBytes();
//		int bodySize = temp.length;
        if(type == Values.DATA){
            buf = new byte[Values.BODYLEN + 20 + 4];

            temp = head.getBytes();
            int headSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, headSize);
            seekPos += Values.LEN_HEAD;

            temp = toLH(type);
            int typeSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += typeSize;

            temp = toLH(packetID);
            int packIdSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += packIdSize;

            temp = toLH(packetLen);
            int lengthSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += lengthSize;


//			Log.e("in generate pack", "body sizse is " + packetBody.length);
            System.arraycopy(packetBody, 0, buf, seekPos, packetLen);
            seekPos += packetLen;
        }else{
            buf = new byte[Values.SYNACKLEN + 20 + 4];


            temp = head.getBytes();
            int headSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, headSize);
            seekPos += Values.LEN_HEAD;

            temp = toLH(type);
            int typeSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += typeSize;

//			Log.d("johnchain", "seekPos = " + seekPos);

            temp = toLH(packetID);
            int packIdSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += packIdSize;

            temp = toLH(packetLen);
            int lengthSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, 4);
            seekPos += lengthSize;

            /**
             * pack synack below
             */
            temp = toLH(synack.downloadable);
            int downloadableSize = temp.length;
            System.arraycopy(temp, 0, buf, 20, downloadableSize);
            seekPos += downloadableSize;

            temp = toLH(synack.threadNum);
            int threadNumSize = temp.length;
            System.arraycopy(temp, 0, buf, 24, threadNumSize);
            seekPos += threadNumSize;
//			Log.e("in message pack func", "bef pack port Pos = " + seekPos);
            for(int port : synack.port){
                temp = toLH(port);
                int portSize = temp.length;
                System.arraycopy(temp, 0, buf, seekPos, portSize);
                seekPos += portSize;
            }

//			Log.e("in message pack func", "bef pack file info, pos = " + seekPos);
            /**
             * pack file info below
             */
            System.arraycopy(synack.fileInfo.fileName.getBytes(), 0, buf, seekPos, synack.fileInfo.fileName.length());
            seekPos += Values.LEN_FILENAME;  // Attention here ! This is important.
//			Log.e("in message pack func", "filename length = " + synack.fileInfo.filename.length());
//			Log.e("in message pack func", "after pack filename Pos = " + seekPos);

//			temp = longtoLH(synack.fileInfo.filesize);
            temp = toLH(synack.fileInfo.fileSize);
            int filesizeSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, filesizeSize);
            seekPos += filesizeSize;

            temp = toLH(synack.fileInfo.breakPoint);
            int breakPointSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, breakPointSize);
            seekPos += breakPointSize;

            temp = toLH(synack.fileInfo.endPoint);
            int endPointSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, endPointSize);
            seekPos += endPointSize;

            temp = toLH(synack.fileInfo.blockSize);
            int blocksizeSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, blocksizeSize);
            seekPos += blocksizeSize;

            temp = toLH(synack.fileInfo.blockNum);
            int blockNumSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, blockNumSize);
            seekPos += blockNumSize;

//			Log.e("in pack", "bef pack downloadedBlock, seekPos = " + seekPos);
            temp = toLH(synack.fileInfo.downloadedBlock);
            int downloadedBlockSize = temp.length;
            System.arraycopy(temp, 0, buf, seekPos, downloadedBlockSize);
            seekPos += downloadedBlockSize;
//			Log.e("in pack", "aft pack downloadedBlock, seekPos = " + seekPos);

        }
        int checkbyte = 7;
        temp = toLH(checkbyte);
        int checkbyteSize = temp.length;
        System.arraycopy(temp, 0, buf, seekPos, checkbyteSize);
    }

    /**
     * 返回要发送的数组
     */
    public byte[] getBuf() {
        return buf;
    }

    public static MyMessage depack2(byte[] originStream) {
        int seekPos = 0;
        byte[] headS = Arrays.copyOfRange(originStream, seekPos, seekPos + 8);
        seekPos += 8;
        byte[] typeS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
        seekPos += 4;
        byte[] packNoS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
        seekPos += 4;
        byte[] lengthS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
        seekPos += 4;

//		Log.e("in depack", "headS :" + headS.toString());

        typeS = ConvertNum.reverseEndian(typeS);
        packNoS = ConvertNum.reverseEndian(packNoS);
        lengthS = ConvertNum.reverseEndian(lengthS);

        String typeHex = ConvertNum.bytesToHexString(typeS);
        String packNoHex = ConvertNum.bytesToHexString(packNoS);
        String lengthHex = ConvertNum.bytesToHexString(lengthS);

        int type = Integer.parseInt(typeHex, 16);
        int packNo = Integer.parseInt(packNoHex, 16);
        int length = Integer.parseInt(lengthHex, 16);

        byte[] body = new byte[Values.BODYLEN];
        SynAck synackPack = new SynAck();

        if (type == Values.DATA) {
            body = Arrays.copyOfRange(originStream, seekPos, seekPos + Values.BODYLEN);
            seekPos += Values.BODYLEN;

        } else if (type == Values.SYN_ACK || type == Values.SYN) {
            byte[] downoadableS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
            byte[] threadNumS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;

            downoadableS = ConvertNum.reverseEndian(downoadableS);
//			Log.e("in depack", "bef recerseEndian, threadNum :" + ConvertNum.bytesToHexString(threadNumS));
            threadNumS = ConvertNum.reverseEndian(threadNumS);
//			Log.e("in depack", "aft recerseEndian, threadNum :" + ConvertNum.bytesToHexString(threadNumS));

            String downloadableHex = ConvertNum.bytesToHexString(downoadableS);
            String threadNumHex = ConvertNum.bytesToHexString(threadNumS);
//			Log.e("in depack", "aft to hex, threadNum :" + threadNumHex);

            synackPack.downloadable = Integer.parseInt(downloadableHex, 16);
            synackPack.threadNum = Integer.parseInt(threadNumHex, 16);

            Log.d("bef array index error", "threadNumber:" + synackPack.threadNum);
            for (int i = 0; i < Values.THREAD_NUMBER; i++) {
                byte[] portS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
//				Log.e("in depack", "[port :" + i + "]bef reverseEndian :" + ConvertNum.bytesToHexString(portS));
                portS = ConvertNum.reverseEndian(portS);
//				Log.e("in depack", "[port :" + i + "]aft reverseEndian :" + ConvertNum.bytesToHexString(portS));
                String portHex = ConvertNum.bytesToHexString(portS);
//				Log.e("in depack", "[port :" + i + "]aft toHex:" + portHex);
                synackPack.port[i] = Integer.parseInt(portHex, 16);
                seekPos += 4;
            }
//			seekPos += Values.THREAD_NUMBER * 4;


//			Log.e("In depack", "after depack port, seekPos = " + seekPos);

            byte[] filenameS = Arrays.copyOfRange(originStream, seekPos, seekPos + Values.LEN_FILENAME);
            synackPack.fileInfo.fileName = new String(filenameS).trim();
            seekPos += Values.LEN_FILENAME;
//			Log.e("In depack", "after depack filename, seekPos = " + seekPos);
//			Log.e("in depack", "filenameS :" + synackPack.fileInfo.fileName);

            byte[] fileSizeS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
            byte[] breakPointS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
            byte[] endPointS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
            byte[] blockSizeS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
            byte[] blockNumS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;

//			Log.e("in depack", "bef pack downloadedBlock, seekPos = " + seekPos);
            byte[] downloadedBlockS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
            seekPos += 4;
//			Log.e("in depack", "aft pack downloadedBlock, seekPos = " + seekPos);

//			Log.e("in depack", "bef reverseEndian, filesize :" + ConvertNum.bytesToHexString(fileSizeS));
            fileSizeS = ConvertNum.reverseEndian(fileSizeS);
//			Log.e("in depack", "aft reverseEndian, filesize :" + ConvertNum.bytesToHexString(fileSizeS));

            breakPointS = ConvertNum.reverseEndian(breakPointS);
            endPointS = ConvertNum.reverseEndian(endPointS);
            blockSizeS = ConvertNum.reverseEndian(blockSizeS);
            blockNumS = ConvertNum.reverseEndian(blockNumS);
            downloadedBlockS = ConvertNum.reverseEndian(downloadedBlockS);

            String fileSizeHex = ConvertNum.bytesToHexString(fileSizeS);
//			Log.e("in depack", "aft toHex, fileSize hex = [" + fileSizeHex + "]");
//			Log.e("for filesize", "aft toHexString, filesize len =" + fileSizeHex.length());

            String breakPointHex = ConvertNum.bytesToHexString(breakPointS);
//			Log.e("in depack", "breakPointHex = [" + breakPointHex + "]");
            String endPointHex = ConvertNum.bytesToHexString(endPointS);
//			Log.e("in depack", "endPointHex = [" + endPointHex + "]");
            String blockSizeHex = ConvertNum.bytesToHexString(blockSizeS);
//			Log.e("in depack", "blockSizeHex = [" + blockSizeHex + "]");
//			Log.e("for blockSize", "aft toHexString, blockSizeHex len" + blockSizeHex.length());

            String blockNumHex = ConvertNum.bytesToHexString(blockNumS);
//			Log.e("in depack", "blockNumHex = [" + blockNumHex + "]");

            byte[] tbBt = downloadedBlockS;
            String tpStr = tbBt.toString();
            if (downloadedBlockS.equals(tpStr)) {
                Log.e("compare", "========== equal");
            }

            String downloadedBlockHex = ConvertNum.bytesToHexString(downloadedBlockS);
//			Log.e("in depack", "downloadedBlockHex = [" + downloadedBlockHex + "]");
//			Log.e("for blockSize", "aft toHexString, downloadedBlockHex len" + downloadedBlockHex.length());

//			synackPack.fileInfo.filesize 		= Long.parseLong(fileSizeHex, 16);
            synackPack.fileInfo.fileSize = Integer.parseInt(fileSizeHex, 16);
            synackPack.fileInfo.breakPoint = Integer.parseInt(breakPointHex, 16);
            synackPack.fileInfo.endPoint = Integer.parseInt(endPointHex, 16);
            synackPack.fileInfo.blockSize = Integer.parseInt(blockSizeHex, 16);
            synackPack.fileInfo.blockNum = Integer.parseInt(blockNumHex, 16);
            synackPack.fileInfo.downloadedBlock = Integer.parseInt(downloadedBlockHex, 16);
        }

        byte[] checkbyteS = Arrays.copyOfRange(originStream, seekPos, seekPos + 4);
        checkbyteS = ConvertNum.reverseEndian(checkbyteS);
        String checkbyteHex = ConvertNum.bytesToHexString(checkbyteS);
        int checkbyte = Integer.parseInt(typeHex, 16);

        MyMessage msg = new MyMessage();
        msg.head = new String(headS).trim();
        msg.type = type;
        msg.packNo = packNo;
        msg.length = length;
        msg.body = body;
        msg.synackPack = synackPack;
        msg.checkbyte = checkbyte;
//		Log.e("in depack", "downloadedBlock = " + msg.synackPack.fileInfo.downloadedBlock + " checkbyte = " + msg.checkbyte);
        return msg;
    }
}