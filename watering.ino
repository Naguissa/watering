/**
 * See Watering.h for config and options
 */

// Choose IDE, because we need different hacks depending IDE
#define IDE_ARDUINO 1
#define IDE_ECLIPSE 2
#define IDE_USED IDE_ECLIPSE

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "watering.h"

// Move here to avoid problems on Eclipse
#if FS_TYPE == FS_SD_CARD
	#include <SD.h>
#elif FS_TYPE == FS_SPIFFS
#if IDE_USED == IDE_ECLIPSE
#define FS_NO_GLOBALS	// Avoid File compile error on Eclipse
	#include "FS.h"
extern fs::FS SPIFFS;
#else
#include "FS.h"
#endif
#endif



#ifdef WATERING_DEBUG
	#include <GDBStub.h>
#endif
#ifdef OTA_ENABLED
	#include <WiFiUdp.h>
	#include <ArduinoOTA.h>
#endif
#ifdef WEB_UPDATE_ENABLED
	#include <ESP8266HTTPUpdateServer.h>
#endif
#include <Wire.h>
#include <SPI.h>

#include <uRTCLib.h>


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





ESP8266WebServer server(80);

#ifdef WEB_UPDATE_ENABLED
	ESP8266HTTPUpdateServer httpUpdater;
#endif



uRTCLib rtc;

