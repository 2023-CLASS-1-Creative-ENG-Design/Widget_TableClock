#include <string.h>
#include <stdio.h>

#include <time.h>
#include "EEPROM.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include <tinyxml2.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#include <AimHangul.h>
#include "CST816S.h"

// Image
#include "DUCK_HI_240.h"
#include "DUCK_TIME_240.h"
#include "DUCK_WEATHER_240.h"
#include "DUCK_BUS_240.h"
#include "DUCK_STOCK_KR_240.h"
#include "DUCK_STOCK_US_240.h"
#include "DUCK_SETTING_240.h"

#include <TaskScheduler.h>

/*************************** [CONSTANT Definition] ************************************/

/* GC9A01 display */

/* CST816S Touch */
#define TOUCH_SDA 2
#define TOUCH_SCL 3
#define TOUCH_INT 9
#define TOUCH_RST 8

#define EEPROM_SIZE 256
/*************************** [CONSTANT Definition] ************************************/

/*************************** [API KEY Definition] ************************************/
#define PUBLIC_DATA_API_KEY "HHec4G%2FFjjMjQIfzZa3yfZuItK93BQh%2BC%2FwkITl%2FCu8X3h%2BAjlF74glKicSnEN%2BVeZEOvstt07Zz%2Be%2BvmAlFVQ%3D%3D"
#define OPENWEATHER_API_KEY "8d27c680ce67b6ffebcf09d005cdd444"
#define FINNHUB_STOCK_API_KEY "ci77auhr01quivoc1e0gci77auhr01quivoc1e10"
#define BUS_API_SERVER "3.14.181.15"

/*************************** [Structure Definition] ************************************/
typedef enum {
	WIDGET_NULL = 0,
	WIDGET_STOP = 2,
	WIDGET_GO = 3
} TABLECLOCK_STATE;

typedef struct
{
	char wifiId[32];
	char wifiPw[32];
	char city[16];
	char bus[32];
	char KR[32];
	char US[32];
	
	uint8_t page;
	uint8_t page_we;
	uint8_t page_kr;
	uint8_t page_us;
} USER_DATA;

typedef struct
{
	int16_t year;  // 연도
	int8_t mon;    // 달
	int8_t mday;   // 일
	int8_t wday;   // 요일
	
	int8_t hour;
	int8_t min;
	int8_t sec;
} TIME;

typedef struct
{
	char city[2][20];
	char weather[2][20];
	int temp[2];
} WEATHER;

typedef struct
{
	char routeName[30];  // 노선 번호
	
	char stationName[100];  // 정류소명
	char stationEncoded[100];
	char stationId[100];    // 정류소 아이디
	char remainingStops[100];
	char arrivalTime[100];
} BUS;

typedef struct
{
	char code[3][9];         // 단축코드
	
	char name[3][120];          // 종목명
	char date[3][9];            // 기준일자
	char closePrice[3][12];     // 종가
	char change[3][10];         // 대비
	char percentChange[3][11];  // 등락률
} STOCK_KR;

typedef struct
{
	char name[3][30];        // 종목명
	
	int date[3];             // 기준일자
	float currentPrice[3];   // 현재가
	float change[3];         // 대비
	float percentChange[3];  // 등락률
	
} STOCK_US;
/*************************** [Structure Definition] ************************************/

/*************************** [Variable Declaration] ************************************/
TFT_eSprite img = TFT_eSprite(&tft);
CST816S touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);

TABLECLOCK_STATE my_state = WIDGET_NULL;

USER_DATA myData;
TIME myTime;
BUS myBus;
WEATHER myWeather;
STOCK_KR myStockKR;
STOCK_US myStockUS;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 32400;  // +9:00 (9hours*60mins*60seconds)
const int daylightOffset_sec = 0;

wifi_mode_t esp_wifi_mode = WIFI_MODE_NULL;

using namespace tinyxml2;
char xmlDoc[10000];
DynamicJsonDocument jsonDoc(2048);
DynamicJsonDocument jsonDoc1(2048);
DynamicJsonDocument jsonDoc2(2048);

/*************************** [Variable Declaration] ************************************/

/*************************** [Function Declaration] ************************************/

void clearStructData();
void getUserData();
void parseUserData(String);
void sendHTML(WiFiClient);
String decodeURIComponent(String);

String getValue(String, String);

void initWiFi();
bool checkWiFiStatus();

bool getDateTime();
bool getWeather(int);

bool getBusStationId();
bool getBusArrival();

bool getStockPriceKRPreviousDay(int);
bool getStockPriceUSRealTime(int);

char *url_encode(const char *str);
/*************************** [Function Declaration] ************************************/

void updateDateTimeCallback();
void updateWeatherACallback();
void updateWeatherBCallback();
void updateBusArrivalCallback();
void updateStockPriceKRPreviousDayACallback();
void updateStockPriceKRPreviousDayBCallback();
void updateStockPriceKRPreviousDayCCallback();
void updateStockPriceUSPreviousDayACallback();
void updateStockPriceUSPreviousDayBCallback();
void updateStockPriceUSPreviousDayCCallback();

