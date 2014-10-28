package com.example.johnchain.tst;

/**
 * Created by johnchain on 14-9-29.
 */
public class MyMessage {
    public String head = "";
    public int type = 0;
    public int packNo = 0;
    public int length = 0;

    public byte[] body = new byte[Values.BODYLEN];
    public SynAck synackPack = null;
    public int checkbyte = 0;
}