void parseConfigIpValue(IPAddress**dest, char * value) {
	if (*dest) {
		free(*dest);
	}
	byte ipParts[4];
	char* part = strtok(value, ".");
	byte nparts = 0;
	while (part != 0 && nparts <= 3) {
		ipParts[nparts] = (byte) atoi(part);
        ++nparts;
    	// Find the next part
    	part = strtok(0, ".");
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

void parseConfigString(char ** dest, String * value) {
		if (*dest) {
			free(*dest);
		}
		*dest = (char *) malloc(value->length() + 1);
		char tmpbuffer[200];
		value->toCharArray(tmpbuffer, 200);
		strcpy(*dest, tmpbuffer);
		*(dest + value->length()) = '\0';
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
		parseConfigString(&reportingApiKey, &value);
	} else if (variable.equals("ssid")) {
		parseConfigString(&ssid, &value);
	} else if (variable.equals("password")) {
		parseConfigString(&password, &value);
	} else if (variable.equals("hostname")) {
		parseConfigString(&mdnshostname, &value);
	} else if (variable.equals("mqttIp")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&mqttIp, tmpbuffer);
	} else if (variable.equals("apiIp")) {
		char tmpbuffer[200];
		value.toCharArray(tmpbuffer, 200);
		parseConfigIpValue(&apiIp, tmpbuffer);
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
	sprintf(lastReport, "{\"v\":\"%ud\",\"t\":\"%ud\",\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"dow\":\"%u\",\"d\":[{\"lastSoilSensorActivationTime\": \"%lu\",\"lastSoilSensorReadTime\": \"%lu\",\"lastSoilSensorRead\":\"%d\",\"pumpRunning\":\"%d\",\"outOfWater\":\"%d\",\"soilSensorStatus\":\"%d\"}],\"hasSD\":\"%d\",\"temp\":\"%d\",\"hum\":\"%d\",\"actMilis\":\"%lu\"} ", DATA_VERSION, DATA_TYPE_LOG, rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek(), lastSoilSensorActivationTime, lastSoilSensorReadTime, lastSoilSensorRead, pumpRunning, outOfWater, soilSensorStatus, hasSD, dht_t, dht_h, actMilis);
	#else
	sprintf(lastReport, "{\"v\":\"%ud\",\"t\":\"%ud\",\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"dow\":\"%u\",\"d\":[{\"lastSoilSensorActivationTime\": \"%lu\",\"lastSoilSensorReadTime\": \"%lu\",\"lastSoilSensorRead\":\"%ld\",\"pumpRunning\":\"%d\",\"outOfWater\":\"%d\",\"soilSensorStatus\":\"%d\"}],\"hasSD\":\"%d\",\"actMilis\":\"%lu\"}", DATA_VERSION, DATA_TYPE_LOG, rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek(), lastSoilSensorActivationTime, lastSoilSensorReadTime, lastSoilSensorRead, pumpRunning, outOfWater, soilSensorStatus, hasSD, actMilis);
	#endif
		EXTRA_YIELD();
}


void doReportConfig() {
	rtc.refresh();
	String tmp;
	char buff[16];
	EXTRA_YIELD();
	sprintf(lastReport, "{\"v\":\"%ud\",\"t\":\"%ud\",\"date\":\"%02u-%02u-%02u %02u:%02u:%02u\",\"dow\":\"%u\",\"soilSensorMinLevel\":\"%u\",\"soilSensorMaxLevel\":\"%u\",\"timeReadMilisStandBy\":\"%lu\",\"timeReadMilisWatering\":\"%lu\",\"timeWarmingMilis\":\"%lu\"}", DATA_VERSION, DATA_TYPE_CONFIG, rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second(), rtc.dayOfWeek(), soilSensorMinLevel, soilSensorMaxLevel, timeReadMilisStandBy, timeReadMilisWatering, timeWarmingMilis);
	EXTRA_YIELD();

	strcat(lastReport, ",\"mqttIp\":");
	if (mqttIp == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = mqttIp->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"apiIp\":");
	if (apiIp == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = apiIp->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"wifiIp\":");
	if (wifiIp == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = wifiIp->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"wifiNet\":");
	if (wifiNet == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = wifiNet->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"wifiGW\":");
	if (wifiGW == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = wifiGW->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"wifiDNS1\":");
	if (wifiDNS1 == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = wifiDNS1->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, ",\"wifiDNS2\":");
	if (wifiDNS2 == NULL) {
		strcat(lastReport, "null");
	} else {
		tmp = wifiDNS2->toString();
		tmp.toCharArray(buff, 16);
		strcat(lastReport, "\"");
		strcat(lastReport, buff);
		strcat(lastReport, "\"");
	}
	EXTRA_YIELD();

	strcat(lastReport, "}]}");
	EXTRA_YIELD();
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
	  	client.println(String("POST /arduino/api/") + reportingApiKey + "/store?_femode=2 HTTP/1.1");
	  	client.print("Host: ");
	  	client.println(host);
		client.println("Cache-Control: no-cache");
		client.println("Content-Type: application/x-www-form-urlencoded");
		client.print("Content-Length: ");
		client.println(strlen(lastReport) + 5);
		client.println();
		client.print("data=");
		client.println(lastReport);
	  	client.println("Connection: close\r\n");
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
	W_DEBUGLN("Status call");
	EXTRA_YIELD();
}

void handleGetConfig() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	EXTRA_YIELD();
	server.send(200, "text/json", "");
	EXTRA_YIELD();
	WiFiClient client = server.client();
	doReportConfig();
	server.sendContent(lastReport);
	EXTRA_YIELD();
	W_DEBUGLN("Status call");
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

		unsigned long nextSoilSensorReadTime = lastSoilSensorReadTime + (lastSoilSensorRead == -1 || lastSoilSensorRead < soilSensorMinLevel || pumpRunning == PUMP_STATUS_ON ? timeReadMilisWatering : timeReadMilisStandBy);

		// Next read overflow control
		if (nextSoilSensorReadTime < lastSoilSensorReadTime || nextSoilSensorReadTime + 30000 < actMilis) { // overflow control, add 30s comparing because execution time & activation
			W_DEBUG(F("Next read overflow: "));
			W_DEBUGLN(nextSoilSensorReadTime);
			lastSoilSensorActivationTime = (actMilis - timeWarmingMilis);
			if (lastSoilSensorActivationTime > actMilis) {
				lastSoilSensorActivationTime = 0;
			}
			lastSoilSensorReadTime = actMilis;
			return;
		}

		// Time to read!
		if (nextSoilSensorReadTime <= actMilis) {
			lastSoilSensorRead = analogRead(SOIL_SENSOR_READ_PIN);
			EXTRA_YIELD();
			lastSoilSensorReadTime = actMilis;
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
	String year = server.arg("year");
	String month = server.arg("month");
	String day = server.arg("day");
	EXTRA_YIELD();

	String hour = server.arg("hour");
	String minute = server.arg("minute");
	String second = server.arg("second");
	EXTRA_YIELD();

	String dow = server.arg("dow");
	EXTRA_YIELD();

	rtc.set((uint8_t) second.toInt(), (uint8_t) minute.toInt(), (uint8_t) hour.toInt(), (uint8_t) dow.toInt(), (uint8_t) day.toInt(), (uint8_t) month.toInt(), (uint8_t) year.toInt());
	EXTRA_YIELD();

	W_DEBUG(F("RTC set: "));
	W_DEBUG(day.toInt());
	W_DEBUG("/");
	W_DEBUG(month.toInt());
	W_DEBUG("/");
	W_DEBUG(year.toInt());
	EXTRA_YIELD();

	W_DEBUG(" (");
	W_DEBUG(dow.toInt());
	W_DEBUG(") ");
	EXTRA_YIELD();

	W_DEBUG(hour.toInt());
	W_DEBUG(":");
	W_DEBUG(minute.toInt());
	W_DEBUG(":");
	W_DEBUGLN(second.toInt());
	EXTRA_YIELD();
}

void handleSetRTC() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();

	setRTC();

	server.sendContent(F("{\"setRTC\": true}"));
	EXTRA_YIELD();
}