Task updateDateTime(5000, TASK_FOREVER, updateDateTimeCallback);
Task updateWeatherA(150000, TASK_FOREVER, updateWeatherACallback);
Task updateWeatherB(200000, TASK_FOREVER, updateWeatherBCallback);
Task updateBusArrival(50000, TASK_FOREVER, updateBusArrivalCallback);
Task updateStockPriceKRPreviousDayA(95000, 1, updateStockPriceKRPreviousDayACallback);
Task updateStockPriceKRPreviousDayB(100000, 1, updateStockPriceKRPreviousDayBCallback);
Task updateStockPriceKRPreviousDayC(150000, 1, updateStockPriceKRPreviousDayCCallback);
Task updateStockPriceUSPreviousDayA(110000, 1, updateStockPriceUSPreviousDayACallback);
Task updateStockPriceUSPreviousDayB(115000, 1, updateStockPriceUSPreviousDayBCallback);
Task updateStockPriceUSPreviousDayC(120000, 1, updateStockPriceUSPreviousDayCCallback);

Scheduler runner;


void updateDateTimeCallback() {
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo)) {
		return;
	}
	
	myTime.year = timeinfo.tm_year + 1900;
	myTime.mon = timeinfo.tm_mon + 1;
	myTime.mday = timeinfo.tm_mday;
	myTime.wday = timeinfo.tm_wday;
	myTime.hour = timeinfo.tm_hour;
	myTime.min = timeinfo.tm_min;
	myTime.sec = timeinfo.tm_sec;
}

void updateWeatherCallback(int city) {
	WiFiClient mySocket;
	HTTPClient myHTTP;
	
	int httpCode;
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", myWeather.city[city], OPENWEATHER_API_KEY);
	myHTTP.begin(mySocket, buffer);
	
	httpCode = myHTTP.GET();
	
	if (httpCode == HTTP_CODE_OK) 
		{
			deserializeJson(jsonDoc, myHTTP.getString());
		} 
	else 
		{
			myHTTP.end();
			return;
		}
	myHTTP.end();
	
	const char* weather = jsonDoc["weather"][0]["main"];
	strcpy(myWeather.weather[city], weather);
	myWeather.temp[city] = (int)(jsonDoc["main"]["temp"]) - 273.0;
}
void updateWeatherACallback() {
	updateWeatherCallback(0);
}
void updateWeatherBCallback() {
	updateWeatherCallback(1);
}

void updateBusArrivalCallback() {
	WiFiClient mySocket;
	HTTPClient myHTTP;
	
	int httpCode;
	char buffer[300];
	StaticJsonDocument<200> doc;
	
	snprintf(buffer, sizeof(buffer), "http://%s:8080/station/%s",BUS_API_SERVER,myBus.stationId);
	myHTTP.begin(mySocket, buffer);
	
	httpCode = myHTTP.GET();
	
	if (httpCode == HTTP_CODE_OK) 
		{
			deserializeJson(jsonDoc2, myHTTP.getString());
		} 
	else 
		{
			myHTTP.end();
			return;
		}
	myHTTP.end();
	
	// JSON 배열의 크기
	int arraySize = jsonDoc2.size();
	int flag = 0;
	// 배열 요소를 반복하여 객체 정보 추출
	for (int i = 0; i < arraySize; i++) {
		JsonObject Bus = jsonDoc2[i];
		if(!Bus.containsKey("name")) continue;
		const char* busName = Bus["name"];
		if (strcmp(busName, myBus.routeName) == 0) {
			// 버스 정보에 대한 반복문
			if(Bus.containsKey("bus") && Bus["bus"].size() > 0 && Bus["bus"][0].containsKey("남은정류소")) {
				const char* remainingStops = Bus["bus"][0]["남은정류소"];
				const char* arrivalTime = Bus["bus"][0]["도착예정시간"];
				strcpy(myBus.remainingStops, remainingStops);
				strcpy(myBus.arrivalTime, arrivalTime);
			}
			else {
				strcpy(myBus.remainingStops, "-1");
				strcpy(myBus.arrivalTime, "-1");
			}
			break;
		}
		else {
			continue;
		}
	}
}

void updateStockPriceKRPreviousDayCallback(int stock) {
	WiFiClient mySocket;
	HTTPClient myHTTP;
	
	int httpCode;
	char buffer[300];
	
	snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/1160100/service/GetStockSecuritiesInfoService/getStockPriceInfo?serviceKey=%s&numOfRows=1&pageNo=1&likeSrtnCd=%s", PUBLIC_DATA_API_KEY, myStockKR.code[stock]);
	myHTTP.begin(mySocket, buffer);
	
	httpCode = myHTTP.GET();
	
	if (httpCode == HTTP_CODE_OK) {
		Serial.println("[OK]");
		strcpy(xmlDoc, myHTTP.getString().c_str());
	} else {
		myHTTP.end();
		return;
	}
	myHTTP.end();
	
	XMLDocument xmlDocument;
	if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
		return;
	};
	
	XMLNode* root = xmlDocument.RootElement();
	
	// 종목명
	XMLElement* element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("itmsNm");
	strcpy(myStockKR.name[stock], element->GetText());
	
	// 기준일자
	element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("basDt");
	strcpy(myStockKR.date[stock], element->GetText());
	myStockKR.date[stock][8] = '\0';
	
	// 종가
	element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("clpr");
	strcpy(myStockKR.closePrice[stock], element->GetText());
	
	// 대비
	element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("vs");
	strcpy(myStockKR.change[stock], element->GetText());
	
	// 등락률
	element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("fltRt");
	strcpy(myStockKR.percentChange[stock], element->GetText());
}

