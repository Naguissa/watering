#include <SPI.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>

#include <Wire.h>
#include <SPI.h>



#include <uRTCLib.h>
#include "watering.h"


#ifdef WATERING_DHTTYPE
	#include <Adafruit_Sensor.h>
	#include <DHT.h>
	#include <DHT_U.h>

	DHT_Unified dht(WATERING_DHT_PIN, WATERING_DHTTYPE);
#endif

#ifdef WATERING_MQTT
	#include <PubSubClient.h>
	WiFiClient wifiClient;
	void callback(char* topic, byte* payload, unsigned int length) {
	  // Ignore incomming messages
	}
#endif

/**
 * See Watering.h for config and options
 */




ESP8266WebServer server(80);


uRTCLib rtc;

#if FS_TYPE == FS_SD_CARD
	#include <SD.h>
#elif FS_TYPE == FS_SPIFFS
		#include "FS.h"
#endif

void parseConfigIpValue(IPAddress**dest, char * value) {
	if (*dest) {
		free(*dest);
	}
	byte ipParts[4];
	char* part = strtok(value, ".");
	byte nparts = 0;
	while (part != 0) {
		ipParts[nparts] = (byte) atoi(part);
        ++nparts;
    	// Find the next part
    	part = strtok(0, ".");
    	// Prevent overflow
    	if (nparts > 3) {
    		break;
    	}
	}

	if (nparts != 4) {
		W_DEBUG(F("IP parse ERROR: "));
		W_DEBUGLN(value);
	} else {
		*dest = new IPAddress(ipParts[0], ipParts[1], ipParts[2], ipParts[3]);
		W_DEBUGLN(F("IP parse OK."));
	}		
	return;
}

void parseConfigLine(String line) {
	int pos;
	String variable, value;
	line.trim();
	W_DEBUG(F("CONFIG LINE START: "));
	W_DEBUGLN(line);
	if (line.startsWith(";") || line.startsWith("#") || line.startsWith("//")) { // comment line
		W_DEBUG(F("CONFIG LINE COMMENT: "));
		W_DEBUGLN(line);
		return;
	}
	pos = line.indexOf('=');
	if (pos < 2) { // Not found or too short for sure, skip
		W_DEBUG(F("CONFIG LINE TOO SHORT: "));
		W_DEBUGLN(line);
		return;
	}
	variable = line.substring(0,pos - 1);
	value = line.substring(pos + 1);
	variable.trim();
	value.trim();

	W_DEBUG(F("CONFIG LINE: "));
	W_DEBUG(variable);
	W_DEBUG(F(" =  "));
	W_DEBUGLN(value);
		
	if (variable.equals("soilSensorMinLevel")) {
		soilSensorMinLevel = value.toInt();
	} else if (variable.equals("soilSensorMaxLevel")) {
		soilSensorMaxLevel = value.toInt();
	} else if (variable.equals("timeReadMilisStandBy")) {
		timeReadMilisStandBy = value.toInt();
	} else if (variable.equals("timeReadMilisWatering")) {
		timeReadMilisWatering = value.toInt();
	} else if (variable.equals("timeWarmingMilis")) {
		timeWarmingMilis = value.toInt();
	} else if (variable.equals("ApiKey")) {
		if (reportingApiKey) {
			free(reportingApiKey);
		}
		reportingApiKey = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(reportingApiKey, tmpbuffer);
		reportingApiKey[value.length()] = '\0';
	} else if (variable.equals("ssid")) {
		if (ssid) {
			free(ssid);
		}
		ssid = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(ssid, tmpbuffer);
		ssid[value.length()] = '\0';
	} else if (variable.equals("password")) {
		if (password) {
			free(password);
		}
		password = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(password, tmpbuffer);
		password[value.length()] = '\0';
/* Disabled by now...
	} else if (variable.equals("mqttIp")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&mqttIp, tmpbuffer);
	} else if (variable.equals("apiIp")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&apiIp, tmpbuffer);
*/
	} else if (variable.equals("wifiIp")) {
		char tmpbuffer[200];
		parseConfigIpValue(&wifiIp, tmpbuffer);
	} else if (variable.equals("wifiNet")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&wifiNet, tmpbuffer);
	} else if (variable.equals("wifiGW")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&wifiGW, tmpbuffer);
	} else if (variable.equals("wifiDNS1")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&wifiDNS1, tmpbuffer);
	} else if (variable.equals("wifiDNS2")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&wifiDNS2, tmpbuffer);
	} else if (variable.equals("wifi_mode")) {
		wifi_mode = value.charAt(0);
	} else {
		W_DEBUG(F("CONFIG LINE UNKNOWN: "));
		W_DEBUGLN(line);
	}
}


