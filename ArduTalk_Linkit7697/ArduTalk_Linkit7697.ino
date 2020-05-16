//   ArduTalk
#define DefaultIoTtalkServerURL  "http://iottalk.niu.edu.tw/play/"
#define DM_name  "Linkit7697" 
#define DF_list  {"P0","P1","P2","P3","P4~","P5~","P8~","P9~", "P10", "P11", "P12", "P13", "P14~", "P15~", "P16~", "P17~"}
#define nODF     10  // The max number of ODFs which the DA can pull.

#define ON_BOARD_LED_PIN 7
#define ON_BOARD_BTN_PIN 6

// linkit 7697 HTTP wrapper
#include "wrapper/server/ServerWrapper.cpp"
#include "wrapper/client/ClientWrapper.cpp"
#include <EEPROM.h>
#include <LWatchDog.h>

char IoTtalkServerURL[100] = "";
String result;
String url = "";
String passwordkey ="";
ClientWrapper http;

unsigned long lastResetPressTime;
String ap_info;

// remove white space
String remove_ws(const String& str )
{
    String str_no_ws ;
    for( char c : str ) if( !std::isspace(c) ) str_no_ws += c ;
    return str_no_ws ;
}

void clr_eeprom(){
    bool reset = false;
    int sig = digitalRead(ON_BOARD_BTN_PIN);

    if(sig == 1) {
      Serial.println("Count down 3 seconds to clear EEPROM.");

      digitalWrite(ON_BOARD_LED_PIN, HIGH);
      lastResetPressTime = millis();
      while(digitalRead(ON_BOARD_BTN_PIN)) {
        if(millis() - lastResetPressTime >= 3000) {
          reset = true;
          break;
        }
      }

      if(reset) {
        digitalWrite(ON_BOARD_LED_PIN, LOW);
        
        Serial.println("Clear EEPROM and reboot.");
        for(int i = 0; i < EEPROM.length(); i++) EEPROM.write(i, 0);
        
        if(digitalRead(ON_BOARD_BTN_PIN)) {
          Serial.println("Please release button to reboot.");
          while(digitalRead(ON_BOARD_BTN_PIN));
        }
        // Use watch dog to reboot
        LWatchDog.begin(1);
        Serial.println("Reboot in 1 second. Please wait.");
      } else {
        Serial.println("Cancel.");
      }
    }
}

void save_netInfo(char *wifiSSID, char *wifiPASS, char *ServerIP){  //stoage format: [SSID,PASS,ServerIP]
    char *netInfo[3] = {wifiSSID, wifiPASS, ServerIP};
    int addr=0,i=0,j=0;
    EEPROM.write (addr++,'[');  // the code is equal to (EEPROM.write (addr,'[');  addr=addr+1;)
    for (j=0;j<3;j++){
        i=0;
        while(netInfo[j][i] != '\0') EEPROM.write(addr++,netInfo[j][i++]);
        if(j<2) EEPROM.write(addr++,',');
    }
    EEPROM.write (addr++,']');
//    EEPROM.commit();
}

int read_netInfo(char *wifiSSID, char *wifiPASS, char *ServerIP){   // storage format: [SSID,PASS,ServerIP]
    char *netInfo[3] = {wifiSSID, wifiPASS, ServerIP};
    String readdata="";
    int addr=0;
  
    char temp = EEPROM.read(addr++);
    if(temp == '['){
        for (int i=0; i<3; i++){
            readdata ="";
            while(1){
                temp = EEPROM.read(addr++);
                if (temp == ',' || temp == ']') break;
                readdata += temp;
            }
            readdata.toCharArray(netInfo[i],100);
        }

        if (String(ServerIP).length () < 7){
            Serial.println("ServerIP loading failed.");
            return 2;
        }
        else{ 
            Serial.println("Load setting successfully.");
            return 0;
        }
    }
    else{
        Serial.println("no data in eeprom");
        return 1;
    }
}


