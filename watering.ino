#include <SPI.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>

#include <Wire.h>
#include <SPI.h>



#include <uRTCLib.h>
#include "watering.h"

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


	if (variable.equals("soilSensorMinLevel")) {
		soilSensorMinLevel = value.toInt();
		W_DEBUG(F("CONFIG LINE: soilSensorMinLevel = "));
		W_DEBUGLN(soilSensorMinLevel);
	} else if (variable.equals("soilSensorMaxLevel")) {
		soilSensorMaxLevel = value.toInt();
		W_DEBUG(F("CONFIG LINE: soilSensorMaxLevel = "));
		W_DEBUGLN(soilSensorMaxLevel);
	} else if (variable.equals("timeReadMilisStandBy")) {
		timeReadMilisStandBy = value.toInt();
		W_DEBUG(F("CONFIG LINE: timeReadMilisStandBy = "));
		W_DEBUGLN(timeReadMilisStandBy);
	} else if (variable.equals("timeReadMilisWatering")) {
		timeReadMilisWatering = value.toInt();
		W_DEBUG(F("CONFIG LINE: timeReadMilisWatering = "));
		W_DEBUGLN(timeReadMilisWatering);
	} else if (variable.equals("timeWarmingMilis")) {
		timeWarmingMilis = value.toInt();
		W_DEBUG(F("CONFIG LINE: timeWarmingMilis = "));
		W_DEBUGLN(timeWarmingMilis);
	} else if (variable.equals("ApiKey")) {
		if (reportingApiKey) {
			free(reportingApiKey);
		}
		reportingApiKey = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(reportingApiKey, tmpbuffer);
		reportingApiKey[value.length()] = '\0';
		W_DEBUG(F("CONFIG LINE: reportingApiKey = "));
		W_DEBUGLN(reportingApiKey);
	} else if (variable.equals("ssid")) {
		if (ssid) {
			free(ssid);
		}
		ssid = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(ssid, tmpbuffer);
		ssid[value.length()] = '\0';
		W_DEBUG(F("CONFIG LINE: ssid = "));
		W_DEBUGLN(ssid);
	} else if (variable.equals("password")) {
		if (password) {
			free(password);
		}
		password = (char *) malloc(value.length() + 1);
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		strcpy(password, tmpbuffer);
		password[value.length()] = '\0';
		W_DEBUG(F("CONFIG LINE: password = "));
		W_DEBUGLN(password);
	} else if (variable.equals("wifi_mode")) {
		wifi_mode = value.charAt(0);
		W_DEBUG(F("CONFIG LINE: wifi_mode = "));
		W_DEBUGLN(wifi_mode);
	} else {
		W_DEBUG(F("CONFIG LINE UNKNOWN: "));
		W_DEBUGLN(line);
	}

}


void doReport() {
	rtc.refresh();
	EXTRA_YIELD();
	sprintf(lastReport, "{\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"soilSensorMinLevel\":\"%u\",\"soilSensorMaxLevel\":\"%u\",\"timeWarmingMilis\":\"%u\",\"timeReadMilisStandBy\":\"%u\",\"timeReadMilisWatering\":\"%u\",\"lastSoilSensorActivationTime\": \"%u\",\"lastSoilSensorReadTime\": \"%u\",\"lastSoilSensorRead\":\"%d\",\"pumpRunning\":\"%d\",\"outOfWater\":\"%d\",\"soilSensorStatus\":\"%d\",\"hasSD\":\"%d\",\"actMilis\":\"%d\"}", rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), soilSensorMinLevel, soilSensorMaxLevel, timeWarmingMilis, timeReadMilisStandBy, timeReadMilisWatering, lastSoilSensorActivationTime, lastSoilSensorReadTime, lastSoilSensorRead, pumpRunning, outOfWater, soilSensorStatus, hasSD, actMilis);
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

//		const char* fingerprint = "23 66 E2 D6 94 B5 DD 65 92 0A 4A 9B 34 70 7B B4 35 9B B6 C9";
		const char* host = "163.172.27.140";
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
    	W_DEBUGLN(F("Reporting done"));
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
	
	digitalWrite(SOIL_SENSOR_ACTIVATE_PIN, soilSensorStatus);
	digitalWrite(PUMP_ACTIVATE_PIN, pumpRunning);
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
	
	pinMode(WATER_LEVEL_PIN, INPUT);
	
	Serial.begin(115200);
	Wire.begin(0, 2); // D3 and D4
}





