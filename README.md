# Widget_TableClock

시리얼 모니터에 원하는 정보들을 입력하면 터치 스크린에 실시간으로 해당 정보들을 불러와 볼 수 있는 

Widget_TableClock입니다.



## 1. Setup

`git clone` 하여 폴더를 다운받은 후,  `.pio/libdeps/TFT_esPI` 폴더의 `User_Setup.h`를 삭제하고, root에 존재하는 `User_Setup.h`를 해당 폴더에 옮겨야 합니다.



## 2. Visual Studio Code

Visual Studio Code를 이용해 PlatformIO IDE extension을 다운로드합니다.

![스크린샷 2023-12-06 오전 10.00.47](/Users/leechangmin/Desktop/스크린샷 2023-12-06 오전 10.00.47.png)

다운로드가 완료되면 좌측 사이드바에 새로 생긴 PlatformIO를 클릭하여 PROJECT TASKS에서 Pick a folder를 클릭합니다.

그런 다음 clone한 `Widget_TableClock` 폴더를 선택하여 환경을 세팅합니다.



## 3. How to use API

`Widget_tableclock.cpp` 의 코드 상에 명시해 놓은 API KEY Definition에 

자신의 API_KEY와 SERVER 주소를 작성해야 합니다.

```c
/*************************** [API KEY Definition] ************************************/
#define PUBLIC_DATA_API_KEY "" // 국내 주식 API_KEY
#define OPENWEATHER_API_KEY "" // 날씨 API_KEY
#define FINNHUB_STOCK_API_KEY "" // 해외 주식 API_KEY
#define BUS_API_SERVER "" // 버스 API 사용을 위한 SERVER 주소
```

### 날씨 API

날씨 정보를 불러오는 OpenWeatherMap Open API를 사용하였습니다.

[OpenWeatherMap 사용하기](https://namjackson.tistory.com/27) <- 해당 사이트를 통해 개인 API를 받고, 코드 상의 OPENWEATHER_API_KEY에 명시



실제 코드에서는 시리얼 모니터 상에 입력된 도시 이름을 통해 실시간으로 날씨 정보를 불러와 스크린에 입력합니다.



### 주식 API





### 버스 API

대구에는 공식 버스 API를 지원하지 않습니다. 따라서 github상의 비공식 API를 이용해야 합니다.

[대구버스API](https://github.com/ilhaera/Daegu-bus-API) <- 개인 aws나 오라클 서버에 `git clone` 하여 index.js 8080포트로 열어서 사용

[개인서버 만들고 node.js 서버 올리기](https://wonjunjang.medium.com/aws-ec2-인스턴스에-node-js-express서버-올리기-bb3b77ae4b73) <- aws 인스턴스 생성하고 node.js 서버 올리는 방법



Ex) 아래와 같은 코드로 정류장 이름과 ID를 얻어올 수 있음

```c
"http:개인서버ip:8080/station/search/경북대학교북문앞"
```



실제 코드에서는 시리얼 모니터에 원하는 정류장의 이름과 버스를 입력하면 해당 정류장의 이름을 통해 정류장의 id를 가져오고, 이를 통해 실시간으로 원하는 버스의 도착시간을 얻습니다.



## 4. How to operate



`[WiFi ID][WiFi PW][CITY1, CITY2][BUS, STATION][KR1, KR2, KR3][US1, US2, US3]`

위의 정보들을 시리얼 모니터 상에 입력하여 동작을 시작합니다.