String scan_network(void){
    int AP_N,i;  //AP_N: AP number 

    // scan for nearby networks:
    Serial.println("** Scan Networks **");
    AP_N = WiFi.scanNetworks();
    if (AP_N == -1) {
        Serial.println("Couldn't get a wifi connection");
    }

    // print the list of networks seen:
    Serial.print("number of available networks:");
    Serial.println(AP_N);

    // combine the ssid list to html
    String AP_List="<select name=\"SSID\" style=\"width: 150px; font-size:16px; color:blue; \" required>" ;// make ap_name in a string
    AP_List += "<option value=\"\" disabled selected>Select AP</option>";

    if(AP_N > 0) {
        for (i = 0; i < AP_N; i++) {
            AP_List += "<option value=\""+String((char*) WiFi.SSID(i))+"\">" + String((char*) WiFi.SSID(i)) + "</option>";
            Serial.print(i);
            Serial.print(") ");
            Serial.print(WiFi.SSID(i));
            Serial.print("\tSignal: ");
            Serial.print(WiFi.RSSI(i));
            Serial.println(" dBm");
        }
    }
    else AP_List = "<option value=\"\">NO AP</option>";
    AP_List +="</select><br><br>";
    return(AP_List); 
}

ServerWrapper server ( 80 );
void handleRoot(int retry){
  String temp = "<html><title>Wi-Fi Setting</title>";
  temp += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>";
  temp += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body bgcolor=\"#F2F2F2\">";
  if (retry) temp += "<font color=\"#FF0000\">Please fill all fields.</font>";
  temp += "<form action=\"setup\"><div>";
  temp += "<center>SSID:<br>";
  temp += ap_info; 
  temp += "Password:<br>";
  temp += "<input type=\"password\" name=\"Password\" vplaceholder=\"Password\" style=\"width: 150px; font-size:16px; color:blue; \">";
  temp += "<br><br>IoTtalk Server URL <br>";  
  temp += "<input type=\"serverIP\" name=\"serverIP\" value=\"";
  temp += DefaultIoTtalkServerURL; 
  temp += "\" style=\"width: 150px; font-size:16px; color:blue;\" required>";
  temp += "<br><br><input style=\"-webkit-border-radius: 11; -moz-border-radius: 11";
  temp += "border-radius: 0px;";
  temp += "text-shadow: 1px 1px 3px #666666;";
  temp += "font-family: Arial;";
  temp += "color: #ffffff;";
  temp += "font-size: 18px;";
  temp += "background: #AAAAAA;";
  temp += "padding: 10px 20px 7px 20px;";
  temp += "text-decoration: none;\"" ;
  temp += "type=\"submit\" value=\"Submit\" on_click=\"javascript:alert('TEST');\"></center>";
  temp += "</div></form><br>";
  temp += "</body></html>";
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  server.send( 404, "text/html", "Page not found.");
}

void saveInfoAndConnectToWiFi() {
    Serial.println("Get network information.");
    char _SSID_[100]="";
    char _PASS_[100]="";
    
    if (server.arg(0) != "" && server.arg(2) != ""){//arg[0]-> SSID, arg[1]-> password (both string)
        server.arg(0).toCharArray(_SSID_,100);
        server.arg(1).toCharArray(_PASS_,100);
        String urlTemp = server.arg(2);
        urlTemp.replace("%3A", ":");
        urlTemp.replace("%3a", ":");
        urlTemp.replace("%2F", "/");
        urlTemp.replace("%2f", "/");
        urlTemp.replace("%20", " ");
        urlTemp.replace("+", " ");
        urlTemp.toCharArray(IoTtalkServerURL,100);
        server.send(200, "text/html", "<html><body><center><span style=\" font-size:72px; color:blue; margin:100px; \"> Setup successfully. </span></center></body></html>");
        server.stop();
        save_netInfo(_SSID_, _PASS_, IoTtalkServerURL);

        WiFi.softAPdisconnect(true);
        delay(100);
        connect_to_wifi(_SSID_, _PASS_);      
    }
    else {
        handleRoot(1);
    }
}


