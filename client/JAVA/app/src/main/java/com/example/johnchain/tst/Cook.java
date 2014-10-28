package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Properties;
import java.util.TimeZone;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class Cook extends Thread{
    public MyMessage myMsg = new MyMessage();
    public Handler msg;
    public int threadId;

    long lastTime = 0;

    @Override
    public void run(){
        super.run();
        Log.d("johnchain", "Cook in");

        File confFile = new File(Values.path + ".diag_log.conf");
        if(confFile.exists()){
            Values.isNewTask = false;
            Log.d("johnchain", "Cook conf file exist, old task");
            Properties p = new Properties();
            try {
                p.load(new FileInputStream(confFile));
                Values.fileName = p.getProperty("fileName");

                if(Values.fileName == null){
                    Log.d("johnchain", "filename is null");
                    lastTime = 0;
                }else{
                    File tf = new File(Values.path + Values.fileName);
                    lastTime = tf.lastModified();  // return 0 if file not exist
                    Log.d("johnchain", "filename " + Values.fileName + " is not null, last time = " + lastTime);
                }

                if(lastTime > 0) {
                    Values.taskArray = Utils.arrayStringToIntArray(p.getProperty("detail"));
                    Log.d("johnchain", "Cook last file in conf exist");
                }else {
                    Values.taskArray = new int[Values.blockNum];
                    Log.d("johnchain", "Cook last file in conf no longer exist");
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }else{
            Values.isNewTask = true;
            Values.fileName = "";
            Log.d("johnchain", "Cook conf file not exist, new task");
            try {
                confFile.createNewFile();
            } catch (IOException e) {
                Log.d("johnchain", "Cook create confiugre file failed");
            }

            Values.taskArray = new int[Values.blockNum];
        }
//
//        Log.d("johnchain", "Cook Read conf file : " + Utils.intArrayToArrayString(Values.taskArray));

//        if(true) return ;

        while(!Thread.currentThread().isInterrupted() && Values.cookLive){
//			Values.fileQueue.clear();

//            File ppp = new File(Values.path);
//            if(ppp.canRead()){
//                Log.d("johnchain", "Cook path can read");
//            }else if(ppp.canWrite()){
//                Log.d("johnchain", "Cook path can write");
//            }else if(ppp.canExecute()){
//                Log.d("johnchain", "Cook path can execute");
//            }else{
//                Log.d("johnchain", "Cook path :" + Values.path + " unknown property");
//            }

			/* 获取目标文件列表 */
            File[] fileList = Utils.getFileList(Values.path);
            if(fileList == null){
                Log.w("johnchain", "Cook get no file List");
                break;
            }
            List<File> orderedList = new ArrayList<File>();
            for(int i = 0; i < fileList.length; i++){
                if(fileList[i].lastModified() >= lastTime)
                    if(Utils.checkSuffix(fileList[i].getName(), "qmdl"))
                        orderedList.add(fileList[i]);
            }

			/* 文件列表按由旧到新顺序排序 */
//            Log.d("johnchain", "bef sort");
            List<File> newFileList = Utils.sortFileList(orderedList);
//			Log.d("johnchain", "after sort");
            int i;
            for(i = 0; i < newFileList.size() - 1; i++){
                Values.fileQueue.add(newFileList.get(i).getName());
				Log.d("johnchain", "Cook add file: " + newFileList.get(i).getName());
            }

            if(! Values.fileQueue.isEmpty()){
                lastTime = newFileList.get(i).lastModified();
//                Log.d("johnchain", "lastTime = " + lastTime);
                synchronized(Values.fileQueue){
                    Values.fileQueue.notifyAll();
                }
            }else{
                Log.d("johnchain", "Cook no new file found ");
            }
            try {
                Thread.sleep(1000);
                Log.d("johnchain", "Cook awake");
                if(Values.fileName == null)
                    continue;

                FileOutputStream outputStream;
                try {
                    Properties p = new Properties();
                    outputStream = new FileOutputStream(confFile);

                    p.setProperty("fileName", Values.fileName);
                    p.setProperty("fileSize", Integer.toString(Values.fileSize));
                    p.setProperty("costtime", Long.toString(Values.timeCost));
                    p.setProperty("detail", Arrays.toString(Values.taskArray));

                    Date now = new Date();
                    SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");//可以方便地修改日期格式
                    dateFormat.setTimeZone(TimeZone.getTimeZone("GMT+8"));
                    String settime = dateFormat.format( now );

                    p.store(outputStream, "Init in commander at: " + settime);

                    outputStream.flush();
                    outputStream.close();
                } catch (FileNotFoundException e) {
                    Log.e("johnchain", "Cook configure file not exists");
                } catch (IOException e) {
                    Log.e("johnchain", "Cook configure file operate error");
                }

            } catch (InterruptedException e) {
                Log.d("johnchain", "Cook interrupted while sleep");
            }
        }
    }
}
