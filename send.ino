#include <HardwareSerial.h>

HardwareSerial loraSerial(1);

void sendAT(String cmd, int wait = 500) {
  loraSerial.println(cmd);
  delay(wait);
  while (loraSerial.available()) {
    Serial.println("[TX Wio-E5] " + loraSerial.readStringUntil('\n'));
  }
}

void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  delay(2000);
  Serial.println("Transmitter Setup");

  // P2Pモード設定
  sendAT("AT+MODE=TEST");
  // 日本の920MHz帯用設定
  sendAT("AT+TEST=RFCFG,923200000,12,125,17,8,5,ON");
  
  Serial.println("Transmitter Ready");
}

void loop() {
  static int counter = 0;
  String message = "Hello " + String(counter++);
  
  sendAT("AT+TEST=TXLRSTR,\"" + message + "\"");
  Serial.println("Sent: " + message);
  
  delay(2000);  // 2秒ごとに送信
}
