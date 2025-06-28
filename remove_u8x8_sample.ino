#include <Arduino.h>

// LoRaモジュールとの通信に使うバッファ
static char recv_buf[512];
static bool is_exist = false;

// ATコマンド送信＆応答チェック関数
static int at_send_check_response(const char *p_ack, int timeout_ms, const char *p_cmd, ...) {
    int ch = 0;
    int index = 0;
    int startMillis = 0;
    va_list args;

    memset(recv_buf, 0, sizeof(recv_buf));
    va_start(args, p_cmd);
    char cmd_buf[256];
    vsnprintf(cmd_buf, sizeof(cmd_buf), p_cmd, args);
    va_end(args);

    Serial1.print(cmd_buf);
    Serial.print("[SEND] ");
    Serial.print(cmd_buf);

    delay(200);
    startMillis = millis();

    if (p_ack == NULL) return 0;

    do {
        while (Serial1.available() > 0) {
            ch = Serial1.read();
            recv_buf[index++] = ch;
            Serial.write(ch);
            delay(2);
        }

        if (strstr(recv_buf, p_ack) != NULL) {
            return 1;
        }

    } while (millis() - startMillis < timeout_ms);
    return 0;
}

// LoRaモジュールからの受信データをパースする関数
static int recv_parse(void) {
    char ch;
    int index = 0;
    memset(recv_buf, 0, sizeof(recv_buf));

    while (Serial1.available() > 0) {
        ch = Serial1.read();
        recv_buf[index++] = ch;
        Serial.write(ch);
        delay(2);
    }

    if (index) {
        char *p_start = NULL;
        char data[32] = {0};
        int rssi = 0, snr = 0;

        p_start = strstr(recv_buf, "+TEST: RX \"5345454544");
        if (p_start) {
            p_start = strstr(recv_buf, "5345454544");
            if (p_start && sscanf(p_start, "5345454544%4s", data) == 1) {
                Serial.print("RX: 0x");
                Serial.println(data);
            }

            p_start = strstr(recv_buf, "RSSI:");
            if (p_start && sscanf(p_start, "RSSI:%d,", &rssi) == 1) {
                Serial.print("RSSI: ");
                Serial.println(rssi);
            }

            p_start = strstr(recv_buf, "SNR:");
            if (p_start && sscanf(p_start, "SNR:%d", &snr) == 1) {
                Serial.print("SNR: ");
                Serial.println(snr);
            }
            return 1;
        }
    }
    return 0;
}

// LoRa受信処理
static int node_recv(uint32_t timeout_ms) {
    at_send_check_response("+TEST: RXLRPKT", 1000, "AT+TEST=RXLRPKT\r\n");
    int startMillis = millis();
    do {
        if (recv_parse()) {
            return 1;
        }
    } while (millis() - startMillis < timeout_ms);
    return 0;
}

// LoRa送信処理
static int node_send(void) {
    static uint16_t count = 0;
    int ret = 0;
    char data[32], cmd[128];

    snprintf(data, sizeof(data), "%04X", count);
    snprintf(cmd, sizeof(cmd), "AT+TEST=TXLRPKT,\"5345454544%s\"\r\n", data);

    Serial.print("TX: 0x");
    Serial.println(data);

    ret = at_send_check_response("TX DONE", 2000, cmd);
    if (ret == 1) {
        count++;
        Serial.println("送信成功！");
    } else {
        Serial.println("送信失敗！");
    }
    return ret;
}

// スレーブ：受信後に送信
static void node_recv_then_send(uint32_t timeout) {
    if (node_recv(timeout)) {
        delay(100);
        node_send();
    } else {
        Serial.println("受信タイムアウト");
    }
}

// マスター：送信後に受信
static void node_send_then_recv(uint32_t timeout) {
    if (node_send()) {
        if (!node_recv(timeout)) {
            Serial.println("受信タイムアウト！");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 16, 17);  // RX=GPIO16, TX=GPIO17 (必要に応じて変更)
    delay(500);
    Serial.println("ピンポン通信開始！");

    if (at_send_check_response("+AT: OK", 1000, "AT\r\n")) {
        is_exist = true;
        at_send_check_response("+MODE: TEST", 1000, "AT+MODE=TEST\r\n");
        at_send_check_response("+TEST: RFCFG", 1000,
            "AT+TEST=RFCFG,866,SF12,125,12,15,14,ON,OFF,OFF\r\n");

#ifdef NODE_SLAVE
        Serial.println("[NODE] スレーブモード");
#else
        Serial.println("[NODE] マスターモード");
#endif
    } else {
        is_exist = false;
        Serial.println("E5モジュールが見つかりません。");
    }
}

void loop() {
    if (is_exist) {
#ifdef NODE_SLAVE
        node_recv_then_send(2000);
#else
        node_send_then_recv(2000);
        delay(3000);
#endif
    }
}