void doReport() {
	rtc.refresh();
	EXTRA_YIELD();
	#ifdef WATERING_DHTTYPE
		sprintf(lastReport, "{\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"dow\":\"%u\",\"soilSensorMinLevel\":\"%u\",\"soilSensorMaxLevel\":\"%u\",\"lastSoilSensorActivationTime\": \"%u\",\"lastSoilSensorReadTime\": \"%u\",\"lastSoilSensorRead\":\"%d\",\"pumpRunning\":\"%d\",\"outOfWater\":\"%d\",\"soilSensorStatus\":\"%d\",\"hasSD\":\"%d\",\"temp\":\"%d\",\"hum\":\"%d\",\"actMilis\":\"%d\"}", rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek(), soilSensorMinLevel, soilSensorMaxLevel, lastSoilSensorActivationTime, lastSoilSensorReadTime, lastSoilSensorRead, pumpRunning, outOfWater, soilSensorStatus, hasSD, dht_t, dht_h, actMilis);
	#else
		sprintf(lastReport, "{\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"dow\":\"%u\",\"soilSensorMinLevel\":\"%u\",\"soilSensorMaxLevel\":\"%u\",\"lastSoilSensorActivationTime\": \"%u\",\"lastSoilSensorReadTime\": \"%u\",\"lastSoilSensorRead\":\"%d\",\"pumpRunning\":\"%d\",\"outOfWater\":\"%d\",\"soilSensorStatus\":\"%d\",\"hasSD\":\"%d\",\"actMilis\":\"%d\"}", rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek(), soilSensorMinLevel, soilSensorMaxLevel, lastSoilSensorActivationTime, lastSoilSensorReadTime, lastSoilSensorRead, pumpRunning, outOfWater, soilSensorStatus, hasSD, actMilis);
	#endif
}



void report() {
	doReport();
	EXTRA_YIELD();

/**
 * Report to SD/SPIFFS
    if (hasSD) {
	} else {
		W_DEBUGLN(F("Reporting to SD/SPIFFS failed due not connected"));
	}
 */


	if (reportingApiKey) {
	 // Use WiFiClient class to create TCP connections
//		WiFiClientSecure client;
		WiFiClient client;

		IPAddress host(163, 172, 27, 140);
// HTTPS, disabled for speed-up. No so private data....
//		const char* fingerprint = "23 66 E2 D6 94 B5 DD 65 92 0A 4A 9B 34 70 7B B4 35 9B B6 C9";
//		if (!client.connect(host, 443)) {
		if (!client.connect(host, 80)) {
	    	W_DEBUG(F("Reporting connection failed: "));
			W_DEBUG(host);
	    	W_DEBUG(":");
			W_DEBUGLN(443);
	    	return;
	  	}
		EXTRA_YIELD();

	  	// This will send the request to the server
	  	client.println(String(F("POST /arduino/api/")) + reportingApiKey + F("/store?_femode=2 HTTP/1.1"));
	  	client.print(F("Host: "));
	  	client.println(host);
		client.println(F("Cache-Control: no-cache"));
		client.println(F("Content-Type: application/x-www-form-urlencoded"));
		client.print(F("Content-Length: "));
		client.println(strlen(lastReport) + 5);
		client.println();
		client.print(F("data="));
		client.println(lastReport);
	  	client.println(F("Connection: close\r\n"));
	  	delay(10);
	
	  	// Read all the lines of the reply from server and print them to Serial
	  	while(client.available()){
	    	client.readStringUntil('\r');
			EXTRA_YIELD();
	  	}
    	W_DEBUGLN(F("API reporting done"));

		#ifdef WATERING_MQTT
		PubSubClient mqttclient(host, 1883, callback, wifiClient);
		if (mqttclient.connect(reportingApiKey)) {
    		if (mqttclient.publish(reportingApiKey, lastReport)) {
      			W_DEBUGLN("MQTT Report OK");
    		} else {
      			W_DEBUGLN("MQTT Report failed");
			}
		} else {
    		W_DEBUGLN("MQTT connection failed");
		}
		#endif

	  	
	} else {
		W_DEBUGLN(F("Reporting to server failed due lack of ApiKey"));
	}

	
	EXTRA_YIELD();
}





