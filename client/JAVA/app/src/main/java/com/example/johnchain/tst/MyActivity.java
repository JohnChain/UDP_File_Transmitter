package com.example.johnchain.tst;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.locks.ReentrantLock;


public class MyActivity extends Activity {

    TextView output1;
    TextView output2;
    EditText hostname;
    EditText filename;
    EditText port;
    TextView result;

    DatagramSocket socket;
    ProgressBar progBar;

    Handler commanderToMainHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);

        Log.i("johnchain", "downloadedBlock:" + Values.downloadedBlock + " blockSize:" + Values.BODYLEN +
                " fileSize:" + Values.fileSize + " blockNum:" + Values.blockNum + "threadNum" + Values.THREAD_NUMBER);

        port = (EditText)findViewById(R.id.port);
        hostname = (EditText)findViewById(R.id.hostname);
        filename = (EditText)findViewById(R.id.filename);
        output1 = (TextView)findViewById(R.id.output1);
        output2 = (TextView)findViewById(R.id.output2);
        Button startBtn = (Button)findViewById(R.id.start);
        Button quitBtn = (Button)findViewById(R.id.quit);
        Button pauseBtn = (Button)findViewById(R.id.pause);

        result = (TextView)findViewById(R.id.result);

        progBar = (ProgressBar)findViewById(R.id.progressBar1);
        progBar.setMax(100);
        port.setText("8888");
        hostname.setText("10.64.57.73");
//		hostname.setText("172.16.0.2");
        filename.setText("tempFile");

        commanderToMainHandler = new Handler(){
            @Override
            public void handleMessage(Message msg){
                if(msg.what == Values.MT_SYN){
                    MyMessage myMsg = (MyMessage) msg.obj;
                    output2.setText(Utils.stringMessage(myMsg));
                    Log.d("johnchain", Utils.stringMessage(myMsg));

                }else if(msg.what == Values.MT_PROG){
                    Bundle bud = msg.getData();
                    progBar.setProgress(bud.getInt("TotalProg"));

                    filename.setText(bud.getString("filename"));

                    output1.setText("commanderAlive:" + Values.commanderLive + " soldierLive:" + Values.soldierLive +
                            " downloadedBlock:" + Values.downloadedBlock + " blockNum:" + Values.blockNum);
                    output2.setText("");

                    int speed = (Values.downloadedBlock - Values.forSpeed) * Values.BODYLEN / 2 / 1000;
                    Values.forSpeed = Values.downloadedBlock;
                    result.setText("cost time: " + Values.timeCost +" s" + " | speed: " + speed + "KB/s");
                }
            }
        };

        startBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    Values.SERV_ADDRESS = InetAddress.getByName(hostname.getText().toString());
                    Values.SERV_PORT = Integer.parseInt(port.getText().toString());
                } catch (UnknownHostException e) {
                    Log.e("johnchain", "Commander UnknownHostException, please input legal hostname");
                    return ;
                }

                CommanderThread stFun = new CommanderThread();
                Thread commanderThread = new Thread(stFun);
                commanderThread.start();

                Values.commanderLive 	= true;
                Values.soldierLive 	= true;
            }
        });
        quitBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Values.commanderLive = false;
            }
        });

        pauseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
//				commanderLive = true;
//				soldierLive = false;

                String result = Arrays.toString(Values.taskArray);
                Log.d("johnchain", "Commander " + result);

                output2.append("CommanderLive = " + Values.commanderLive +
                        " | soldierLiver = " + Values.soldierLive +
                        " | cookLive = " + Values.cookLive);

                output2.append("cost_readFile = " + Values.cost_readFile / 1000 +
                        " | cost_packPack = " + Values.cost_packPack / 1000 +
                        " | cost_recvPack = " + Values.cost_recvPack / 1000 +
                        " | cost_depack = " + Values.cost_depack / 1000);

