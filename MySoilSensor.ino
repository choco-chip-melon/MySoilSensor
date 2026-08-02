#include <Arduino.h>
#include <NimBLEDevice.h>

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//#define SUPER_MINI
#define DEEP_SLEEP

// BLE Company ID (適当な値を設定してください)
#define COMPANY_ID 0x1234
#define DEVICE_NO 0x01

#ifdef SUPER_MINI
  //Super Mini用

  // 電池電圧測定ピン
  // 分圧回路を接続していない場合は固定値
  // 未使用(予約)
  #define PIN_BATTERY A2

  // 温度センサー(Temperature sensor) DS18B Onewire 
  #define PIN_DS18B20_ONEWIRE 20

  // 水分センサー(Moisture sensor) LM393電源
  #define PIN_LM393_1_VCC 1
  #define PIN_LM393_2_VCC 2

  // 水分センサー(Moisture sensor) ADS1115 I2C
  #define PIN_ADS1115_SDA 3
  #define PIN_ADS1115_SCL 4

  #define LED 8

#else
  //XIAO Seed Studio用
  // 電池電圧測定ピン
  // 分圧回路を接続していない場合は固定値
  // 未使用(予約)
  #define PIN_BATTERY A2

  // 温度センサー(Temperature sensor) DS18B Onewire 
  #define PIN_DS18B20_ONEWIRE 6

  // 水分センサー(Moisture sensor) LM393電源
  #define PIN_LM393_1_VCC_VCC 7
  #define PIN_LM393_2_VCC 8

  // 水分センサー(Moisture sensor) ADS1115 I2C
  #define PIN_ADS1115_SDA 9
  #define PIN_ADS1115_SCL 10

  #define LED 8

#endif

// Serial
#define SERIAL_BAUDRATE 115200

// ADS1115
Adafruit_ADS1115 ads;

// BLE UUID
#define BLE_DEVICE_NAME "MySoilSensor"

BLECharacteristic *pCharacteristic;

// 温度センサー DS18B Onewire 設定
OneWire oneWire(PIN_DS18B20_ONEWIRE);
DallasTemperature ds18b20(&oneWire);

DeviceAddress tempSensor0, tempSensor1;

//#define DEEP_SLEEP
#define DEEP_SLEEP_CYCLE_SEC 60ULL

int8_t status = 0;
bool use_ads1115 = false;

//                             0b76543210
#define STATUS_ADS1115_FOUND   0b00000001
#define STATUS_TEMP_CH0_FOUND  0b00000010
#define STATUS_TEMP_CH1_FOUND  0b00000100