void handleStatus() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();
	doReport();
	server.sendContent(lastReport);
	W_DEBUGLN(F("Status call"));
	EXTRA_YIELD();
}



void soilSensorProcessValue() {
	if (lastSoilSensorRead > soilSensorMaxLevel) {
		pumpRunning = PUMP_STATUS_OFF;
	} else if (lastSoilSensorRead < soilSensorMinLevel) {
		pumpRunning = PUMP_STATUS_ON;
	}
}


void soilSensorChecks() {
	// START counter overflow control
	if (lastSoilSensorActivationTime > actMilis + 1000 || lastSoilSensorActivationTime == 0) { // overflow or 1st run; reset all timers and check inmediately
		W_DEBUGLN(F("Start or next read overflow"));
		lastSoilSensorActivationTime = 1; // To avoid entering always on same loop
		lastSoilSensorReadTime = 0;
		soilSensorStatus = SOIL_SENSOR_STATUS_ON;
		return;
	}
	// END overflow control & 1st run


	// Normal operation
	// Sensor is ON
	if (soilSensorStatus == SOIL_SENSOR_STATUS_ON) {
		
		// Warming, skip
		if (lastSoilSensorActivationTime + timeWarmingMilis > actMilis) {
			return;
		}
		
		unsigned long nextSoilSensorReadTime = lastSoilSensorReadTime + (pumpRunning == PUMP_STATUS_ON ? timeReadMilisWatering : timeReadMilisStandBy);

		// Next read overflow control
		if (nextSoilSensorReadTime < lastSoilSensorReadTime || nextSoilSensorReadTime + 12500 < actMilis) { // overflow control, add 12,5s comparing to actMilis because execution time & activation
			W_DEBUGLN(F("Next read overflow"));
			lastSoilSensorActivationTime = actMilis;
			lastSoilSensorReadTime = actMilis;
			return;
		}
		
		// Time to read!
		if (nextSoilSensorReadTime <= actMilis) {
			lastSoilSensorRead = analogRead(SOIL_SENSOR_READ_PIN);
			EXTRA_YIELD();
			lastSoilSensorReadTime = actMilis;
			// Recalculate next read to decide if it's appropiate to power off sensor
			W_DEBUGLN(F("Sensor read!"));
			EXTRA_YIELD();

			#ifdef WATERING_DHTTYPE
  				sensors_event_t event;  
  				dht.temperature().getEvent(&event);
				if (!isnan(event.temperature)) {
					dht_t = (int) (event.temperature * 100);
				}
				dht.humidity().getEvent(&event);
				if (!isnan(event.relative_humidity)) {
					dht_h = (int) (event.relative_humidity * 100);
				}
			#endif
			
			report();
			EXTRA_YIELD();
			return;
		}
		nextSoilSensorReadTime = lastSoilSensorReadTime + (lastSoilSensorRead > soilSensorMinLevel || pumpRunning == PUMP_STATUS_ON ? timeReadMilisWatering : timeReadMilisStandBy);
		if (nextSoilSensorReadTime > actMilis + timeWarmingMilis + 1000) { // Next read is after warming cycle; disable sensor
			W_DEBUGLN(F("Sensor timeout deactivation!"));
			soilSensorStatus = SOIL_SENSOR_STATUS_OFF;
		}
	// Sensor off; waiting for next test
	} else {
		unsigned long nextSoilSensorPowerTime = lastSoilSensorReadTime + (pumpRunning == PUMP_STATUS_ON ? timeReadMilisWatering : timeReadMilisStandBy) - timeWarmingMilis;
		if (nextSoilSensorPowerTime <= actMilis) { // Let's activate it!
			W_DEBUGLN(F("Sensor activation!"));
			lastSoilSensorActivationTime = actMilis;
			soilSensorStatus = SOIL_SENSOR_STATUS_ON;
			return;
		}
	}
	
}