void setupSD(void){
	#ifdef FS_SD_CARD
		if (SD.begin(SS)){
			W_DEBUGLN(F("SD Card initialized."));
			hasSD = true;
		} else {
			hasSD = false;
		}
	#else
		#ifdef FS_SPIFFS
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


void printRTC() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	
	rtc.refresh();
	char content[120];
	sprintf(content, "{\"year\": %u,\"month\":%u,\"day\":%u,\"hour\":%u,\"minute\":%u,\"second\":%u,\"dow\":%u}", rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek());
	server.sendContent(content);
	
	W_DEBUG(F("RTC DateTime: "));
	W_DEBUG(rtc.year());
	W_DEBUG('/');
	W_DEBUG(rtc.month());
	W_DEBUG('/');
	W_DEBUG(rtc.day());
	W_DEBUG(' ');
	W_DEBUG(rtc.hour());
	W_DEBUG(':');
	W_DEBUG(rtc.minute());
	W_DEBUG(':');
	W_DEBUG(rtc.second());
	W_DEBUG(F(" DOW: "));
	W_DEBUGLN(rtc.dayOfWeek());
	EXTRA_YIELD();
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





#ifdef FS_SD_CARD
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
	
	void printDirectory() {
		if(!server.hasArg("dir")) {
			return returnFail(F("BAD ARGS"));
		}
		String path = server.arg("dir");
		if (path != "/" && !SD.exists((char *)path.c_str())) {
			return returnFail(F("BAD PATH"));
		}
		File dir = SD.open((char *)path.c_str());
		if (!dir.isDirectory()) {
			dir.close();
			return returnFail(F("NOT DIR"));
		}
		dir.rewindDirectory();
		server.setContentLength(CONTENT_LENGTH_UNKNOWN);
		server.send(200, "text/json", "");
	//	WiFiClient client = server.client();
		server.sendContent("[");
		for (int cnt = 0; true; ++cnt) {
			File entry = dir.openNextFile();
			if (!entry) {
				break;
			}
			String output;
			if (cnt > 0) {
				output = ',';
			}
			output += "{\"type\":\"";
			output += (entry.isDirectory()) ? "dir" : "file";
			output += "\",\"name\":\"";
			output += entry.name();
			output += "\"}";
			server.sendContent(output);
			entry.close();
			EXTRA_YIELD();
		}
		server.sendContent("]");
		dir.close();
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
			String message = "File Not Found\n\n";
			message += "URI: ";
			message += server.uri();
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
	#ifdef FS_SPIFFS
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
		void printDirectory() {
			if(!server.hasArg("dir")) {
				return returnFail("BAD ARGS");
			}
			String path = server.arg("dir");
			Dir dir = SPIFFS.openDir((char *)path.c_str());
			server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			server.send(200, "text/json", "");
			server.sendContent("[");
			Dir auxDir;
			String auxItemPath;
			bool firstFlag = true;
			while (dir.next()) {
				String output;
				if (firstFlag) {
					firstFlag = false;
				} else {
					output = ',';
				}
				output += "{\"type\":\"file\",\"name\":\"";
				output += dir.fileName();
				output += "\"";
				output += "}";
				server.sendContent(output);
				EXTRA_YIELD();
			}
			server.sendContent("]");
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
		void printNoFS() {
			returnFail(F("NO FS IN THIS VERSION"));
		}
		void printDirectory() {
			printNoFS();
		}
		void handleFiles() {
			printNoFS();
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
		WiFi.begin(ssid, password);
		// Wait for connection
		uint8_t i = 0;
		while (WiFi.status() != WL_CONNECTED && i++ < 30) {//wait 30 seconds
			delay(500);
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
 		WiFi.softAP(ssid, password);
		W_DEBUG("SoftAP created! IP address: ");
		W_DEBUGLN(WiFi.localIP());
	}

	server.stop();
	server.on("/api/resetSD", HTTP_GET, handleResetSD);
	server.on("/api/list", HTTP_GET, printDirectory);
	server.on("/api/rtc", HTTP_GET, printRTC);
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