void updateStockPriceKRPreviousDayACallback() {
	updateStockPriceKRPreviousDayCallback(0);
}
void updateStockPriceKRPreviousDayBCallback() {
	updateStockPriceKRPreviousDayCallback(1);
}
void updateStockPriceKRPreviousDayCCallback() {
	updateStockPriceKRPreviousDayCallback(2);
}

void updateStockPriceUSPreviousDayCallback(int stock) {
	WiFiClientSecure* client = new WiFiClientSecure;
	HTTPClient myHTTP;
	
	int httpCode;
	char buffer[300];
	snprintf(buffer, sizeof(buffer), "https://finnhub.io/api/v1/quote?symbol=%s&token=%s", myStockUS.name[stock], FINNHUB_STOCK_API_KEY);
	client->setInsecure();
	myHTTP.begin(*client, buffer);
	
	httpCode = myHTTP.GET();
	
	if (httpCode == HTTP_CODE_OK) 
		{
			deserializeJson(jsonDoc, myHTTP.getString());
		} 
	else 
		{
			myHTTP.end();
			return;
		}
	myHTTP.end();
	
	myStockUS.date[stock] = (int)(jsonDoc["t"]);
	myStockUS.currentPrice[stock] = (float)(jsonDoc["c"]);
	myStockUS.change[stock] = (float)(jsonDoc["d"]);
	myStockUS.percentChange[stock] = (float)(jsonDoc["dp"]);
}
void updateStockPriceUSPreviousDayACallback() {
	updateStockPriceUSPreviousDayCallback(0);
}
void updateStockPriceUSPreviousDayBCallback() {
	updateStockPriceUSPreviousDayCallback(1);
}
void updateStockPriceUSPreviousDayCCallback() {
	updateStockPriceUSPreviousDayCallback(2);
}


char *url_encode(const char *str) {
	const char *reserved = "=&+?/;#~%'\"<>{}[]|\\^";
	size_t len = strlen(str);
	char *encoded = (char *)malloc(len * 3 + 1);
	size_t j = 0;
	
	for (size_t i = 0; i < len; ++i) {
		if (isalnum(str[i]) || strchr(reserved, str[i]) != NULL) {
			encoded[j++] = str[i];
		} else {
			snprintf(&encoded[j], 4, "%%%02X", (unsigned char)str[i]);
			j += 3;
		}
	}
	
	encoded[j] = '\0';
	return encoded;
}

const char* ssid = "ESP32_test";
const char* password = "123456789";

WiFiServer server(80);

String header;

