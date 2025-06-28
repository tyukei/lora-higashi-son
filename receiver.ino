#include <HardwareSerial.h>

HardwareSerial loraSerial(1);

void sendAT(String cmd, int wait = 500) {
  loraSerial.println(cmd);
  delay(wait);
  while (loraSerial.available()) {
    String response = loraSerial.readStringUntil('\n');
    Serial.println("[RX Wio-E5] " + response);
    
    // 受信データの解析
    if (response.indexOf("+TEST: RX") != -1) {
      Serial.println(">>> Data Received! <<<");
    }
  }
}

void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  delay(2000);
  Serial.println("Receiver Setup");

  // P2Pモード設定
  sendAT("AT+MODE=TEST");
  // 日本の920MHz帯用設定（送信側と同じ設定）
  sendAT("AT+TEST=RFCFG,923200000,12,125,17,8,5,ON");
  
  // 連続受信モード開始
  sendAT("AT+TEST=RXLRPKT");
  
  Serial.println("Receiver Ready - Listening...");
}

void loop() {
  // 受信データをチェック
  while (loraSerial.available()) {
    String response = loraSerial.readStringUntil('\n');
    Serial.println("[RX] " + response);
    
    if (response.indexOf("+TEST: RX") != -1) {
      Serial.println(">>> Message Received! <<<");
      // 次の受信のために再度受信モードに
      sendAT("AT+TEST=RXLRPKT");
    }
  }
  
  delay(10);
}