void soilSensorActuations() {
	outOfWater = OOW_READ_EXTRA_FUN(digitalRead(WATER_LEVEL_PIN));
	if (lastSoilSensorRead == -1 || outOfWater) {
  		pumpRunning = PUMP_STATUS_OFF;
	} else {
		if (lastSoilSensorRead > soilSensorMinLevel) {
  			pumpRunning = PUMP_STATUS_ON;
		}
		if (lastSoilSensorRead < soilSensorMaxLevel) {
  			pumpRunning = PUMP_STATUS_OFF;
		}
	}
	
	digitalWrite(PUMP_ACTIVATE_PIN, pumpRunning);
	digitalWrite(SOIL_SENSOR_ACTIVATE_PIN, soilSensorStatus);
	#ifdef WATERING_DHTTYPE
		digitalWrite(WATERING_DHT_ACTIVATE_PIN, soilSensorStatus); // Follows same behaviour
	#endif

}



void soilSensorLoop() {
	soilSensorChecks();
	EXTRA_YIELD();
	soilSensorActuations();
	EXTRA_YIELD();
}



void setupHW(void){
	pinMode(SOIL_SENSOR_READ_PIN, INPUT);
	
	pinMode(SOIL_SENSOR_ACTIVATE_PIN, OUTPUT);
	digitalWrite(SOIL_SENSOR_ACTIVATE_PIN, 0);
	
	pinMode(PUMP_ACTIVATE_PIN, OUTPUT);
	digitalWrite(PUMP_ACTIVATE_PIN, 0);
	
	pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

	#ifdef WATERING_DHTTYPE
		pinMode(WATERING_DHT_ACTIVATE_PIN, OUTPUT);
	#endif
	
	Serial.begin(115200);
	Wire.begin(0, 2); // D3 and D4
}





void setupSD(void){
	#if FS_TYPE == FS_SD_CARD
		if (SD.begin(SS)){
			W_DEBUGLN(F("SD Card initialized."));
			hasSD = true;
		} else {
			hasSD = false;
		}
	#else
		#if FS_TYPE == FS_SPIFFS
			if (hasSD) {
				SPIFFS.end();
			}
		    hasSD = SPIFFS.begin();
		    if (!hasSD) {
		    	SPIFFS.format();
		    	hasSD = SPIFFS.begin();
		    }
		    if (hasSD) {
		     	W_DEBUGLN(F("SPIFFS initialized."));
		    }
		#else
		  	hasSD = false;
		#endif
	#endif
	EXTRA_YIELD();
}


void returnFail(String msg) {
	server.send(500, "text/plain", msg + "\r\n");
}


void setRTC() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();
	
	String year = server.arg("year");
	String month = server.arg("month");
	String day = server.arg("day");
	String hour = server.arg("hour");
	String minute = server.arg("minute");
	String second = server.arg("second");
	String dow = server.arg("dow");
	
	rtc.set((uint8_t) second.toInt(), (uint8_t) minute.toInt(), (uint8_t) hour.toInt(), (uint8_t) dow.toInt(), (uint8_t) day.toInt(), (uint8_t) month.toInt(), (uint8_t) year.toInt());
	W_DEBUG(F("RTC set: "));
	W_DEBUG(day.toInt());
	W_DEBUG("/");
	W_DEBUG(month.toInt());
	W_DEBUG("/");
	W_DEBUG(year.toInt());
	W_DEBUG(" (");
	W_DEBUG(dow.toInt());
	W_DEBUG(") ");
	W_DEBUG(hour.toInt());
	W_DEBUG(":");
	W_DEBUG(minute.toInt());
	W_DEBUG(":");
	W_DEBUGLN(second.toInt());
	
	server.sendContent(F("{\"setRTC\": \"true\"}"));
	EXTRA_YIELD();
}





