package mohsen.project;


import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.DhcpInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Message;
import android.support.v7.app.ActionBarActivity;
import android.text.format.Formatter;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;


import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import android.os.Handler;
import java.net.SocketException;
import java.net.UnknownHostException;


public class MyActivity extends ActionBarActivity implements View.OnClickListener{

    public static final String TAG="MAR";
    public TextView textTemp;
    public TextView textLon;
    public TextView textLof;
    public EditText ipInput;
    public EditText portInput;
    public Button btn1;
    public Button btn2;
    public Button btn3;
    public Handler Handler;
    public static String dIP;
    public static String Port;
    public byte[] buf;
    public int identifier;

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);
        textTemp = (TextView) findViewById(R.id.textView1);
        textLon = (TextView) findViewById(R.id.textView2);
        textLof = (TextView) findViewById(R.id.textView3);
        ipInput = (EditText) findViewById(R.id.IPAddr);
        portInput = (EditText) findViewById(R.id.Port);
        btn1 = (Button) findViewById(R.id.on_button);
        btn1.setOnClickListener(this);
        btn2 = (Button) findViewById(R.id.off_button);
        btn2.setOnClickListener(this);
        btn3 = (Button) findViewById(R.id.temp_button);
        btn3.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {

        if(v.getId()==R.id.off_button) {
            textLon.setText("");
            textLof.setText("");
            buf=("mar,c=0").getBytes();
            sendingAndrecievingToAnotherClient();
            identifier=0;
            InputMethodManager inputManager = (InputMethodManager)
                    getSystemService(Context.INPUT_METHOD_SERVICE);

            inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(),
                    InputMethodManager.HIDE_NOT_ALWAYS);
        }
        else if(v.getId()==R.id.on_button) {
            textLof.setText("");
            textLon.setText("");
            buf=("mar,c=1").getBytes();
            sendingAndrecievingToAnotherClient();
            identifier=1;
            InputMethodManager inputManager = (InputMethodManager)
                    getSystemService(Context.INPUT_METHOD_SERVICE);

            inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(),
                    InputMethodManager.HIDE_NOT_ALWAYS);
        }
        else if(v.getId()==R.id.temp_button) {
            textTemp.setText("");
            buf=("mar,c=2").getBytes();
            identifier=2;
            sendingAndrecievingToAnotherClient();
            InputMethodManager inputManager = (InputMethodManager)
                    getSystemService(Context.INPUT_METHOD_SERVICE);

            inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(),
                    InputMethodManager.HIDE_NOT_ALWAYS);
        }


    }
    private void sendingAndrecievingToAnotherClient()
    {
        new Thread(new Client()).start();
        new Thread(new Server()).start();
        Handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                String st = (String) msg.obj;
                if (identifier==2) {
                    textTemp.append(st);
                }
                else if(identifier==1) {
                    textLon.append(st);
                }
                else if(identifier==0) {
                    textLof.append(st);
                }
            }
        };

    }

    public class Client implements Runnable {
        @Override
        public void run() {
            DatagramSocket csocket=null;
            try {
                csocket = new DatagramSocket();
                int DestPort=0;
                if(!portInput.getText().toString().isEmpty())
                {
                    DestPort=Integer.parseInt(portInput.getText().toString());
                }
                else
                {
                    DestPort=1200;
                }
                String ClientIP;
                if(!ipInput.getText().toString().isEmpty())
                {
                    ClientIP=ipInput.getText().toString();
                }
                else
                {
                    ClientIP="192.168.1.130";
                }
                InetAddress clientAddr = InetAddress.getByName(ClientIP);
                DatagramPacket packet = new DatagramPacket(buf,
                        buf.length, clientAddr, DestPort);
                csocket.send(packet);
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
                InetAddress serverAddr = InetAddress.getByName(ServerIP);
                int sourcePort;
                if(!portInput.getText().toString().isEmpty())
                {
                    sourcePort=Integer.parseInt(portInput.getText().toString());
                }
                else {
                    sourcePort = 1200;
                }
                byte[] rbuf = new byte[64];
                socket = new DatagramSocket(sourcePort, serverAddr);
                DatagramPacket packet = new DatagramPacket(rbuf, rbuf.length);
                socket.receive(packet);
                if(identifier==2) {
                    updateTrack("Temperature=" + new String(packet.getData()) + "\u2103");
                }else{
                    updateTrack(new String(packet.getData()));
                }
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
       // cmdInput.setText(cInput);

    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();

        dIP = ipInput.getText().toString();
        Port = portInput.getText().toString();
       // cInput = cmdInput.getText().toString();

    }

}



