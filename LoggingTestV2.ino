//V2 add time stamps
#include <elegantWebpage.h>
#include <AsyncElegantOTA.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <logger_spiffs.h>
#include "SPIFFS.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WebSerial.h>

const char* ssid       = "***";
const char* password   = "***";

int counter = 72; //Delete me

// Specify the path where the log is placed
LoggerSPIFFS sysLog("/log/syslog.txt");

AsyncWebServer  server(80);

//Structure time
/**
 * Input time in epoch format and return tm time format
 * by Renzo Mischianti <www.mischianti.org> 
 */
static tm getDateTimeByParams(long time){
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}

/**
 * Input tm time format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String getDateTimeStringByParams(tm *newtime, char* pattern = (char *)"%m/%d/%Y  %r"){
    char buffer[30];
    strftime(buffer, 30, pattern, newtime);
    return buffer;
}

 
/**
 * Input time in epoch format format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org> 
 */
static String getEpochStringByParams(long time, char* pattern = (char *)"%m/%d/%Y  %r"){
//    struct tm *newtime;
    tm newtime;
    newtime = getDateTimeByParams(time);
    return getDateTimeStringByParams(&newtime, pattern);
}
 

// Configure NTPClient
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
WiFiUDP ntpUDP;
int gtmOffset = 0; //Using GMT as Timezone.h will adjust accordingly
int ntpPollInterval = 5; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", gtmOffset*60*60, ntpPollInterval*60*1000); //Set to EST and update every 5 minutes

//Configure DST

// US Eastern Time Zone (New York)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);


void setup()
{
  Serial.begin(115200);

  
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

//Start logging
  while(!Serial);
  Serial.println();
  Serial.println("ESP Logger - Log on internal flash memory");

  sysLog.begin();
  sysLog.setFlusherCallback(senderHelp);

  Serial.println("Starting to log...");
  
//OTA support at /update
  AsyncElegantOTA.begin(&server); 

// WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);

//Configure web server
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/log/syslog.txt", "text/html");
  });


  
  server.begin();

//Start NTP client
  timeClient.begin();
  delay (5000);
  if (timeClient.update()){
     Serial.print ( "Adjust local clock" );
     WebSerial.print ( "Adjust local clock" );
     unsigned long epoch = timeClient.getEpochTime();
     // Update local clock
     setTime(epoch);
  }else{
     Serial.print ( "NTP update failed!!!" );
     WebSerial.print ( "NTP update failed!!!" );
  }


 sysLog.flush(); //clear the log on startup, remove this line to keep logs forever. 

}

void loop() {

  timeClient.update();


  delay(5000);
  Serial.println(getEpochStringByParams(usET.toLocal(now())) + "This is a log entry!<br>");
  WebSerial.println(getEpochStringByParams(usET.toLocal(now())) + " This has formatted time.");

  
   
   sysLog.append("This is a log entry!!!<br>");
  
  //sysLog.append(getEpochStringByParams(usET.toLocal(now())) + "This is a log entry!<br>"); //Write a log entry every 5 seconds

    
}

/**
 * Flush a chuck of logged records. To exemplify, the records are
 * flushed on Serial, but you are free to modify it to send data
 * over the network.
 */
bool senderHelp(char* buffer, int n){
  int index=0;
  // Check if there is another string to print
  while(index<n && strlen(&buffer[index])>0){
    Serial.print("---");
    int bytePrinted=Serial.print(&buffer[index]);
    Serial.println("---");
    // +1, the '\0' is processed
    index += bytePrinted+1;
  }
  return true;
}

/* Library sources 
https://github.com/fabianoriccardi/esp-logger
https://github.com/me-no-dev/ESPAsyncWebServer
https://github.com/ayushsharma82/AsyncElegantOTA
https://github.com/arduino-libraries/NTPClient
https://github.com/JChristensen/Timezone
 */