void start_web_server(void){
    server.on ( "/", [](){handleRoot(0);} );
    server.on ( "/setup", saveInfoAndConnectToWiFi);
    server.onNotFound ( handleNotFound );
    server.begin();   
}


void wifi_setting(void){
    // Scan Network first
    Serial.println("Scanning network.");
    ap_info = scan_network();

    Serial.println("Start AP.");
    String softapname = "MCU-";
    uint8_t MAC_array[6];
    WiFi.macAddress(MAC_array);
    for (int i=0;i<6;i++){
        if( MAC_array[i]<0x10 ) softapname+="0";
        softapname+= String(MAC_array[i],HEX);      //Append the mac address to url string
    }
 
    IPAddress ip(192,168,0,1);
    IPAddress gateway(192,168,0,1);
    IPAddress subnet(255,255,255,0);  
    // WiFi.mode(WIFI_AP_STA);
    // WiFi.disconnect();
    WiFi.softAPConfig(ip,gateway,subnet);
    WiFi.softAP(&softapname[0]);
    start_web_server();
    Serial.println ( "Switch to AP mode and start web server." );
}

uint8_t wifimode = 1; //1:AP , 0: STA 
void connect_to_wifi(char *wifiSSID, char *wifiPASS){
  long connecttimeout = millis();
  
  Serial.println("-----Connect to Wi-Fi-----");
  String fixWifiSSID = String(wifiSSID);
  fixWifiSSID.replace("+", " ");
  Serial.println("SSID : \"" + fixWifiSSID + "\"");
  WiFi.begin(fixWifiSSID.c_str(), wifiPASS);
  
  while (WiFi.status() != WL_CONNECTED && (millis() - connecttimeout < 10000) ) {
      delay(1000);
      Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println ( "Connected!\n");
    digitalWrite(ON_BOARD_LED_PIN, LOW);
    wifimode = 0;
  }
  else if (millis() - connecttimeout > 10000){
    Serial.println("Connect fail");
    wifi_setting();
  }

}



int iottalk_register(void){
    url = String(IoTtalkServerURL);  
    
    String df_list[] = DF_list;
    int n_of_DF = sizeof(df_list)/sizeof(df_list[0]); // the number of DFs in the DF_list
    String DFlist = ""; 
    for (int i=0; i<n_of_DF; i++){
        DFlist += "\"" + df_list[i] + "\"";  
        if (i<n_of_DF-1) DFlist += ",";
    }
  
    uint8_t MAC_array[6];
    WiFi.macAddress(MAC_array);//get esp12f mac address
    for (int i=0;i<6;i++){
        if( MAC_array[i]<0x10 ) url+="0";
        url+= String(MAC_array[i],HEX);      //Append the mac address to url string
    }
 
    //send the register packet
    Serial.println("[HTTP] POST..." + url);
    String profile="{\"profile\": {\"d_name\": \"";
    //profile += "MCU.";
    for (int i=3;i<6;i++){
        if( MAC_array[i]<0x10 ) profile+="0";
        profile += String(MAC_array[i],HEX);
    }
    profile += "\", \"dm_name\": \"";
    profile += DM_name;
    profile += "\", \"is_sim\": false, \"df_list\": [";
    profile +=  DFlist;
    profile += "]}}";

    http.begin(url);
    http.addHeader("Content-Type","application/json");
    int httpCode = http.POST(profile);

    Serial.println("[HTTP] Register... code: " + (String)httpCode );
    Serial.println(http.getString());
    //http.end();
    url +="/";  
    return httpCode;
}

String df_name_list[nODF];
String df_timestamp[nODF];
void init_ODFtimestamp(){
  for (int i=0; i<nODF; i++) df_timestamp[i] = "";
  for (int i=0; i<nODF; i++) df_name_list[i] = "";  
}

