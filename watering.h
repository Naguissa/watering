// Log and web server filesystem:
#define FS_NONE 0
#define FS_SD_CARD 1
#define FS_SPIFFS 2

#define FS_TYPE FS_SD_CARD

// Comment to disable Serial debug prints
#define WATERING_DEBUG


// For different sensors, positive or neative ones
// Out Of Water sensor
//  - Negative sensor:
#define OOW_READ_EXTRA_FUN(x) not(x)
//  - Positive sensor: 
//#define OOW_READ_EXTRA_FUN(x) x



/**
 * Initial config, overwritten if CONFIG.TXT is found on FileSystem
 */

char wifi_mode = 'A'; // A-AP, S-Station (usual)

// Behaviour configuration. Remember, sensor value is higher when dryer is soil.
int soilSensorMinLevel = 700; // Below this level pump starts. Remember, 10-bit on ESP8266
int soilSensorMaxLevel = 550; // Avobe this level pump stops. Remember, 10-bit on ESP8266

// Timers adjustments
#ifdef WATERING_DEBUG
	unsigned long int timeReportMilis = 30000; // // ms between reports. TESTING, 30s
	unsigned long int timeWarmingMilis = 15000; // ms warming sensor before read. TESTING, 15s
	unsigned long int timeReadMilisStandBy = 45000; // ms between sensor reads - Stand by. TESTING, 45s
	unsigned long int timeReadMilisWatering = 5000; //ms between sensor reads - watering, 5s
#else
	unsigned long int timeReportMilis = 600000; // ms between reports. 10min	
	unsigned long int timeWarmingMilis = 35000; // ms warming sensor before read. 35sec
	unsigned long int timeReadMilisStandBy = 600000; //ms between sensor reads - Stand by. 600s; 10 min
	unsigned long int timeReadMilisWatering = 5000; //ms between sensor reads - watering, 5s
#endif


#define SOIL_SENSOR_READ_PIN A0
#define SOIL_SENSOR_ACTIVATE_PIN D0
#define PUMP_ACTIVATE_PIN D1
#define WATER_LEVEL_PIN D2

char *reportingApiKey = NULL;
char* ssid;
char* password;


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
void parseConfigLine(String);
void doReport();
void report();
void handleStatus();
void soilSensorProcessValue();
void soilSensorActuations();
void soilSensorChecks();
void soilSensorLoop();
void reportLoop();
void setupHW(void);
void setupSD(void);
void returnFail(String msg);
void printRTC();
void setRTC();
void loadConfig();
void printDirectory();
void handleFiles();
void handleResetSD();
void setupWiFi(void);
void setupMandatoryInitialValues();
void setup(void);
void loop(void);
#ifdef WATERING_DEBUG
	void debugStatus();
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