void sendHTML(WiFiClient client) {
	client.println("HTTP/1.1 200 OK");
	client.println("Content-type:text/html; charset=UTF-8");
	client.println("Connection: close");
	client.println();

	client.println("<!DOCTYPE html>\n<html>\n  <head>\n    <title>Data Input</title>\n    <style>		\n    body {font-family: Arial, sans-serif;display: flex;justify-content: center;align-items: center; height: 100vh;margin: 0;background-color: #f3f3f3;}\n    form {background: #fff;padding: 20px 20px 5px 20px;border-radius: 6px;box-shadow: 0 0 10px rgba(0,0,0,0.2);width: 350px;margin-bottom: 10px}\n    h2 {color: #af0f0f;padding: 10px;margin: -20px -20px 0 -20px;border-radius: 8px;text-align: center;}\n    .bodyTitle {background-color:#af0f0f;color: #fff;padding: 10px;margin: -20px -20px 10px -20px;border-radius: 8px 8px 0 0;}\n    .input-group {display: flex;justify-content: space-between;align-items: center;margin-bottom: 10px;}\n    input[type='text'] {width: 140px;padding: 5px;}\n    input[type='submit'] {width: 100%;padding: 8px;background-color: #797977;color: white;border: none;cursor: pointer;border-radius: 8px;transition: background-color 0.3s ease;}\n    input[type='submit']:hover {background-color: #494949;}\n    input[type='submit']:active {transform: scale(0.98);}\n    label, input[type='text'] {display: block;width: 100%;}\n    </style>\n    </head>\n  <body>\n    <div class="container">\n    <h2>SMART TABLE CLOCK</h2>\n    <form action='/submitData' method='post' id='dataForm'>\n        <form id='bus'>\n            <h3 class='bodyTitle'>BUS</h3>\n            <div class="input-group">\n                <label for='busNumber'>Bus Number:</label>\n                <input type='text' id='busNumber' name='busNumber'><br>\n            </div>\n            <div class="input-group">\n                <label for='busStop'>Bus Stop:</label>\n                <input type='text' id='busStop' name='busStop'><br>\n            </div>\n        </form>\n        <form id='koreaStock'>\n            <h3 class='bodyTitle'>STOCK</h3>\n            <div class="input-group">\n                <label for='koreanStock1'>Korean Stock 1:</label>\n                <input type='text' id='koreanStock1' name='koreanStock1'><br>\n            </div>\n            <div class="input-group">\n                <label for='koreanStock2'>Korean Stock 2:</label>\n                <input type='text' id='koreanStock2' name='koreanStock2'><br>\n            </div>\n            <div class="input-group">\n                <label for='koreanStock3'>Korean Stock 3:</label>\n                <input type='text' id='koreanStock3' name='koreanStock3'><br>\n            </div>\n            <div class="input-group">\n                <label for='usStock1'>US Stock 1:</label>\n                <input type='text' id='usStock1' name='usStock1'><br>\n            </div>\n            <div class="input-group">\n                <label for='usStock2'>US Stock 2:</label>\n                <input type='text' id='usStock2' name='usStock2'><br>\n            </div>\n            <div class="input-group">\n                <label for='usStock3'>US Stock 3:</label>\n                <input type='text' id='usStock3' name='usStock3'><br>\n            </div>\n        </form>\n        <form id='city'>\n            <h3 class='bodyTitle'>WEATHER</h3>\n            <div class="input-group">\n                <label for='city1'>City 1:</label>	\n<input type='text' id='city1' name='city1'><br>\n            </div>\n            <div class="input-group">\n                <label for='city2'>City 2:</label>		\n                <input type='text' id='city2' name='city2'><br>\n            </div>\n        </form>\n        <form id='wifi'>\n            <h3 class='bodyTitle'>WIFI</h3>\n            <div class="input-group">\n                <label for='wifiName'>WiFi Name:</label>\n                <input type='text' id='wifiName' name='wifiName'><br>\n            </div>\n            <div class="input-group">\n                <label for='wifiPassword'>WiFi Password:</label>	\n<input type='text' id='wifiPassword' name='wifiPassword'><br>\n            </div>\n        </form>\n        <div class="input-group">\n            <input type='text' id='eof' name='eof' style='display:none;'><br>\n            <input type='submit' value='Submit'>\n        </div>\n    </form>\n    </div>\n  </body>\n</html>");
	//client.println("<!DOCTYPE html>\n<html>\n<head>\n	<title>Data Input</title>\n	<style>\n		body {\n			font-family: Arial, sans-serif;\n			margin: 50px;\n		}\n		h2 {\n			color: #333;\n		}\n		label {\n			display: block;\n			margin-top: 10px;\n		}\n		input[type='text'] {\n			width: 200px;\n			padding: 5px;\n			margin-top: 5px;\n		}\n		input[type='submit'] {\n			width: 100px;\n			padding: 8px;\n			margin-top: 20px;\n			background-color: #4CAF50;\n			color: white;\n			border: none;\n			cursor: pointer;\n		}\n	</style>\n</head>\n<body>\n	<h2>Enter Bus Number, Bus Stop, Korean Stock, US Stock, WiFi Name, WiFi Password, City 1, and City 2</h2>\n	<form action='/submitData' method='post' id='dataForm'>\n		<label for='busNumber'>Bus Number:</label>\n		<input type='text' id='busNumber' name='busNumber'><br>\n		<label for='busStop'>Bus Stop:</label>\n		<input type='text' id='busStop' name='busStop'><br>\n		<label for='koreanStock1'>Korean Stock 1:</label>\n		<input type='text' id='koreanStock1' name='koreanStock1'><br>\n		<label for='koreanStock2'>Korean Stock 2:</label>\n		<input type='text' id='koreanStock2' name='koreanStock2'><br>\n		<label for='koreanStock3'>Korean Stock 3:</label>\n		<input type='text' id='koreanStock3' name='koreanStock3'><br>\n		<label for='usStock1'>US Stock 1:</label>\n		<input type='text' id='usStock1' name='usStock1'><br>\n		<label for='usStock2'>US Stock 2:</label>\n		<input type='text' id='usStock2' name='usStock2'><br>\n		<label for='usStock3'>US Stock 3:</label>\n		<input type='text' id='usStock3' name='usStock3'><br>\n		<label for='wifiName'>WiFi Name:</label>\n		<input type='text' id='wifiName' name='wifiName'><br>\n		<label for='wifiPassword'>WiFi Password:</label>\n		<input type='text' id='wifiPassword' name='wifiPassword'><br>\n		<label for='city1'>City 1:</label>\n		<input type='text' id='city1' name='city1'><br>\n		<label for='city2'>City 2:</label>\n		<input type='text' id='city2' name='city2'><input type='text' id='eof' name='eof' style='display:none;'><br>\n		<input type='submit' value='Submit'>\n	</form>\n</body>\n</html>");
}

String decodeURIComponent(String s) {
	String result = "";
	char c;
	for (uint i = 0; i < s.length(); i++) {
		c = s.charAt(i);
		if (c == '%') {
			String hex = s.substring(i + 1, i + 3);
			i += 2;
			c = strtol(hex.c_str(), NULL, 16);
		} else if (c == '+') {
			c = ' ';
		}
		result += c;
	}
	return result;
}