int DFindex(char *df_name){
    for (int i=0; i<nODF; i++){
        if (String(df_name) ==  df_name_list[i]) return i;
        else if (df_name_list[i] == ""){
            df_name_list[i] = String(df_name);
            return i;
        }
    }
    return nODF+1;  // df_timestamp is full
}

int push(char *df_name, String value){
    http.begin( url + String(df_name));
    http.addHeader("Content-Type","application/json");
    String data = "{\"data\":[" + value + "]}";
    int httpCode = http.PUT(data);
    if (httpCode != 200) Serial.println("[HTTP] PUSH \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    while (httpCode != 200){
        digitalWrite(ON_BOARD_LED_PIN, LOW);
        digitalWrite(ON_BOARD_LED_PIN, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200){
            http.PUT(data);
           // if (switchState) digitalWrite(4,HIGH);
        }
        else delay(3000);
    }
    http.end();
    return httpCode;
}

String pull(char *df_name){
    http.begin( url + String(df_name) );
    Serial.println(url + String(df_name));
    http.addHeader("Content-Type","application/json");
    int httpCode = http.GET(); //http state code
    
    if (httpCode != 200) Serial.println("[HTTP] "+url + String(df_name)+" PULL \"" + String(df_name) + "\"... code: " + (String)httpCode + ", retry to register.");
    while (httpCode != 200){
        digitalWrite(ON_BOARD_LED_PIN, LOW);
        digitalWrite(ON_BOARD_LED_PIN, HIGH);
        httpCode = iottalk_register();
        if (httpCode == 200){
            http.GET();
            //if (switchState) digitalWrite(4,HIGH);  
        }
        else delay(3000);
    }
    String get_ret_str = http.getString();  //After send GET request , store the return string
//    Serial.println
    
    Serial.println("output "+String(df_name)+": \n"+get_ret_str);
    http.end();

    get_ret_str = remove_ws(get_ret_str);
    // remove "endl"
    get_ret_str.replace("\r", "");
    get_ret_str.replace("\n", "");

    int string_index = 0;

    // skip string "samples"
    string_index = get_ret_str.indexOf("samples", string_index) + 1;
    string_index = get_ret_str.indexOf("[", string_index) + 1;
    // point to the first element of samples
    string_index = get_ret_str.indexOf("[", string_index);

    // No elements
    if(string_index == -1) {
        return "___NULL_DATA___";
    }

    // find the position of \" and \"
    int posStartQuotation, posEndQuotation;
    posStartQuotation = get_ret_str.indexOf("\"", string_index);
    posEndQuotation = get_ret_str.indexOf("\"", posStartQuotation+1);

    // extract string between \" and \"
    String timestamp = "";
    for(int i = posStartQuotation+1; i < posEndQuotation; i++) {
        timestamp += get_ret_str[i];
    }

    if (df_timestamp[DFindex(df_name)] != timestamp){
        // find the position of [ and ]
        int posStartBrackets, posEndBrackets;
        posStartBrackets = get_ret_str.indexOf("[", posEndQuotation);
        posEndBrackets = get_ret_str.indexOf("]", posStartBrackets+1);

        // extract string between [ and ]
        String data = "";
        for(int i = posStartBrackets+1; i < posEndBrackets; i++) {
        data += get_ret_str[i];
        }

        return data;
    } else {
        return "___NULL_DATA___";
    }
}

long sensorValue, suspend = 0;
long cycleTimestamp = millis();
void setup() {
    pinMode(ON_BOARD_LED_PIN, OUTPUT);// D4 : on board led
    digitalWrite(ON_BOARD_LED_PIN, HIGH);
    // pinMode(ON_BOARD_BTN_PIN, INPUT_PULLUP); // D3, GPIO0: clear eeprom button
    // clear eeprom button (use interrupt instead.)
    attachInterrupt(ON_BOARD_BTN_PIN, clr_eeprom, CHANGE); 

    // IDF
    pinMode(0, INPUT);
    pinMode(1, INPUT);
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(8, INPUT);
    pinMode(9, INPUT);

    // ODF
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(14, OUTPUT);
    pinMode(15, OUTPUT);
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);

