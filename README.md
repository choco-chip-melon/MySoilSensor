# MySoilSensor

ESP32C3を使用したBLE対応土壌センサーです。

家庭菜園向けに、

- 土壌温度（DS18B20 ×2）
- 土壌水分量（LM393 + ADS1115 ×2）

を測定し、BLE Advertisementで送信します。

測定終了後はDeep Sleepへ移行するため、乾電池で長期間動作できます。

## 概要
詳細はQiitaの記事を参照してください。

- オリジナル土壌センサーの作成（ハードウェア編）
- オリジナル土壌センサーの作成（ソフトウェア編）
- PythonによるBLE受信・SQLite保存


## 動作環境

- Seeed Studio XIAO ESP32C3
- Arduino IDE 2.x
- ESP32 Arduino Core

## 使用部品

|部品|数量|
|---|---:|
|Seeed Studio XIAO ESP32C3|1|
|ADS1115|1|
|DS18B20（防水）|2|
|LM393 土壌水分センサー|2|

## 使用ライブラリ

Arduinoライブラリマネージャから以下をインストールしてください。

- OneWire
- DallasTemperature
- Adafruit ADS1X15
- NimBLE-Arduino

## 機能

- DS18B20による土壌温度測定
- ADS1115による土壌水分測定
- BLE Advertisement送信
- Deep Sleepによる省電力動作
- GPIOによる土壌水分センサー電源制御

## 配線

|ESP32C3|接続先|
|---|---|
|GPIO20|DS18B20|
|GPIO3|ADS1115 SDA|
|GPIO4|ADS1115 SCL|
|GPIO0|水分センサー電源①|
|GPIO1|水分センサー電源②|
|ADS1115 CH0|水分センサー①|
|ADS1115 CH1|水分センサー②|

## ファイル構成

```
MySoilSensor.ino
```

## ソースについて
Seeed Studio XIAO ESP32C3ではなく、ESP32 C3 Super Miniの場合は以下のコメントを解除しdefine定義を有効にする。
```
#define SUPER_MINI
```

## ライセンス

MIT License