String getValue(String data, String key) {
	String value = "";
	String keyString = key + "=";
	int idx = data.indexOf(keyString);
	if (idx != -1) {
		int endIdx = data.indexOf('&', idx);
		if (endIdx == -1) {
			endIdx = data.length();
		}
		value = data.substring(idx + keyString.length(), endIdx);
		value.replace("+", " ");
		value = decodeURIComponent(value);
	}
	return value;
}
			
void parseUserData(String data) {
	String busNumber = getValue(data, "busNumber");
	String busStop = getValue(data, "busStop");
	String koreanStock1 = getValue(data, "koreanStock1");
	String koreanStock2 = getValue(data, "koreanStock2");
	String koreanStock3 = getValue(data, "koreanStock3");
	String usStock1 = getValue(data, "usStock1");
	String usStock2 = getValue(data, "usStock2");
	String usStock3 = getValue(data, "usStock3");
	String city1 = getValue(data, "city1");
	String city2 = getValue(data, "city2");
	String wifiName = getValue(data, "wifiName");
	String wifiPassword = getValue(data, "wifiPassword");
	
	// 파싱한 데이터 출력
	Serial.print("Bus Number: ");
	Serial.println(busNumber);
	Serial.print("Bus Stop: ");
	Serial.println(busStop);
	Serial.print("Korean Stock 1: ");
	Serial.println(koreanStock1);
	Serial.print("Korean Stock 2: ");
	Serial.println(koreanStock2);
	Serial.print("Korean Stock 3: ");
	Serial.println(koreanStock3);
	Serial.print("US Stock 1: ");
	Serial.println(usStock1);
	Serial.print("US Stock 2: ");
	Serial.println(usStock2);
	Serial.print("US Stock 3: ");
	Serial.println(usStock3);
	Serial.print("City 1: ");
	Serial.println(city1);
	Serial.print("City 2: ");
	Serial.println(city2);
	Serial.print("WiFi Name: ");
	Serial.println(wifiName);
	Serial.print("WiFi Password: ");
	Serial.println(wifiPassword);
	
	strncpy(myData.wifiId, wifiName.c_str(), sizeof(myData.wifiId) - 1);
	myData.wifiId[sizeof(myData.wifiId) - 1] = '\0'; // null-terminate
	
	strncpy(myData.wifiPw, wifiPassword.c_str(), sizeof(myData.wifiPw) - 1);
	myData.wifiPw[sizeof(myData.wifiPw) - 1] = '\0'; // null-terminate
	
	strncpy(myBus.routeName, busNumber.c_str(), sizeof(myBus.routeName) - 1);
	myBus.routeName[sizeof(myBus.routeName) - 1] = '\0'; // null-terminate
	
	strncpy(myBus.stationName, busStop.c_str(), sizeof(myBus.stationName) - 1);
	myBus.stationName[sizeof(myBus.stationName) - 1] = '\0'; // null-terminate
	
	strncpy(myStockKR.code[0], koreanStock1.c_str(), sizeof(myStockKR.code[0]) - 1);
	myStockKR.code[0][sizeof(myStockKR.code[0]) - 1] = '\0'; // null-terminate
	
	strncpy(myStockKR.code[1], koreanStock2.c_str(), sizeof(myStockKR.code[1]) - 1);
	myStockKR.code[1][sizeof(myStockKR.code[1]) - 1] = '\0'; // null-terminate
	
	strncpy(myStockKR.code[2], koreanStock3.c_str(), sizeof(myStockKR.code[2]) - 1);
	myStockKR.code[2][sizeof(myStockKR.code[2]) - 1] = '\0'; // null-terminate
	
	strncpy(myStockUS.name[0], usStock1.c_str(), sizeof(myStockUS.name[0]) - 1);
	myStockUS.name[0][sizeof(myStockUS.name[0]) - 1] = '\0'; // null-terminate
	
	strncpy(myStockUS.name[1], usStock2.c_str(), sizeof(myStockUS.name[1]) - 1);
	myStockUS.name[1][sizeof(myStockUS.name[1]) - 1] = '\0'; // null-terminate
	
	strncpy(myStockUS.name[2], usStock3.c_str(), sizeof(myStockUS.name[2]) - 1);
	myStockUS.name[2][sizeof(myStockUS.name[2]) - 1] = '\0';
	
	strncpy(myWeather.city[0], city1.c_str(), sizeof(myWeather.city[0]) - 1);
	myWeather.city[0][sizeof(myWeather.city[0]) - 1] = '\0';
	
	strncpy(myWeather.city[1], city2.c_str(), sizeof(myWeather.city[1]) - 1);
	myWeather.city[1][sizeof(myWeather.city[1]) - 1] = '\0';
}

