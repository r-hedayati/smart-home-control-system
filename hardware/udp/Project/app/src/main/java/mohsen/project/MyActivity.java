package mohsen.project;


import android.app.Activity;
import android.app.ActivityManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.DhcpInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Bundle;
import android.os.Message;
import android.support.v7.app.ActionBarActivity;
import android.text.format.Formatter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.net.wifi.p2p.WifiP2pInfo;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import android.os.Handler;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.channels.Channel;
import java.util.ArrayList;
import java.util.List;


public class MyActivity extends ActionBarActivity  implements View.OnClickListener{

    //public static final String ClientIP = "192.168.88.30";
    //public static final String ServerIP = "192.168.1.52";
    //public static final int Port = 7000;
    public static final String TAG="MAR";
    public TextView text;
    public EditText ipInput;
    public EditText portInput;
    public EditText cmdInput;
    public Button btn1;
    public Button btn2;
    public Handler Handler;
    private ProgressDialog progress;
    public static String dIP;
    public static String Port;
    public static String cInput;

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);
        text = (TextView) findViewById(R.id.textView1);
        ipInput = (EditText) findViewById(R.id.IPAddr);
        portInput = (EditText) findViewById(R.id.Port);
        cmdInput = (EditText) findViewById(R.id.Command);
        btn1 = (Button) findViewById(R.id.on_button);
        btn1.setOnClickListener(this);
        btn2 = (Button) findViewById(R.id.off_button);
        btn2.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {

        if(v.getId()==R.id.on_button) {
            text.setText("");
            sendingAndrecievingToAnotherClient();
            /*InputMethodManager inputManager = (InputMethodManager)
                    getSystemService(Context.INPUT_METHOD_SERVICE);

            inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(),
                    InputMethodManager.HIDE_NOT_ALWAYS);*/
        }
        else if(v.getId()==R.id.off_button) {

        }


    }
    private void sendingAndrecievingToAnotherClient()
    {
        new Thread(new Client()).start();
        new Thread(new Server()).start();
        Handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                String text1 = (String) msg.obj;
                text.append(text1);
            }
        };

    }

    public class Client implements Runnable {
        @Override
        public void run() {
            DatagramSocket csocket=null;
            try {
                //csocket.setBroadcast(true);
                csocket = new DatagramSocket();
                //updateTrack("Client: Start connecting\n");
                int DestPort=0;
                if(!portInput.getText().toString().isEmpty())
                {
                    DestPort=Integer.parseInt(portInput.getText().toString());
                }
                else
                {
                    DestPort=1234;
                }
                String ClientIP;
                if(!ipInput.getText().toString().isEmpty())
                {
                    ClientIP=ipInput.getText().toString();
                }
                else
                {
                    ClientIP="192.168.1.60";
                }
                InetAddress clientAddr = InetAddress.getByName(ClientIP);
                byte[] buf;
                if(!cmdInput.getText().toString().isEmpty())
                {
                    buf=cmdInput.getText().toString().getBytes();
                }
                else
                {
                    buf = ("Empty Command").getBytes();
                }
                DatagramPacket packet = new DatagramPacket(buf,
                        buf.length, clientAddr, DestPort);
                //updateTrack("Client: Sending ‘" + new String(buf) + "’\n");
                csocket.send(packet);
                //updateTrack("Client: Message sent\n");
               // updateTrack("Client: Succeed!\n");
            } catch (SocketException e) {
                e.printStackTrace();
            }catch (UnknownHostException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (csocket != null) {
                    csocket.close();
                }
            }
        }
    }


  /*private InetAddress getBroadcastAddress() throws IOException {
        WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        DhcpInfo dhcp = wifi.getDhcpInfo();
       if (dhcp==null){
           Log.d(TAG,"Could not get dhcp info");
           return null;
       }

        int broadcast = (dhcp.ipAddress & dhcp.netmask) | ~dhcp.netmask;
        byte[] quads = new byte[4];
        for (int k = 0; k < 4; k++)
            quads[k] = (byte) ((broadcast >> k * 8) & 0xFF);
        return InetAddress.getByAddress(quads);
    }*/

   public class Server implements Runnable {

        @Override
        public void run() {
            DatagramSocket socket=null;
            try {
                WifiManager wifiMgr = (WifiManager) getSystemService(WIFI_SERVICE);
                WifiInfo wifiInfo = wifiMgr.getConnectionInfo();
                int ip = wifiInfo.getIpAddress();
                String ServerIP = Formatter.formatIpAddress(ip);
                InetAddress serverAddr = InetAddress.getByName("192.168.1.52");
                int sourcePort;
                if(!portInput.getText().toString().isEmpty())
                {
                    sourcePort=Integer.parseInt(portInput.getText().toString());
                }
                else
                {
                    sourcePort=1234;
                }
                updateTrack1("\nServer: Start connecting\n");
                byte[] buf = new byte[64];
                socket = new DatagramSocket(sourcePort, serverAddr);
                DatagramPacket packet = new DatagramPacket(buf, buf.length);
                updateTrack1("Server: Receiving\n");
                socket.receive(packet);
                updateTrack1("Server: Message received: ‘" + new String(packet.getData()) + "’\n");
                //updateTrack1("Server: Succeed!\n");
            } catch (SocketException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (socket != null) {
                    socket.close();
                }
            }
        }
    }

    public void updateTrack(String s) {
        Message msg = new Message();
        String textTochange = s;
        msg.obj = textTochange;
        Handler.sendMessage(msg);
    }

    public void updateTrack1(String s) {
        Message msg = new Message();
        String textTochange = s;
        msg.obj = textTochange;
        //Handler.sendMessage(msg);
        Handler.sendMessageDelayed(msg, 50);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_activity_actions, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        switch (item.getItemId()) {

            case R.id.action_settings:
                //openSettings();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();

        ipInput.setText(dIP);
        portInput.setText(Port);
        cmdInput.setText(cInput);

    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();

        dIP = ipInput.getText().toString();
        Port = portInput.getText().toString();
        cInput = cmdInput.getText().toString();

    }

}