#if FS_TYPE == FS_SD_CARD
	void loadConfig() {
		String line;
	 	File dataFile = SD.open(F("/CONFIG.TXT"), FILE_READ);
	 	int pos;
	  	// check for open error
		if (!dataFile) {
	 		W_DEBUGLN(F("Error opening SDCARD:/CONFIG.TXT in ReadOnly mode - Not available."));
	 		return;
		}
	  	if(dataFile.isDirectory()){
	     	dataFile.close();
	     	W_DEBUGLN(F("Error opening SDCARD:/CONFIG.TXT in ReadOnly mode - it's a folder"));
	 		return;
		}
		// read lines from the file
		while (dataFile.available()) {
			line = dataFile.readStringUntil('\n');
			parseConfigLine(line);
		}
	}
	
	
	void handleFiles() {
		String dataType = "text/plain";
		String path = server.uri();
	
	  	File dataFile = SD.open(path.c_str(), FILE_READ);
	  	if(dataFile.isDirectory()){
	 		dataFile.close();
			path += "/index.htm";
			dataFile = SD.open(path.c_str(), FILE_READ);
		}
	
		if (!dataFile) {
			String message = "<html><body><h1>File Not Found</h1>";
			message += "URI: ";
			message += server.uri();
			message += "<br>Method: ";
			message += (server.method() == HTTP_GET)?"GET":"POST";
			message += "<br>Arguments: ";
			message += server.args();
			message += "<br><p>Try <a href=\"/api/resetSD\">reset SD</a>, <a href=\"/api/status\">status</a> or <a href=\"/api/rtc\">rtc</a>.</p></body></html>";
			for (uint8_t i=0; i<server.args(); i++){
				message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
			}
			server.send(404, "text/html", message);
			return;
		}
	  
		String extension = path.substring(path.lastIndexOf("."));
		extension.toLowerCase();
		
		if(extension.equals(".htm")) dataType = "text/html";
		else if(extension.equals(".css")) dataType = "text/css";
		else if(extension.equals(".js")) dataType = "application/javascript";
		else if(extension.equals(".png")) dataType = "image/png";
		else if(extension.equals(".gif")) dataType = "image/gif";
		else if(extension.equals(".jpg")) dataType = "image/jpeg";
		else if(extension.equals(".ico")) dataType = "image/x-icon";
		else if(extension.equals(".xml")) dataType = "text/xml";
		else if(extension.equals(".pdf")) dataType = "application/pdf";
		else if(extension.equals(".zip")) dataType = "application/zip";
	
		W_DEBUG("URI: ");
		W_DEBUGLN(path);
		W_DEBUG("Extension: ");
		W_DEBUGLN(extension);
		W_DEBUGLN();
		if (server.hasArg("download")) {
			dataType = "application/octet-stream";
		}
		if (server.streamFile(dataFile, dataType) != dataFile.size()) {
	    	W_DEBUGLN("Sent less data than expected!");
	 	}
		dataFile.close();
	}
#else
	#if FS_TYPE == FS_SPIFFS
		void loadConfig() {
			String line;
		 	File dataFile = SPIFFS.open("/CONFIG.TXT", "r");
		 	int pos;
		  	// check for open error
			if (!dataFile) {
		 		W_DEBUGLN(F("Error opening SPIFFS:/CONFIG.TXT in ReadOnly mode - Not available."));
		 		return;
			}
		
			// read lines from the file
			while (dataFile.available()) {
				line = dataFile.readStringUntil('\n');
				parseConfigLine(line);
			}
			
		}

		void handleFiles() {
			String dataType = "text/plain";
			String path = server.uri();
		
			if (path.charAt(path.length() - 1) == '/') {
				path += "index.htm";
			}
		
		  	File dataFile = SPIFFS.open(path.c_str(), "r");
			if (!dataFile) {
				String message = "File Not Found\n\n";
				message += "URI: ";
				message += server.uri();
				message += "\nFull URI: ";
				message += path;
				message += "\nMethod: ";
				message += (server.method() == HTTP_GET)?"GET":"POST";
				message += "\nArguments: ";
				message += server.args();
				message += "\n";
				for (uint8_t i=0; i<server.args(); i++){
					message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
				}
				server.send(404, "text/plain", message);
				return;
			}
		  
			String extension = path.substring(path.lastIndexOf("."));
			extension.toLowerCase();
			
			if(extension.equals(".htm")) dataType = "text/html";
			else if(extension.equals(".css")) dataType = "text/css";
			else if(extension.equals(".js")) dataType = "application/javascript";
			else if(extension.equals(".png")) dataType = "image/png";
			else if(extension.equals(".gif")) dataType = "image/gif";
			else if(extension.equals(".jpg")) dataType = "image/jpeg";
			else if(extension.equals(".ico")) dataType = "image/x-icon";
			else if(extension.equals(".xml")) dataType = "text/xml";
			else if(extension.equals(".pdf")) dataType = "application/pdf";
			else if(extension.equals(".zip")) dataType = "application/zip";
		
			W_DEBUG("URI: ");
			W_DEBUGLN(path);
			W_DEBUG("Extension: ");
			W_DEBUGLN(extension);
			W_DEBUGLN();
			if (server.hasArg("download")) {
				dataType = "application/octet-stream";
			}
			if (server.streamFile(dataFile, dataType) != dataFile.size()) {
		    	W_DEBUGLN("Sent less data than expected!");
		 	}
			dataFile.close();
		}
	#else
		void loadConfig() {}
		void handleFiles() {
			returnFail(F("NO FS IN THIS VERSION"));
		}
	#endif