int getClient() {

    WiFiClient client;

    while(!(client = server.available())) {}
    
    Serial.println("New Client.");
    
    String currentLine = "";
    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            Serial.write(c);
            currentLine += c;
            
            if (c == '\n') {
                if (currentLine.indexOf("GET /") >= 0) {
                    sendHTML(client);
                    currentLine = "";
                    client.stop();
                    Serial.println("Client disconnected.");
                    return 1;
                }
            }
            if(currentLine.indexOf("POST /submitData") >= 0 && currentLine.indexOf("eof") >= 0) {
                parseUserData(currentLine);
                client.stop();
                Serial.println("Client disconnected.");
                return 0;
            }
        }
    }

    return 0;
}
void getUserData() {
	img.pushImage(0, 0, 240, 240, DUCK_SETTING_240);
	img.pushSprite(0, 0);
	// setting...
	tft.setCursor(30, 55);  // 중앙정렬
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print("Setting...");
    
    while(getClient()) {}
		
	auto foo = url_encode(myBus.stationName);
	strcpy(myBus.stationEncoded, foo);
	free(foo);
	
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	
	img.pushImage(0, 0, 240, 240, DUCK_HI_240);
	img.pushSprite(0, 0);
	
	tft.setCursor(35, 80);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print("HI");
	
	initWiFi();
	delay(3000);
	
	while(!getBusStationId()) delay(1000);
}

void setup() 
{
	// put your setup code here, to run once:
	Serial.begin(115200);
	
	Serial.println();
	Serial.println();
	Serial.println("-------------------------------------");
	Serial.println("----------widget_tableclock----------");
	Serial.println("-------------------------------------");
	Serial.println();
	Serial.println();
	
	Serial.println("-------------------------------------");
	Serial.println("[SETUP] Start]");
	Serial.println("-------------------------------------");
	
	touch.begin();
	
	tft.init();
	tft.setRotation(0);
	tft.setSwapBytes(true);
	img.setSwapBytes(true);
	tft.fillScreen(TFT_WHITE);
	img.createSprite(240, 240);
	
	
	Serial.print("Setting AP...");
	WiFi.softAP(ssid, password);
	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);
	
	server.begin();
	
	getUserData();
	
	Serial.println("-------------------------------------");
	Serial.println("[SETUP] End");
	Serial.println("-------------------------------------");
	
	runner.init();
	runner.addTask(updateDateTime);
	runner.addTask(updateWeatherA);
	runner.addTask(updateWeatherB);
	runner.addTask(updateBusArrival);
	runner.addTask(updateStockPriceKRPreviousDayA);
	runner.addTask(updateStockPriceKRPreviousDayB);
	runner.addTask(updateStockPriceKRPreviousDayC);
	runner.addTask(updateStockPriceUSPreviousDayA);
	runner.addTask(updateStockPriceUSPreviousDayB);
	runner.addTask(updateStockPriceUSPreviousDayC);
	delay(1000);
	updateDateTime.enable();
	updateWeatherA.enable();
	updateWeatherB.enable();
	updateBusArrival.enable();
	updateStockPriceKRPreviousDayA.enable();
	updateStockPriceKRPreviousDayB.enable();
	updateStockPriceKRPreviousDayC.enable();
	updateStockPriceUSPreviousDayA.enable();
	updateStockPriceUSPreviousDayB.enable();
	updateStockPriceUSPreviousDayC.enable();
}



uint32_t tick_cur = 0;
uint32_t tick_old[10] = {0};
GESTURE prev = NONE;

void loop() 
{   
	runner.execute();
	
	tick_cur = millis();

	if((tick_cur - tick_old[0]) > 500)
		{
			if(!touch.available()) prev = NONE;
			else if(prev != (GESTURE)touch.data.gestureID)
				{
					Serial.println("-------------------------------------");
					Serial.println("[TOUCH] Current Page");
					prev = (GESTURE)touch.data.gestureID;
					switch (touch.data.gestureID) 
					{
						case SWIPE_LEFT:
							if(myData.page < 4) myData.page++;
							Serial.println("SWIPE_LEFT");
							break;
						case SWIPE_RIGHT:
							if(myData.page > 0) myData.page--;
							Serial.println("SWIPE_RIGHT");
							break;
						case SWIPE_UP:
							if(myData.page == 1)
								{
									if(myData.page_we < 1) myData.page_we++;
								}
							else if(myData.page == 3)
								{
									if(myData.page_kr < 2) myData.page_kr++;
								}
							else if(myData.page == 4)
								{
									if(myData.page_us < 2) myData.page_us++;
								}
							Serial.println("SWIPE_UP");
							break;
						case SWIPE_DOWN:
							if(myData.page == 1)
								{
									if(myData.page_we > 0) myData.page_we--;
								}
							else if(myData.page == 3) 
								{
									if(myData.page_kr > 0) myData.page_kr--;
								}
							else if(myData.page == 4) 
								{
									if(myData.page_us > 0) myData.page_us--;
								}
							Serial.println("SWIPE_DOWN");
							break;
						default:
							Serial.println("NONE");
							goto ignore;
							break;
					}
					
					printf("[PAGE] %d\r\n", myData.page);
					printf("[PAGE_WE] %d\r\n", myData.page_we);
					printf("[PAGE_KR] %d\r\n", myData.page_kr);
					printf("[PAGE_US] %d\r\n", myData.page_us);
					
					switch(myData.page)
					{
						case 0:
							getDateTime();
							tick_old[1] = tick_cur;
							break;
						case 1:
							while(!getWeather(myData.page_we)) delay(1000);
							tick_old[2] = tick_cur;
							break;
						case 2:
							while(!getBusArrival()) delay(1000);
							tick_old[3] = tick_cur;
							break;
						case 3:
							while(!getStockPriceKRPreviousDay(myData.page_kr)) delay(1000);
							tick_old[4] = tick_cur;
							break;
						case 4:
							while(!getStockPriceUSRealTime(myData.page_us)) delay(1000);
							tick_old[5] = tick_cur;
							break;
						default:
							Serial.println("[ERROR] Wrong Page");
							break;
					}
				ignore:
					Serial.println("-------------------------------------");
				}
			tick_old[0] = tick_cur;
		}
	
	if(((tick_cur - tick_old[1]) > 1000) && (myData.page == 0))
		{
			getDateTime();
			tick_old[1] = tick_cur;
		} 
	
	if(((tick_cur - tick_old[2]) > 30000) && (myData.page == 1))
		{
			while(!getWeather(myData.page_we)) delay(1000);
			tick_old[2] = tick_cur;
		} 
	
	if(((tick_cur - tick_old[3]) > 15000) && (myData.page == 2))
		{
			while(!getBusArrival()) delay(1000);
			tick_old[3] = tick_cur;
		} 
	
	if(((tick_cur-tick_old[4]) > (1000*60*60)) && (myData.page == 3))
		{
			while(!getStockPriceKRPreviousDay(myData.page_kr)) delay(1000);
			
			if(!checkWiFiStatus())
				{
					Serial.println("[ERROR] WiFi Disconnect");
					getUserData();
				}
			tick_old[4] = tick_cur;
		}
	
	if(((tick_cur-tick_old[5]) > 15000) && (myData.page == 4))
		{
			while(!getStockPriceUSRealTime(myData.page_us)) delay(1000);
			tick_old[5] = tick_cur;
		}
}