void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    delay(1000);

    Serial.println("Start");

    // BLE初期化
    NimBLEDevice::init(BLE_DEVICE_NAME);

    // 水分センサー LM393 VCC
    pinMode(PIN_LM393_1_VCC, OUTPUT);
    pinMode(PIN_LM393_2_VCC, OUTPUT);

    digitalWrite(PIN_LM393_1_VCC, LOW);
    digitalWrite(PIN_LM393_2_VCC, LOW);

    // LED
    pinMode(LED, OUTPUT);

    //--------------------------------------------------
    // I2C
    //--------------------------------------------------
    Wire.begin(PIN_ADS1115_SDA, PIN_ADS1115_SCL);   //
    if (!ads.begin()) {
    //if (!ads.begin(0x48)) {
        Serial.println("ADS1115 not found\n");
        use_ads1115 = false;
    }else{
      Serial.print("ADS1115 found!\n");
      use_ads1115 = true;

      ads.setGain(GAIN_ONE);
        status |= STATUS_ADS1115_FOUND;
    }

    //--------------------------------------------------
    // DS18B20
    //--------------------------------------------------
    ds18b20.begin();

    uint8_t sensorCount = ds18b20.getDeviceCount();
    Serial.print("DS18B20 found: ");
    Serial.println(sensorCount);

    if (sensorCount >= 1) {
        ds18b20.getAddress(tempSensor0, 0);
        status |= STATUS_TEMP_CH0_FOUND;
    }

    if (sensorCount >= 2) {
        ds18b20.getAddress(tempSensor1, 1);
        status |= STATUS_TEMP_CH1_FOUND;
    }

  Serial.println("Scan Start.\n");
  //--------------------------------------------------
  // センサーON
  //--------------------------------------------------
  digitalWrite(PIN_LM393_1_VCC, HIGH);
  digitalWrite(PIN_LM393_2_VCC, HIGH);

  digitalWrite(LED, LOW);  //LOWで点灯
  delay(500);
  digitalWrite(LED, HIGH);

  //--------------------------------------------------
  // 水分測定
  //--------------------------------------------------
  int16_t moisture_ch0 = -32768;
  int16_t moisture_ch1 = -32768;
  
  if(use_ads1115){
    moisture_ch0 = ads.readADC_SingleEnded(0);
    moisture_ch1 = ads.readADC_SingleEnded(1);
  }

  //--------------------------------------------------
  // センサーOFF（腐食防止）
  //--------------------------------------------------
  digitalWrite(PIN_LM393_1_VCC, LOW);
  digitalWrite(PIN_LM393_2_VCC, LOW);

  //--------------------------------------------------
  // 温度測定
  //--------------------------------------------------
  int16_t temp_ch0 = -32768;
  int16_t temp_ch1 = -32768;

  ds18b20.requestTemperatures();

  if (ds18b20.getDeviceCount() >= 1){
    temp_ch0 = (int16_t)lroundf(ds18b20.getTempC(tempSensor0) * 10.0f);
  }

  if (ds18b20.getDeviceCount() >= 2){
    temp_ch1 = (int16_t)lroundf(ds18b20.getTempC(tempSensor1) * 10.0f);
  }

  //--------------------------------------------------
  // 電池電圧
  //--------------------------------------------------
  uint16_t battery = 0;

  // 現在未使用(ダミー値)
  battery = 4500; // mV

  //--------------------------------------------------
  // 送信データ生成
  //--------------------------------------------------
  uint8_t txData[12];

  txData[0] = DEVICE_NO;
  txData[1] = status;

  txData[2] = battery & 0xFF;
  txData[3] = battery >> 8;

  // 水分量
  txData[4] = moisture_ch0 & 0xFF;
  txData[5] = moisture_ch0 >> 8;

  txData[6] = moisture_ch1 & 0xFF;
  txData[7] = moisture_ch1 >> 8;

  // 温度

  txData[8] = temp_ch0 & 0xFF;
  txData[9] = temp_ch0 >> 8;

  txData[10] = temp_ch1 & 0xFF;
  txData[11] = temp_ch1 >> 8;

  Serial.printf(
      "STATUS=0x%x BAT=%dmV moisture_ch0=%d moisture_ch1=%d temp_ch0=%.1fC temp_ch1=%.1fC\n",
      status, 
      battery,
      moisture_ch0,
      moisture_ch1,
      (float)temp_ch0/10.0,
      (float)temp_ch1/10.0);

  // Advertisingデータ作成
  NimBLEAdvertisementData advData;

  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);

  // Company ID = 0xFFFF（テスト用）
  std::string mfg;
  mfg.push_back(COMPANY_ID&0xff);   // Company ID LSB
  mfg.push_back((COMPANY_ID&0xff00)>>8);   // Company ID MSB
  mfg.append((char*)txData, sizeof(txData));

  advData.setManufacturerData(mfg);

  // Advertising開始
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->start();

  Serial.println("Advertising...");

  delay(3000);
  adv->stop();
  Serial.println("Stoped.");

    Serial.println("Go to Deep Sleep");

  esp_sleep_enable_timer_wakeup(
      DEEP_SLEEP_CYCLE_SEC * 1000000ULL); // 

  delay(100);

  esp_deep_sleep_start();

}

void loop() {
}
