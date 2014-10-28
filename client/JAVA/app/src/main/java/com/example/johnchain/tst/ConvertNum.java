package com.example.johnchain.tst;

public class ConvertNum {
    /* *
     * Convert byte[] to hex string.这里我们可以将byte转换成int，然后利用Integer.toHexString(int)
     *来转换成16进制字符串。
     * @param src byte[] data
     * @return hex string
     */
    public static String bytesToHexString(byte[] src){
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }

    public static byte[] reverseEndian(byte[] s){
//		StringBuffer s_buf = new StringBuffer(s);
//		s_buf.reverse();
//		return s_buf.toString();
        int middle = s.length / 2 - 1;
        for(int i = 0; i <= middle; i++){
            byte t = s[i];
            s[i] = s[s.length - i - 1];
            s[s.length - i - 1] = t;
        }
        return s;
    }
}