#if FS_TYPE == FS_SD_CARD
	void loadConfig() {
		String line;
	 	File dataFile = SD.open(F("/CONFIG.TXT"), FILE_READ);
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


	bool saveConfig() {
		FILE configFile = SD.open("/CONFIG.TXT", FILE_WRITE);
		if (configFile) {
			configFile.println(F("# syntax:"));
			configFile.println(F("# variable = value"));
			configFile.println(F("# (spaces doesn't care)"));
			configFile.println(F("# Valid variables"));
			configFile.println(F("#  wifi_mode - (A-AP, S -or other- - Station"));
			configFile.println(F("#  ssid - Network SSID "));
			configFile.println(F("#  password - Network password (not set for none)"));
			configFile.println(F("#  soilSensorMinLevel - Level when pump STOPS working (remember, soil sensor returns 0 when fully wet)"));
			configFile.println(F("#  soilSensorMaxLevel - Level when pump STARTS working (remember, soil sensor returns 0 when fully wet)"));
			configFile.println(F("#  timeReadMilisStandBy - Time between sensor reads when pump is NOT active"));
			configFile.println(F("#  timeReadMilisWatering - Time between sensor reads when pump IS active"));
			configFile.println(F("#  timeReportMilis - ms between reports to server. Also, ms between two reads"));
			configFile.println(F("#  timeWarmingMilis - ms warming sensor before read. Sensor is stopped when pump is not working and is powered each timeReportMilis"));
			configFile.println(F("#  ApiKey - Get one at http://www.foroelectro.net/arduino"));
			configFile.println(F("# Comments: lines starting with #, ; or //"));
			configFile.print(F("wifi_mode = "));
			configFile.println(wifi_mode);
			if (ssid != NULL && ssid && strlen(ssid)) {
				configFile.print(F("ssid = "));
				configFile.println(ssid);
			} else {
				configFile.println(F(";ssid = <Your WiFi SSID>"));
			}
			if (password != NULL && password && strlen(password)) {
				configFile.print(F("password = "));
				configFile.println(password);
			} else {
				configFile.println(F(";ssid = <Your WiFi password>"));
			}
			configFile.print(F("soilSensorMinLevel = "));
			configFile.println(soilSensorMinLevel);
			configFile.print(F("soilSensorMaxLevel = "));
			configFile.println(soilSensorMaxLevel);
			configFile.print(F("timeReadMilisStandBy = "));
			configFile.println(timeReadMilisStandBy);
			configFile.print(F("timeReadMilisWatering = "));
			configFile.println(timeReadMilisWatering);
			configFile.print(F("timeWarmingMilis = "));
			configFile.println(timeWarmingMilis);
			if (reportingApiKey != NULL && reportingApiKey && strlen(reportingApiKey)) {
				configFile.print(F("ApiKey = "));
				configFile.println(reportingApiKey);
			} else {
				configFile.println(F(";ApiKey = Get one at https://www.foroelectro.net/arduino/"));
			}
//			if (mqttIp != NULL && mqttIp && strlen(mqttIp)) {
//				configFile.print(F("mqttIp = "));
//				configFile.println(mqttIp);
//			} else {
//				configFile.println(F(";mqttIp = <ip from mqtt server ip; check https://www.foroelectro.net/arduino/en/mqtt-doc>"));
//			}
//			if (apiIp != NULL && apiIp && strlen(apiIp)) {
//				configFile.print(F("apiIp = "));
//				configFile.println(apiIp);
//			} else {
//				configFile.println(F(";apiIp = <ip from api server ip; check https://www.foroelectro.net/arduino>"));
//			}
/* ToDo: IP to STR

			if (wifiIp) {
				configFile.print(F("wifiIp = "));
				configFile.println(wifiIp);
			} else {
				configFile.println(F(";wifiIp = <local static IP>"));
			}
			if (wifiNet) {
				configFile.print(F("wifiNet = "));
				configFile.println(wifiNet);
			} else {
				configFile.println(F(";wifiNet = <local static NetMask>"));
			}
			if (wifiGW) {
				configFile.print(F("wifiGW = "));
				configFile.println(wifiGW);
			} else {
				configFile.println(F(";wifiGW = <local static Gateway>"));
			}
			if (wifiDNS1) {
				configFile.print(F("wifiDNS1 = "));
				configFile.println(wifiDNS1);
			} else {
				configFile.println(F(";wifiDNS1 = <Static DNS 1>"));
			}
			if (wifiDNS2) {
				configFile.print(F("wifiDNS2 = "));
				configFile.println(wifiDNS2);
			} else {
				configFile.println(F(";wifiDNS2 = <Static DNS 2>"));
			}
*/
				configFile.println(F(";wifiIp = <local static IP>"));
				configFile.println(F(";wifiNet = <local static NetMask>"));
				configFile.println(F(";wifiDNS1 = <Static DNS 1>"));
				configFile.println(F(";wifiDNS2 = <Static DNS 2>"));
			configFile.close();
	    	return true;
		} else {
			// if the file didn't open, print an error:
	    	W_DEBUGLN("ERROR saving CONFIG.TXT");
	    	return false;
		}
	}


#else
	#if FS_TYPE == FS_SPIFFS
		void loadConfig() {
			String line;
	fs::File dataFile = SPIFFS.open("/CONFIG.TXT", "r");
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

	fs::File dataFile = SPIFFS.open(path.c_str(), "r");
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


		bool saveConfig() {
	fs::File configFile = SPIFFS.open("/CONFIG.TXT", "w");
			if (configFile) {
				configFile.println(F("# syntax:"));
		EXTRA_YIELD();
		configFile.println(F("# variable = value"));
		EXTRA_YIELD();
		configFile.println(F("# (spaces doesn't care)"));
		EXTRA_YIELD();
		configFile.println(F("# Comments: lines starting with #, ; or //"));
		EXTRA_YIELD();

		configFile.print(F("wifi_mode = "));
		configFile.println(wifi_mode);
		if (ssid != NULL && ssid && strlen(ssid)) {
			configFile.print(F("ssid = "));
			configFile.println(ssid);
		} else {
			configFile.println(F(";ssid = <Your WiFi SSID>"));
		}
		EXTRA_YIELD();

		if (password != NULL && password && strlen(password)) {
			configFile.print(F("password = "));
			configFile.println(password);
		} else {
			configFile.println(F(";ssid = <Your WiFi password>"));
		}
		EXTRA_YIELD();

		configFile.print(F("soilSensorMinLevel = "));
		configFile.println(soilSensorMinLevel);
		EXTRA_YIELD();

		configFile.print(F("soilSensorMaxLevel = "));
		configFile.println(soilSensorMaxLevel);
		EXTRA_YIELD();

		configFile.print(F("timeReadMilisStandBy = "));
		configFile.println(timeReadMilisStandBy);
		EXTRA_YIELD();

		configFile.print(F("timeReadMilisWatering = "));
		configFile.println(timeReadMilisWatering);
		EXTRA_YIELD();

		configFile.print(F("timeWarmingMilis = "));
		configFile.println(timeWarmingMilis);
		EXTRA_YIELD();

		if (reportingApiKey != NULL && reportingApiKey && strlen(reportingApiKey)) {
			configFile.print(F("ApiKey = "));
			configFile.println(reportingApiKey);
		} else {
			configFile.println(F(";ApiKey = Get one at https://www.foroelectro.net/arduino/"));
		}
		EXTRA_YIELD();

		if (mqttIp != NULL && mqttIp) {
			configFile.print(F("mqttIp = "));
			configFile.println(mqttIp->toString());
		} else {
			configFile.println(F(";mqttIp = <ip from mqtt server ip; check https://www.foroelectro.net/arduino/en/mqtt-doc>"));
		}
		EXTRA_YIELD();

		if (apiIp != NULL && apiIp) {
			configFile.print(F("apiIp = "));
			configFile.println(apiIp->toString());
				} else {
			configFile.println(F(";apiIp = <ip from api server ip; check https://www.foroelectro.net/arduino>"));
				}
		EXTRA_YIELD();

		if (wifiIp != NULL && wifiIp) {
			configFile.print(F("wifiIp = "));
			configFile.println(wifiIp->toString());
				} else {
			configFile.println(F(";wifiIp = <local static IP>"));
				}
		EXTRA_YIELD();

		if (wifiNet != NULL && wifiNet) {
			configFile.print(F("wifiNet = "));
			configFile.println(wifiNet->toString());
				} else {
			configFile.println(F(";wifiNet = <local static NetMask>"));
				}
		EXTRA_YIELD();

		if (wifiGW != NULL && wifiGW) {
					configFile.print(F("wifiGW = "));
			configFile.println(wifiGW->toString());
		} else {
			configFile.println(F(";wifiGW = <local static Gateway>"));
				}
		EXTRA_YIELD();

		if (wifiDNS1 != NULL && wifiDNS1) {
					configFile.print(F("wifiDNS1 = "));
			configFile.println(wifiDNS1->toString());
		} else {
			configFile.println(F(";wifiDNS1 = <Static DNS 1>"));
				}
		EXTRA_YIELD();

		if (wifiDNS2 != NULL && wifiDNS2) {
					configFile.print(F("wifiDNS2 = "));
			configFile.println(wifiDNS2->toString());
		} else {
			configFile.println(F(";wifiDNS2 = <Static DNS 2>"));
				}
		EXTRA_YIELD();

				configFile.close();
		return true;
			} else {
				// if the file didn't open, print an error:
		W_DEBUGLN("ERROR saving CONFIG.TXT");
		return false;
			}
		}
	#else
		void loadConfig() {}
		void handleFiles() { returnFail(F("NO FS IN THIS VERSION")); }
		bool saveConfig() { return false; }
	#endif
#endif






void handleResetSD() {
	setupSD();
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();
	server.sendContent(F("{\"resetSD\": true}"));
}

void handleSaveConfig() {
	String value;
	char tmpbuffer[20];

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/json", "");
	WiFiClient client = server.client();

	EXTRA_YIELD();

	value = server.arg("soilSensorMinLevel");
	value.trim();
	if (value != "") {
		soilSensorMinLevel = value.toInt();
	}
	EXTRA_YIELD();

	value = server.arg("soilSensorMaxLevel");
	value.trim();
	if (value != "") {
		soilSensorMaxLevel = value.toInt();
	}
	EXTRA_YIELD();

	value = server.arg("timeReadMilisStandBy");
	value.trim();
	if (value != "") {
		timeReadMilisStandBy = value.toInt();
	}
	EXTRA_YIELD();

	value = server.arg("timeReadMilisWatering");
	value.trim();
	if (value != "") {
		timeReadMilisWatering = value.toInt();
	}
	EXTRA_YIELD();

	value = server.arg("timeWarmingMilis");
	value.trim();
	if (value != "") {
		timeWarmingMilis = value.toInt();
	}
	EXTRA_YIELD();

	value = server.arg("mqttIp");
	value.trim();
	if (value == "") {
		if (mqttIp != NULL) {
			free(mqttIp);
			mqttIp = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&mqttIp, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("apiIp");
	value.trim();
	if (value == "") {
		if (apiIp != NULL) {
			free(apiIp);
			apiIp = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&apiIp, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("wifiIp");
	value.trim();
	if (value == "") {
		if (wifiIp != NULL) {
			free(wifiIp);
			wifiIp = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&wifiIp, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("wifiNet");
	value.trim();
	if (value == "") {
		if (wifiNet != NULL) {
			free(wifiNet);
			wifiNet = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&wifiNet, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("wifiGW");
	value.trim();
	if (value == "") {
		if (wifiGW != NULL) {
			free(wifiGW);
			wifiGW = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&wifiGW, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("wifiDNS1");
	value.trim();
	if (value == "") {
		if (wifiDNS1 != NULL) {
			free(wifiDNS1);
			wifiDNS1 = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&wifiDNS1, tmpbuffer);
	}
	EXTRA_YIELD();

	value = server.arg("wifiDNS2");
	value.trim();
	if (value == "") {
		if (wifiDNS2 != NULL) {
			free(wifiDNS2);
			wifiDNS2 = NULL;
		}
	} else {
		value.toCharArray(tmpbuffer, 20);
		parseConfigIpValue(&wifiDNS2, tmpbuffer);
	}
	EXTRA_YIELD();

	saveConfig();

	server.sendContent(F("{\"save\": "));
	server.sendContent(saveConfig() ? F("true") : F("false"));
	server.sendContent(F("}"));
}




void setupWiFi(void){
	server.stop();
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

	if (wifi_mode != 'S' || WiFi.status() == WL_CONNECTED) {
		MDNS.begin(mdnshostname);
		server.on("/api/resetSD", HTTP_GET, handleResetSD);
		server.on("/api/rtc", HTTP_POST, handleSetRTC);
		server.on("/api/status", HTTP_GET, handleStatus);
		server.on("/api/config", HTTP_GET, handleGetConfig);
		server.on("/api/config", HTTP_POST, handleSaveConfig);
		#ifdef WEB_UPDATE_ENABLED
			httpUpdater.setup(&server);
		#endif
  		MDNS.addService("http", "tcp", 80);
		server.onNotFound(handleFiles);
		server.begin();
		W_DEBUGLN(F("HTTP and MDNS server started"));
	}

}




void setupMandatoryInitialValues() {
	// Initial values of SSID and pass
	ssid = (char *) malloc(15);
	strcpy(ssid, "WateringSystem");

	mdnshostname = (char *) malloc(15);
	strcpy(mdnshostname, "WateringSystem");

	password = (char *) malloc(1);
	password[0] = '\0';
}




#ifdef OTA_ENABLED
	void setupOTA(void){
		// Port defaults to 8266
		// ArduinoOTA.setPort(8266);
		ArduinoOTA.setHostname("Watering");
		// No authentication by default
		// ArduinoOTA.setPassword((const char *)"123");
		#ifdef WATERING_DEBUG
			ArduinoOTA.onStart([]() {
	    		Serial.println("Start");
	  		});
			ArduinoOTA.onEnd([]() {
				Serial.println("\nEnd");
			});
			ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
				Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			});
			ArduinoOTA.onError([](ota_error_t error) {
				W_DEBUG("Error[");
				W_DEBUG(error);
				W_DEBUG("]: ");
				switch (error) {
					case OTA_AUTH_ERROR:
						W_DEBUGLN("Auth Failed");
						break;
					case OTA_BEGIN_ERROR:
						W_DEBUGLN("Begin Failed");
						break;
					case OTA_CONNECT_ERROR:
						W_DEBUGLN("Connect Failed");
						break;
					case OTA_RECEIVE_ERROR:
						W_DEBUGLN("Receive Failed");
						break;
					case OTA_END_ERROR:
						W_DEBUGLN("End Failed");
						break;
					default:
						W_DEBUGLN("Unknown error");
				}
			});
		#endif
		ArduinoOTA.begin();
	}
#endif


void setup(void) {
	setupOTA();
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
	#ifdef OTA_ENABLED
		ArduinoOTA.handle();
		EXTRA_YIELD();
	#endif
	soilSensorLoop();
	EXTRA_YIELD();
	server.handleClient();
	EXTRA_YIELD();
	if (wifi_mode == 'S' && WiFi.status() != WL_CONNECTED) {
		setupWiFi();
	}


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

