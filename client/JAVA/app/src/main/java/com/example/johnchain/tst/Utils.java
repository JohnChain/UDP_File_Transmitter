package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.util.Log;

public class Utils {
    public static String stringMessage(MyMessage myMsg){
        String output;
        if(myMsg.type == Values.DATA){
            output = "[head = " + myMsg.head + "][type = " + myMsg.type +
                    "][packNo = " + myMsg.packNo + "][length = " + myMsg.length +
                    "][checkbyte = " + myMsg.checkbyte + "]";
        }else{
            output = "[head = " + myMsg.head + "][type = " + myMsg.type +
                    "][packNo = " + myMsg.packNo + "][length = " + myMsg.length + "]\n" +
                    "[downloadable = " + myMsg.synackPack.downloadable + "]" +
                    "[threadNum = " + myMsg.synackPack.threadNum + "]" +
                    "[port[0] = " + myMsg.synackPack.port[0] + "]" +
                    "[port[1] = " + myMsg.synackPack.port[1] + "]" +
                    "[port[2] = " + myMsg.synackPack.port[2] + "]" +
                    "[port[3] = " + myMsg.synackPack.port[3] + "]" +
                    "[filaname = " + myMsg.synackPack.fileInfo.fileName + "]" +
                    "[filesize = " + myMsg.synackPack.fileInfo.fileSize + "]" +
                    "[breakpoint = " + myMsg.synackPack.fileInfo.breakPoint + "]" +
                    "[endpoint = " + myMsg.synackPack.fileInfo.endPoint + "]" +
                    "[blocksize = " + myMsg.synackPack.fileInfo.blockSize + "]" +
                    "[blocknum = " + myMsg.synackPack.fileInfo.blockNum + "]" +
                    "[downloaded_block = " + myMsg.synackPack.fileInfo.downloadedBlock + "]" +
                    "[checkbyte = " + myMsg.checkbyte + "]";
        }
        return output;
    }

    public static String stringSynAck(SynAck synackPack){
        String output = "[downloadable = " + synackPack.downloadable + "]" +
                "[threadNum = " + synackPack.threadNum + "]" +
                "[port[0] = " + synackPack.port[0] + "]" +
                "[port[1] = " + synackPack.port[1] + "]" +
                "[port[2] = " + synackPack.port[2] + "]" +
                "[port[3] = " + synackPack.port[3] + "]" +
                "[filaname = " + synackPack.fileInfo.fileName + "]" +
                "[filesize = " + synackPack.fileInfo.fileSize + "]" +
                "[breakpoint = " + synackPack.fileInfo.breakPoint + "]" +
                "[endpoint = " + synackPack.fileInfo.endPoint + "]" +
                "[blocksize = " + synackPack.fileInfo.blockSize + "]" +
                "[blocknum = " + synackPack.fileInfo.blockNum + "]" +
                "[downloaded_block = " + synackPack.fileInfo.downloadedBlock + "]";
        return output;
    }

    public static MyMessage deepCopyMsg( MyMessage myMsg){
        MyMessage msg = new MyMessage();
        msg.head = myMsg.head;
        msg.type = myMsg.type;
        msg.packNo = myMsg.packNo;
        msg.length = myMsg.length;

        SynAck synPack = new SynAck();
        SynAck synPack1 = myMsg.synackPack;
        synPack.downloadable = synPack1.downloadable;
        synPack.threadNum = synPack1.threadNum;
        for(int i = 0; i < synPack1.threadNum; i++)
            synPack.port[i] = synPack1.port[i];
        synPack.fileInfo.fileName = synPack1.fileInfo.fileName;
        synPack.fileInfo.fileSize = synPack1.fileInfo.fileSize;
        synPack.fileInfo.breakPoint = synPack1.fileInfo.breakPoint;
        synPack.fileInfo.endPoint = synPack1.fileInfo.endPoint;
        synPack.fileInfo.blockSize = synPack1.fileInfo.blockSize;
        synPack.fileInfo.blockNum = synPack1.fileInfo.blockNum;
        synPack.fileInfo.downloadedBlock = synPack1.fileInfo.downloadedBlock;

        msg.synackPack = synPack;
        return msg;
    }


    public static File[] getFileList(String filePath) {
        ArrayList<String> items = new ArrayList<String>();
        ArrayList<String> paths = new ArrayList<String>();
//		Log.d("johnchain", "当前路径: "+filePath);// 设置当前所在路径

        File[] files = null;
        try{
            File f = new File(filePath);
            files = f.listFiles();// 列出所有文件
            // 将所有文件存入list中
            if(files != null){
//                Log.d("johnchain", "files not null");
//                int count = files.length;// 文件个数
//                for (int i = 0; i < count; i++) {
//                    File file = files[i];
//                    items.add(file.getName());
//                    paths.add(file.getPath());
////                    Log.d("johnchain", "add file " + file.getName());
////                    Log.d("johnchain", "add path " + file.getPath());
//                }
            }else{
                Log.d("johnchain", "files null");
            }
        }catch(Exception ex){
            ex.printStackTrace();
        }
        return files;
    }

    public static List<File> sortFileList(List<File> orderedList){

        Collections.sort(orderedList, new Comparator<File>(){
            public int compare(File file1, File file2)
            {
//				  return file1.compareToIgnoreCase(file2);
                if(file1.lastModified() < file2.lastModified()){
//					  Log.d("johnchain", "in compare " + file1.getName() + "is less than" + file2.getName());
                    return -1;
                }else{
//					  Log.d("johnchain", "in compare " + file1.getName() + "is greater than" + file2.getName());
                    return 1;
                }
            }
        });

        return orderedList;
    }


    public static boolean checkSuffix(String fileName, String suffix){
        String FileEnd = fileName.substring(fileName.lastIndexOf(".") + 1, fileName.length()).toLowerCase();
        if(FileEnd.equals(suffix)){
            return true;
        }else
            return false;
    }


    static String intArrayToArrayString(int[] array) {

        // 利用 Arrays.toString 可以超簡單輸出 array
        // 輸出結果：[4, 2, 5, 1, 5, 2, 4, 3]
        String arrayString = Arrays.toString(array);
//        System.out.println(arrayString);
        return arrayString;
    }

    static int[] arrayStringToIntArray(String arrayString) {

        // 將剛剛輸出之 array string 先作去頭去尾處理
        // 並用 split 來分開各個項目
        String[] items = arrayString.replaceAll("\\[", "")
                .replaceAll("\\]", "").split(",");

        // items.length 是所有項目的個數
        int[] results = new int[items.length];

        // 將結果放入 results，
        // 並利用 Integer.parseInt 來將整數字串轉換為 int
        for (int i = 0; i < items.length; i++) {
            results[i] = Integer.parseInt(items[i].trim());
        }

        // 此時已將字串轉換至 results 中，
        // 但為了檢查，我們還是要把 results 印出來。
        // 輸出結果：4, 2, 5, 1, 5, 2, 4, 3,
//        for (int element : results) {
//            System.out.print(element + ", ");
//        }

        return results;
    }

}