//    EEPROM.begin(512);
    Serial.begin(115200);

    char wifissid[100]="";
    char wifipass[100]="";
    int statesCode = read_netInfo(wifissid, wifipass, IoTtalkServerURL);
    //for (int k=0; k<50; k++) Serial.printf("%c", EEPROM.read(k) );  //inspect EEPROM data for the debug purpose.
  
    if (!statesCode)  connect_to_wifi(wifissid, wifipass);
    else{
        Serial.println("Laod setting failed! statesCode: " + String(statesCode)); // StatesCode 1=No data, 2=ServerIP with wrong format
        wifi_setting();
    }

    while(wifimode){
        // server.handleClient(); //waitting for connecting to AP ;
        server.accept();
        // delay(10);
    }

    statesCode = 0;
    while (statesCode != 200) {
        statesCode = iottalk_register();
        if (statesCode != 200){
            Serial.println("Retry to register to the IoTtalk server. Suspend 3 seconds.");
            // if (digitalRead(ON_BOARD_BTN_PIN) == LOW) clr_eeprom();
            delay(3000);
        }
    }
    init_ODFtimestamp();
}

void pushIDF(char* idf_name, int pin, bool isDigital) {
    int val;
    if(isDigital) {
        val = digitalRead(pin);
    } else {
        val = analogRead(pin);
    }
    
    push(idf_name, String(val));
}

void pullODF(char* odf_name, int pin, bool isDigital) {
    String result = pull(odf_name);
    if (!result.equals("___NULL_DATA___")){
        Serial.println (String(odf_name)+": "+result);
        int parsed = result.toInt();
        if(isDigital) {
            if (parsed > 0 ) {
                digitalWrite(pin, HIGH);
            } else {
                digitalWrite(pin, LOW);
            }
        } else {
            if (parsed >= 0 && parsed <= 255) {
                analogWrite(pin, parsed);
            }
        }
    }
}

int pinA0; 
long LEDflashCycle = millis();
long LEDonCycle = millis();
int LEDhadFlashed = 0;
void loop() {
    // if (digitalRead(ON_BOARD_BTN_PIN) == LOW) clr_eeprom();

    if (millis() - cycleTimestamp > 200) {

        // P0 ~ P3 : digital input
        pushIDF("P0", 0, true);
        pushIDF("P1", 1, true);
        pushIDF("P2", 2, true);
        pushIDF("P3", 3, true);

        // P4, P5, P8, P9 : analog input
        pushIDF("P4~", 4, false);
        pushIDF("P5~", 5, false);
        pushIDF("P8~", 8, false);
        pushIDF("P9~", 9, false);

        // P10 ~ P13 : digital output
        pullODF("P10", 10, true);
        pullODF("P11", 11, true);
        pullODF("P12", 12, true);
        pullODF("P13", 13, true);

        // P14 ~ P17 : analog output
        pullODF("P14~", 14, false);
        pullODF("P15~", 15, false);
        pullODF("P16~", 16, false);
        pullODF("P17~", 17, false);

        cycleTimestamp = millis();
    }

    // LED blinking

    if (!LEDhadFlashed && millis() - LEDflashCycle > 2000){
        LEDhadFlashed = 1;
        digitalWrite(ON_BOARD_LED_PIN, HIGH);
        LEDonCycle = millis();
    }

    if (LEDhadFlashed && millis()-LEDonCycle > 100) {
        digitalWrite(ON_BOARD_LED_PIN, LOW);
        LEDhadFlashed = 0;
        LEDflashCycle = millis();
    }
}