/***************************Function Definition************************************/
void clearStructData() 
{
	memset(&myData, 0, sizeof(USER_DATA));
	memset(&myTime, 0, sizeof(TIME));
	memset(&myBus, 0, sizeof(BUS));
	memset(&myWeather, 0, sizeof(WEATHER));
	memset(&myStockKR, 0, sizeof(STOCK_KR));
	memset(&myStockUS, 0, sizeof(STOCK_US));
	memset(&myData, 0, sizeof(USER_DATA));
	memset(&myData, 0, sizeof(USER_DATA));
	
	memset(xmlDoc, 0, sizeof(xmlDoc));
}

void initWiFi() {
	uint8_t wifi_timeout = 0;
	Serial.println("WIFI Connecting");
	WiFi.begin(myData.wifiId, myData.wifiPw);
	
	if (checkWiFiStatus())
		Serial.println("WIFI CONNECTED");
	else {
		Serial.println();
		Serial.println("WiFi Not Connected, Check [WiFi ID][WiFi PW]");
		getUserData();
	}
}

bool checkWiFiStatus() {
	uint8_t wifi_timeout = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		wifi_timeout++;
		Serial.print(".");
		if (wifi_timeout == 20) {
			return 0;
		}
	}
	return 1;
}

bool getDateTime() 
{
	// 화면 전환
	img.pushImage(0, 0, 240, 240, DUCK_TIME_240);
	img.pushSprite(0, 0);
	
	// 월/일 시간:분
	tft.setCursor(80, 50);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	
	if (myTime.mon < 10) tft.print("0");
	tft.print(myTime.mon);
	tft.print("/");
	if (myTime.mday < 10) tft.print("0");
	tft.print(myTime.mday);
	
	tft.setCursor(80, 80);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	
	if (myTime.hour < 10) tft.print("0");
	tft.print(myTime.hour);
	tft.print(":");
	if (myTime.min < 10) tft.print("0");
	tft.println(myTime.min);
	
	return true;
}

bool getWeather(int city) 
{
	// 화면 전환
	img.pushImage(0, 0, 240, 240, DUCK_WEATHER_240);
	img.pushSprite(0, 0);
	
	// 도시
	tft.setCursor(60, 40);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myWeather.city[city]);
	
	// 날씨
	tft.setCursor(60, 70);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myWeather.weather[city]);
	
	// 기온
	tft.setCursor(60, 100);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myWeather.temp[city]);
	tft.print("C");
	
	return true;
}

bool getBusStationId() 
{
	Serial.println("-------------------------------------");
	Serial.println("[REQUEST] Bus Station Info");
	
	WiFiClient mySocket;
	HTTPClient myHTTP;                                                                                                    
	
	int httpCode;
	char buffer[300];
	
	snprintf(buffer, sizeof(buffer), "http://%s:8080/station/search/%s", BUS_API_SERVER,myBus.stationEncoded);
	myHTTP.begin(mySocket, buffer);
	delay(500);
	
	httpCode = myHTTP.GET();
	printf("[HTTP CODE] %d \r\n", httpCode);
	
	if (httpCode == HTTP_CODE_OK) 
		{
			Serial.println("[OK]");
			auto foo = myHTTP.getString();
			Serial.println(foo);
			deserializeJson(jsonDoc1, foo);
		} 
	else 
		{
			Serial.println("[ERROR]");
			myHTTP.end();
			Serial.println("-------------------------------------");
			return false;
		}
	myHTTP.end();
	
	const char* busID = jsonDoc1[0]["id"];
	strcpy(myBus.stationId, busID);
	
	// 정류소아이디
	printf("[정류소아이디] %s\r\n", myBus.stationId);
	
	// 정류소명
	printf("[정류소명] %s\r\n", myBus.stationName);
	
	Serial.println("-------------------------------------");
	return true;
}

