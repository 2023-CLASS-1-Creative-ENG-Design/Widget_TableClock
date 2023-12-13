# Widget_TableClock

시리얼 모니터에 원하는 정보들을 입력하면 터치 스크린에 실시간으로 해당 정보들을 불러와 볼 수 있는 

Widget_TableClock입니다.
![WidgetTableClock](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/29f4f043-6a55-4d44-abb9-985fdab96d5f)

![widget](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/44b750e0-ff15-42cd-a3c7-b06d0045d6c6)



## 1. Setup

`git clone` 하여 폴더를 다운받은 후,  `.pio/libdeps/TFT_esPI` 폴더의 `User_Setup.h`를 삭제하고, root에 존재하는 `User_Setup.h`를 해당 폴더에 옮겨야 합니다.




## 2. Visual Studio Code

Visual Studio Code를 이용해 PlatformIO IDE extension을 다운로드합니다.
<img width="687" alt="platform" src="https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/daf36bb2-c3e6-4ddd-9a14-d5b380181ff2">

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
<국내> </br>
[한국투자증권 Open Trading API](https://apiportal.koreainvestment.com/intro) </br>
[pykis 라이브러리](https://github.com/pjueon/pykis) </br>
한국투자증권이 당사 계좌 보유 유저에게 제공하는 "Open Trading API" 를 사용하였습니다. 보다 편리한 사용을 위해, 기존의 한국투자증권이 제공하는 API 를 활용하는 "pykis 라이브러리"를 사용하였습니다.  

pykis 라이브러리를 사용하는 flask 파이썬 서버를 오라클 클라우드에서 백그라운드로 동작시켰고, 해당 ip 주소의 해당 포트에 대해 국내 주식 종목 코드를 입력하면 실시간(현재가)가격과 전날 종가 가격을 받아옵니다.

사용 예시)
```c
"http://개인서버 ip주소:5000/005930" (005939: 삼성전자 종목 코드)
```
출력값: ```"73000 72600"```

<해외> </br>
[Finhub API](https://finnhub.io/docs/api) </br>
해외 주식 가격 정보를 제공하는 "Finhub API" 를 사용하였습니다. 해당 API를 사용하기 위해서는 해당 사이트에 가입하여 token 을 발급받아야 합니다.

사용 예시)
```c
"https://finnhub.io/api/v1/quote?symbol={해외 주식 종목명}&token={발급받은 토큰명} (AAPL: 애플 종목 코드)
```
출력값: 다음과 같은 json 형식의 데이터가 출력됩니다.
``` {"c":195.71,"d":1.44,"dp":0.7412,"h":195.99,"l":193.67,"o":194.2,"pc":194.27,"t":1702069201}```
위 json 데이터를 파싱하여 원하는 정보를 사용할 수 있습니다.
c -> 현재 가격, d -> 변화가격(달러), dp -> 등락율, t -> 오늘 날짜를 나타냅니다. 


###  버스 API

대구에는 공식 버스 API를 지원하지 않습니다. 따라서 github상의 비공식 API를 이용해야 합니다.

[대구버스API](https://github.com/ilhaera/Daegu-bus-API) <- 개인 aws나 오라클 서버에 `git clone` 하여 index.js 8080포트로 열어서 사용

[개인서버 만들고 node.js 서버 올리기](https://wonjunjang.medium.com/aws-ec2-인스턴스에-node-js-express서버-올리기-bb3b77ae4b73) <- aws 인스턴스 생성하고 node.js 서버 올리는 방법



Ex) 아래와 같은 코드로 정류장 이름과 ID를 얻어올 수 있음

```c
"http:개인서버ip:8080/station/search/경북대학교북문앞"
```



실제 코드에서는 시리얼 모니터에 원하는 정류장의 이름과 버스를 입력하면 해당 정류장의 이름을 통해 정류장의 id를 가져오고, 이를 통해 실시간으로 원하는 버스의 도착시간을 얻습니다.


## 4. Hardware
![Hardware](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/dc801fbe-40bd-4b7a-97bb-21d2bd78bc1c)
![hardware](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/ebb1af98-32c1-4b44-8dff-aeb7afb13d97)


- [ESP32-C3-DevKitM-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html)
    - [ESP32­-C3­-MINI-­1](https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf)
    - [ESP32-C3FN4](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
  - Partition Scheme : Huge APP (3MB)   
- [1.28 inch round Touch Display](https://www.waveshare.com/1.28inch-Touch-LCD.htm)
    - LCD controller : GC9A01
    - Touch controller : CST816S

## 5. How to operate

코드를 아두이노 장치에 업로드 후, widget_tableclock.cpp에 명시된 ssid와 password를 통해 노트북, 휴대폰 등의 장치로 wifi에 접속합니다.

![wifiname](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/9a491ca9-c74c-49a2-a189-7120db5f857e)

접속이 완료되면 시리얼 모니터에 ESP32가 할당된 아래의 ip 주소가 나옵니다.

![serial](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/123005829/93656f1f-3c3d-455d-baf7-b569c78cf0a7)

wifi에 접속된 기기로 `"http://ESP32의 ip주소"`에 접속하면 아래의 화면이 출력됩니다.

![iphtml](https://github.com/2023-CLASS-1-Creative-ENG-Design/Widget_TableClock/assets/82355395/7c2eeaac-fe8b-406d-af9e-0078b362e2e1)

BUS
- Bus Number : 정류장에서 도착정보를 알고 싶은 버스의 번호
- Bus Stop : 도착 정보를 원하는 정류장의 정확한 이름


STOCK
- Korean Stock : 원하는 한국 주식의 종목코드 세개
- Korean Stock Name: 종목코드에 대응하는 종목의 한글명
- US Stock : 원하는 미국 주식의 종목코드 세개


WEATHER
- City 1 : 날씨와 시간정보를 원하는 도시이름
- City 2 : 날씨와 시간정보를 원하는 도시이름

WIFI
- WiFi Name : ESP32에 할당할 인터넷 사용가능한 wifi의 이름
- WiFi Password : 해당 wifi의 비밀번호

위의 정보들을 모두 기입후 웹페이지 하단의 submit 버튼을 누르면 esp32에서 데이터들을 받아 구조체에 저장한 후, API들을 통해 실시간으로 화면에 출력하는 방식입니다.

[WidgetTableClock 데모영상](https://www.youtube.com/watch?v=HHhxhThtotU)


[발표에 사용한 ppt 자료](https://www.canva.com/design/DAF2YugmFEQ/0_DHDZ-xFF8v2WAsDSTQKg/view?utm_content=DAF2YugmFEQ&utm_campaign=share_your_design&utm_medium=link&utm_source=shareyourdesignpanel)