//                String tempTarget = "sdcard/Download/" + Values.fileName + "_comp";
//                File cmp_file = new File(tempTarget);
//                try {
//                    if(!cmp_file.exists()){
//                        cmp_file.createNewFile();
//                    }
//
//                    FileOutputStream outputStream;
//                    outputStream = new FileOutputStream(cmp_file);
//                    //				p.setProperty("fileSize", Integer.toString(myMsg.synackPack.fileInfo.filesize));
//                    outputStream.write(result.getBytes());
//                    outputStream.flush();
//                } catch (FileNotFoundException e) {
//                    e.printStackTrace();
//                } catch (IOException e) {
//                    // TODO Auto-generated catch block
//                    e.printStackTrace();
//                }
//                if(cmp_file.exists()){
////					cmp_file.delete();
//                }
            }
        });
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
                Log.d("johnchain", "revd done, bytes: " + recvPacket.getLength());

            } catch (IOException e) {
                Log.e("johnchain", "Commander receive SYN_ACK timeout, will resend SYN, send bytes = " + resendPacket.getLength());
                try{
                    socket.send(resendPacket);
                    times ++;
                    continue;
                }catch (IOException e2){
                    Log.e("johnchain", "Commander send SYN failed, transmition aborted");
                    return null;
                }
            }
            myMsg = PacketManager.depack2(recvPacket.getData());
            if(myMsg.type == replyType){
                Log.d("johnchain","Commander received SYN_ACK | " + Utils.stringMessage(myMsg));
                break;
            }else{
                Log.d("johnchain", "Commander received type: " + myMsg.type + " not what we wanted, receive again");
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
        PacketManager mag = new PacketManager(Values.HEAD, Values.SYN, 0, Values.SYNACKLEN, null, synPack);
        DatagramPacket sendPacket = new DatagramPacket(mag.getBuf(), mag.getBuf().length, Values.SERV_ADDRESS, Values.SERV_PORT);
        try {
            socket.send(sendPacket);
            Log.d("johnchain", "Commander send SYN to server bytes = " + sendPacket.getLength() +
                    "|\n" + Utils.stringSynAck(synPack));
        } catch (IOException e) {
            Log.e("johnchain", "Commander send SYN failed, transmition aborted");
            return Values.RST_ERROR;
        }

        try {
            socket.setSoTimeout(Values.SOCKET_TIMEOUT * 2);
        } catch (SocketException e1) {
            Log.e("johnchain", "Commander set timeout failed");
        }

        MyMessage myMsg = waitReply(Values.SYN_ACK, sendPacket);
        if(myMsg != null)
            return Values.RST_OK;
        else
            return Values.RST_TIMEOUT;
    }

    public void initNew(){
        Values.forSpeed = 0;
        Values.taskIndex = 0;
        Values.soldierLive = true;

    }

    public class CommanderThread extends Thread{
        private final ReentrantLock lock = new ReentrantLock();

        @Override
        public void run(){
            super.run();
            Log.d("johnchain", "Commander in");

            try{
                socket = new DatagramSocket(Values.LOCAL_PORT); // bind local port
            }catch (SocketException e){
                e.printStackTrace();
            }
            if(socket == null){
                Log.e("johnchain", "Commander socket is null, transmition aborted");
                return ;
            }

            Log.e("johnchain", "Commander bind port to socket done");

            DatagramPacket recvPacket = null;
            DatagramPacket sendPacket = null;
            SynAck synPack = new SynAck();
            synPack.threadNum = Values.THREAD_NUMBER;
            synPack.port[0] = 9999;
            synPack.port[1] = 9989;
            synPack.fileInfo.blockSize = Values.BODYLEN;

            Cook cook = new Cook();
            cook.msg = commanderToMainHandler;
            Thread cookThread = new Thread(cook);
            Values.cookLive = true;
            cookThread.start();

//			if(true) return ;

            while(Values.commanderLive){

                while(Values.commanderLive && Values.fileQueue.isEmpty()){
                    synchronized(Values.fileQueue){
                        try {
                            Values.fileQueue.wait(2000);
                        } catch (InterruptedException e) {
                            Log.d("johnchain", "Commander interrupted while wait for fileQueue");
                            continue;
                        }
                    }
                    Log.d("johnchain", "Commander check fileQueue again");
                }

                Values.fileName = (String) Values.fileQueue.poll();
                if(Values.fileName == null){
                    continue;
                }

                File taskFile = new File(Values.path + Values.fileName);
                File confFile = new File(Values.path + ".diag_log.conf");

                Values.fileSize = (int) taskFile.length();
                if(Values.fileSize % Values.BODYLEN == 0)
                    Values.blockNum = Values.fileSize / Values.BODYLEN;
                else
                    Values.blockNum = Values.fileSize / Values.BODYLEN + 1;

                if(Values.isNewTask == true){

                    synPack.downloadable = 0;
                    Values.downloadedBlock = 0;
                    Values.taskArray = new int[Values.blockNum];
                }else{
                    Values.isNewTask = true;  //此处设置，仅对第一次启动时为断点传输情况有效

                    synPack.downloadable = 1;
                    // TODO 计算已下载数据块总数
                    for(int i : Values.taskArray){
                        if(i == Values.DONE)
                            Values.downloadedBlock ++;
                    }
                    if(Values.downloadedBlock == Values.blockNum){
                        Log.e("johnchain", "Commander transmition already finished");
                        return ;
                    }
                }

                synPack.fileInfo.fileName = Values.fileName;
                synPack.fileInfo.fileSize = Values.fileSize;
                synPack.fileInfo.blockNum = Values.blockNum;
                synPack.fileInfo.downloadedBlock = Values.downloadedBlock;

                PacketManager mag = new PacketManager(Values.HEAD, Values.SYN, 0, Values.SYNACKLEN, null, synPack);
                sendPacket = new DatagramPacket(mag.getBuf(), mag.getBuf().length, Values.SERV_ADDRESS, Values.SERV_PORT);

				/* 发送连接请求 */
                Log.d("johnchain", "Commander Sent SYN detail(bytes: " + sendPacket.getLength() + ") : " + Utils.stringSynAck(synPack));

                try {
                    socket.send(sendPacket);
                } catch (IOException e1) {
                    Log.e("johnchain", "Commander Network error, Network unreachable and transmition aborted");
                    Values.cookLive = false;
                    Values.soldierLive = false;
                    Values.commanderLive = false;
                    return ;
                }

				/* 等待请求回应 */
                final byte[] buf = new byte[Values.MAXLEN];
                recvPacket = new DatagramPacket(buf, buf.length);

                int i;
                for(i = 0; i < 3; i++) {
                    try {
                        socket.setSoTimeout(Values.SOCKET_TIMEOUT);
                        socket.receive(recvPacket);
                        break;
                    } catch (IOException e) {
                        Log.e("johnchain", "Commander timed out to receive SYN_ACK, send SYN again");
                        continue;
                    }
                }
                if(i == 3) {
                    Log.e("johnchain", "Commander fail to receive SYN_ACK, please check the network");
                    return;
                }
                Log.d("johnchain", "Commander recv SYN_ACK byte length = " + recvPacket.getLength());
                MyMessage myMsg = PacketManager.depack2(recvPacket.getData());
                Log.d("johnchain","Commander recv SYN_ACK byte " +Utils.stringSynAck(myMsg.synackPack));


                Message m = commanderToMainHandler.obtainMessage();
                m.obj = myMsg;
                m.what = Values.MT_SYN;
//				commanderToMainHandler.sendMessage(m);

                initNew();

                if(true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
//                    continue;
                }

//                monitorSoldier(myMsg);

				/* 记录下传输信息 */
//                FileOutputStream outputStream;
//                try {
//                    Properties p = new Properties();
//                    outputStream = new FileOutputStream(confFile);
//                    p.setProperty("fileName", Values.fileName);
//                    p.setProperty("fileSize", Integer.toString(Values.fileSize));
//                    p.setProperty("costtime", Long.toString(Values.timeCost));
//                    p.setProperty("detail", Arrays.toString(Values.taskArray));
//                    p.store(outputStream, "Init in commander");
//                    outputStream.flush();
//                    outputStream.close();
//                } catch (FileNotFoundException e) {
//                    Log.e("johnchain", "Commander configure file not exists");
//                } catch (IOException e) {
//                    Log.e("johnchain", "Commander configure file operate error");
//                }
//				break;
            }

            Log.d("johnchain", "Commander exit while");
            Values.cookLive = false;
            try {
//                cook.join();
                cookThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            socket.close();
            Log.d("johnchain", "Commander exit");
        }
        public void monitorSoldier(MyMessage myMsg){
            /**
             * 开启多线程
             */

            Thread[] threadList = new Thread[Values.THREAD_NUMBER];
            for(int i = 0; i < Values.THREAD_NUMBER; i ++){
                Soldier soldier = new Soldier();
                soldier.myMsg = Utils.deepCopyMsg(myMsg);
                soldier.threadId = i + 1;
                Thread soldierThread = new Thread(soldier);
                soldierThread.start();
                threadList[i] = soldierThread;
            }
            /**
             * 轮询查看下载进度，并告知UI线程
             */
            boolean monitorLive = true;
            long t1 = System.currentTimeMillis();
            Log.d("johnchain", "Commander commanderLive = " + Values.commanderLive);
            while(!Thread.currentThread().isInterrupted() && monitorLive && Values.commanderLive){
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e) {
                    Log.e("johnchain", "Commander get interrupt signal from UI thread");
                    break;
                }

                if(Values.downloadedBlock >= Values.blockNum){
                    Log.i("johnchain", "Commander downloaded already finished");
                    monitorLive = false;
                }
                Message msg = commanderToMainHandler.obtainMessage();
                msg.what = Values.MT_PROG;
                Bundle bundle = new Bundle();

                try{
                    int prog = (Values.downloadedBlock * 100 / Values.blockNum);
                    bundle.putInt("TotalProg", prog);
                    bundle.putString("filename", Values.fileName);
                    msg.setData(bundle);
//		    		Log.d("johnchain" ,"Commander prog = " + prog + " [" + Values.downloadedBlock + "/" + Values.blockNum + "]");

                    Values.timeCost = (System.currentTimeMillis()-t1) / 1000;
                    commanderToMainHandler.sendMessage(msg);
                }catch (ArithmeticException e){
                    Log.e("johnchain", "Commander Uninitialed blockNum, 0");
                    monitorLive = false;
                }
            }
            Values.soldierLive = false;
            for(Thread thread : threadList){
                if(thread.isAlive()){
                    try {
                        thread.join();
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            }
        }
        public class Soldier extends Thread{
            public MyMessage myMsg = new MyMessage();
            public int threadId;

            @Override
            public void run(){
                super.run();
                SoldierMission soldierMission = new SoldierMission(threadId, myMsg);
                soldierMission.processMission();
            }
        }
    }



    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.my, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