bool getBusArrival()  // getBusArrivalItem Operation
{
	// 화면 전환
	img.pushImage(0, 0, 240, 240, DUCK_BUS_240);
	img.pushSprite(0, 0);
	
	// 정류소명
	String staStr(myBus.stationName);
	int nameLen = staStr.length() / 3;
	AimHangul_v2(120 - (float)(nameLen/2) * 16, 50, staStr, TFT_WHITE);  // 중앙정렬
	
	// 버스와 도착 시간
	char preBuf[100];
	if(strcmp(myBus.arrivalTime, "-1")) {
		snprintf(preBuf, sizeof(preBuf), "%s번 버스 %s후 도착", myBus.routeName, myBus.arrivalTime);
	}
	else {
		snprintf(preBuf, sizeof(preBuf), "%s번 버스 경로상 부재", myBus.routeName);
	}
	String preStr(preBuf);
	AimHangul_v2(35, 100, preStr, TFT_WHITE);  // 중앙정렬
	
	return true;
}

bool getStockPriceKRPreviousDay(int stock)  // 한국주식 전날 시세
{
	// 화면 전환
	img.pushImage(0, 0, 240, 240, DUCK_STOCK_KR_240);
	img.pushSprite(0, 0);
	
	// 날짜
	tft.setCursor(120-(12*4), 30); // 중앙정렬
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
	tft.printf("%s", myStockKR.date[stock]);
	
	// 종목
	String str(myStockKR.name[stock]);
	int nameLen = strlen(myStockKR.name[stock]) / 3;
	// printf("name len %d\r\n", nameLen);
	AimHangul_v2(120 - (float)(nameLen/2) * 16, 50, str, TFT_WHITE);  // 중앙정렬
	
	// 종가
	int closePriceLen = strlen(myStockKR.closePrice[stock]);
	// printf("closePriceLen %d\r\n", closePriceLen);
	tft.setCursor(120 - (float)(closePriceLen/2) * 20, 50 + 36);  // 중앙정렬
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myStockKR.closePrice[stock]);
	// printf("%d\r\n", tft.getCursorX());
	
	// 대비, 등락률
	char changeBuffer[50];
	snprintf(changeBuffer, sizeof(changeBuffer), "%s %s%%", myStockKR.change[stock], myStockKR.percentChange[stock]);
	int changeBufferLen = strlen(changeBuffer);
	// printf("changeBufferLen %d\r\n", changeBufferLen);
	tft.setCursor(120 - (float)(changeBufferLen/2) * 12, 50 + 36 + 30);  // 중앙정렬
	if (myStockKR.change[stock][0] == '-') tft.setTextColor(TFT_BLUE, TFT_WHITE, 1);
	else tft.setTextColor(TFT_RED, TFT_WHITE, 1);
	tft.setTextSize(2);
	tft.print(changeBuffer);
	
	return true;
}

bool getStockPriceUSRealTime(int stock)  // 미국주식 실시간 시세
{
	img.pushImage(0, 0, 240, 240, DUCK_STOCK_US_240);
	img.pushSprite(0, 0);
	
	time_t rawTime = myStockUS.date[stock];
	struct tm ts;
	ts = *localtime(&rawTime);
	
	// 시각
	tft.setCursor(72, 30);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
	
	if (ts.tm_hour < 10) tft.print("0");
	tft.print(ts.tm_hour);
	tft.print(":");
	if (ts.tm_min < 10) tft.print("0");
	tft.print(ts.tm_min);
	tft.print(":");
	if(ts.tm_sec < 10) tft.print("0");
	tft.print(ts.tm_sec);
	
	// 종목명
	int nameLen = strlen(myStockUS.name[stock]);
	tft.setCursor(120 - (float)(nameLen/2) * 18, 55);  // 중앙정렬
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myStockUS.name[stock]);
	
	// 현재가
	char curPriceBuf[10];
	snprintf(curPriceBuf, sizeof(curPriceBuf), "%.2f", myStockUS.currentPrice[stock]);
	int curPriceLen = strlen(curPriceBuf);
	tft.setCursor(120 - (float)(curPriceLen/2) * 18, 50 + 36);  // 중앙정렬
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(3);
	tft.print(myStockUS.currentPrice[stock]);
	
	// 대비, 등락률
	char changeBuf[50];
	snprintf(changeBuf, sizeof(changeBuf), "%.2f %.2f%%", myStockUS.change[stock], myStockUS.percentChange[stock]);
	int changeBufLen = strlen(changeBuf);
	tft.setCursor(120 - (float)(changeBufLen/2) * 12, 50 + 36 + 30);  // 중앙정렬
	if (myStockUS.change[stock] >= 0) tft.setTextColor(TFT_RED, TFT_WHITE, 1);
	else tft.setTextColor(TFT_BLUE, TFT_WHITE, 1);
	tft.setTextSize(2);
	tft.print(changeBuf);
	
	return true;
}
/***************************Function Definition************************************/
