#ifndef __WATERING_H__
	#define __WATERING_H__

	// Log and web server filesystem definitions; don't hange
	#define FS_NONE 0
	#define FS_SD_CARD 1
	#define FS_SPIFFS 2

	#define FS_TYPE FS_SPIFFS

	// Comment to disable Serial debug prints
	#define WATERING_DEBUG

	// Comment to disable MQTT support
	//#define WATERING_MQTT

	// Comment to disable OTA support
	#define OTA_ENABLED

	// Comment to disable web browser update support
	//#define WEB_UPDATE_ENABLED


	// If uncommented, enables low power mode: gets MCU to sleep when detects a long period of waiting time.
	// Not compatible with FS_SD_CARD
	//#define WATERING_LOW_POWER_MODE

	// For different sensors, positive or neative ones
	// Out Of Water sensor
	//  - Negative sensor:
	#define OOW_READ_EXTRA_FUN(x) not(x)
	//  - Positive sensor:
	//#define OOW_READ_EXTRA_FUN(x) x

	// If uncommented, enables DHT functionality for that specific sensor. Uncomment only one.
	// Not compatible with FS_SD_CARD.
	// Remember DHT11 is 5V
	//#define WATERING_DHTTYPE DHT11
	//#define WATERING_DHTTYPE DHT21
	#define WATERING_DHTTYPE DHT22

	#ifdef WATERING_DHTTYPE
		// DHT data pin
		#define WATERING_DHT_PIN D6
		// DHT activation
		#define WATERING_DHT_ACTIVATE_PIN D7

		// status variables
		int dht_h = 0;
		int dht_t = 0;
	#endif

	#define SOIL_SENSOR_READ_PIN A0
	#define PUMP_ACTIVATE_PIN D1
	#define WATER_LEVEL_PIN D2

	#if FS_TYPE == FS_SPIFFS
		#define SOIL_SENSOR_ACTIVATE_PIN D5
	#else
		#define SOIL_SENSOR_ACTIVATE_PIN D0
	#endif


	/**
	 * Initial config, overwritten if CONFIG.TXT is found on FileSystem
	 */

	char wifi_mode = 'A'; // A-AP, S-Station (usual)

	// Behaviour configuration. Remember, sensor value is higher when dryer is soil.
	int soilSensorMinLevel = 700; // Below this level pump starts. Remember, 10-bit on ESP8266
	int soilSensorMaxLevel = 550; // Avobe this level pump stops. Remember, 10-bit on ESP8266

	// Timers adjustments
	#ifdef WATERING_DEBUG
		unsigned long int timeWarmingMilis = 15000; // ms warming sensor before read. TESTING, 15s
		unsigned long int timeReadMilisStandBy = 45000; // ms between sensor reads - Stand by. TESTING, 45s
		unsigned long int timeReadMilisWatering = 5000; //ms between sensor reads - watering, 5s
	#else
		unsigned long int timeWarmingMilis = 35000; // ms warming sensor before read. 35sec
		unsigned long int timeReadMilisStandBy = 600000; //ms between sensor reads - Stand by. 600s; 10 min
		unsigned long int timeReadMilisWatering = 5000; //ms between sensor reads - watering, 5s
	#endif


	char *reportingApiKey = NULL;
	char* ssid = NULL;
	char* password = NULL;
	char* mdnshostname = NULL;

	// Fixed IPs to speed-up connections
	IPAddress* mqttIp = NULL;
	IPAddress* apiIp = NULL;
	IPAddress* wifiIp = NULL;
	IPAddress* wifiNet = NULL;
	IPAddress* wifiGW = NULL;
	IPAddress* wifiDNS1 = NULL;
	IPAddress* wifiDNS2 = NULL;


	/** END Initial config **/



	#define SOIL_SENSOR_STATUS_OFF false
	#define SOIL_SENSOR_STATUS_ON true

	#define PUMP_STATUS_OFF false
	#define PUMP_STATUS_ON true

	#define OOW_SENSOR_STATUS_OFF false
	#define OOW_SENSOR_STATUS_ON true


	/********************************
	 * Function declarations
	 ******************************* */
	void parseConfigIpValue(IPAddress**, char *);
	void parseConfigString(char **, String *);
	void parseConfigLine(String);
	void doReport();
void doReportConfig();
	void report();
void returnFail(String);
	void handleStatus();
void handleGetConfig();
	void soilSensorProcessValue();
	void soilSensorActuations();
	void soilSensorChecks();
	void soilSensorLoop();
	void setupHW(void);
	void setupSD(void);
void handleSetRTC();
	void setRTC();
	void loadConfig();
	void handleFiles();
	void handleResetSD();
	void parseIPVaraible();
	void setupWiFi(void);
	void setupMandatoryInitialValues();
	void setup(void);
	void loop(void);
	bool saveConfig(void);
void handleSaveConfig(void);
	#ifdef WATERING_DEBUG
		void debugStatus();
	#endif
	#ifdef WATERING_LOW_POWER_MODE
		void goToDeepSleep();
	#endif
	#ifdef OTA_ENABLED
		void setupOTA(void);
	#endif




	/** Status global variables **/
	char lastReport[512]; // Take care, very long string
	unsigned long int lastReportTime = 0;
	unsigned long int lastSoilSensorActivationTime = 0;
	unsigned long int lastSoilSensorReadTime = 0;
	int lastSoilSensorRead = -1;
	bool pumpRunning = PUMP_STATUS_OFF;
	bool outOfWater = OOW_SENSOR_STATUS_ON;
	bool soilSensorStatus = SOIL_SENSOR_STATUS_ON;
	bool hasSD = false;
	unsigned long actMilis = 0;


	#ifdef WATERING_DEBUG
		#define W_DEBUG(a) Serial.print(a)
		#define W_DEBUGLN(a) Serial.println(a)
		#define W_DEBUG2(a, b) Serial.print(a, b)
		#define W_DEBUGLN2(a, b) Serial.println(a, b)
	#else
		#define W_DEBUG(a)
		#define W_DEBUGLN(a)
		#define W_DEBUG2(a, b)
		#define W_DEBUGLN2(a, b)
	#endif




	// ChipSelect form SD reader connected to SPI
	//#define WATERING_SD_CS 4


	//Usually not needed to change (RTC and EEPROM):
	//#define URTCLIB_ADDRESS 0x68
	//#define URTCLIB_EE_ADDRESS 0x57



	#ifdef ESP8266
#define EXTRA_YIELD() yield()
	#else
#define EXTRA_YIELD()
	#endif




	#if defined(WATERING_LOW_POWER_MODE) && FS_TYPE == FS_SD_CARD
		#error "Low power mode cannot be active at the same time as FS_SD_CARD"
	#endif

	#if defined(WATERING_DHTTYPE) && FS_TYPE == FS_SD_CARD
		#error "DHT functionality cannot be active at the same time as FS_SD_CARD"
	#endif

	// Done to support new report systems
#define DATA_VERSION 2
#define DATA_TYPE_LOG 1
#define DATA_TYPE_CONFIG 2

#endif