#endif



 


void handleResetSD() {
	setupSD();
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();
	server.sendContent(F("{resetSD: true}"));
}




void setupWiFi(void){
	if (wifi_mode == 'S') {
		WiFi.mode(WIFI_STA);
		
		if (wifiIp != NULL && wifiNet != NULL && wifiGW != NULL) { // Static IP
			Serial.print(F("Static IP"));
			if (wifiDNS1 != NULL) {
				if (wifiDNS2 != NULL) { // Static DNS
					WiFi.config(*wifiIp, *wifiGW, *wifiNet, *wifiDNS1);
				} else {
					WiFi.config(*wifiIp, *wifiGW, *wifiNet, *wifiDNS1, *wifiDNS2);
				}
			} else if (wifiDNS2 != NULL) {
				WiFi.config(*wifiIp, *wifiGW, *wifiNet, *wifiDNS2);
			} else {
				WiFi.config(*wifiIp, *wifiGW, *wifiNet);
			}
		}
		WiFi.begin(ssid, password);
		// Wait for connection
		uint8_t i = 0;
		while (WiFi.status() != WL_CONNECTED && i++ < 30) {//wait 30 seconds
			W_DEBUG('.');
			delay(1000);
		}
		if(i == 31){
			W_DEBUG("Could not connect to ");
			W_DEBUGLN(ssid);
			return;
		}
		W_DEBUG("Connected! IP address: ");
		W_DEBUGLN(WiFi.localIP());
		
	} else { // Default mode, 'A' (AP)
		WiFi.mode(WIFI_AP);
		
		if (wifiIp != NULL && wifiNet != NULL && wifiGW != NULL) { // Static IP with GW
			Serial.print(F("Static IP"));
			WiFi.softAPConfig(*wifiIp, *wifiGW, *wifiNet);
		} else if (wifiIp != NULL && wifiNet != NULL && wifiGW == NULL) { // Static IP without GW
			Serial.print(F("Static IP"));
			WiFi.softAPConfig(*wifiIp, *wifiIp, *wifiNet);
		}
		// Static DNS(s)
 		WiFi.softAP(ssid, password);
		W_DEBUG("SoftAP created! IP address: ");
		W_DEBUGLN(WiFi.localIP());
	}

	server.stop();
	server.on("/api/resetSD", HTTP_GET, handleResetSD);
	server.on("/api/rtc", HTTP_POST, setRTC);
	server.on("/api/status", HTTP_GET, handleStatus);
	
	server.onNotFound(handleFiles);
	
	server.begin();
	W_DEBUGLN("HTTP server started");
}




void setupMandatoryInitialValues() {
	// Initial values of SSID and pass
	ssid = (char *) malloc(15);
	strcpy(ssid, "WateringSystem");
	
	password = (char *) malloc(1);
	password[0] = '\0';	
}

void setup(void){
	delay(1500);
	setupMandatoryInitialValues();
	setupHW();
	setupSD();
    W_DEBUGLN(F("Hardware OK"));
    loadConfig();
    W_DEBUGLN(F("Config OK"));
    
	setupWiFi();
    W_DEBUGLN(F("Wifi OK"));
    W_DEBUGLN(F("-- Setup done --"));
}








#ifdef WATERING_DEBUG
void debugStatus() {
	doReport();
	W_DEBUG(millis());
	W_DEBUG(" -- ");
	W_DEBUGLN(lastReport);
}

unsigned long int tick = 0;
#endif

 
void loop(void){
	actMilis = millis();
	soilSensorLoop();
	EXTRA_YIELD();
	server.handleClient();
	EXTRA_YIELD();

#ifdef WATERING_DEBUG
	tick++;
	if (tick >= 50000) {
		tick = 0;
		debugStatus();
	}
#endif
}


#ifdef WATERING_LOW_POWER_MODE
	void goToDeepSleep() {



		
	}
#endif

