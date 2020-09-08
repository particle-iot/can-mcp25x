/**
 * @file test.cpp
 * @author Ed Ablan
 * @version 1.0
 * @date 06/25/2020
 *
 * @brief test send for can-mcp25x library
 *
 * @details send stuff over can
 *
 * @copyright Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 */

#include <mcp_can.h>
#include <SPI.h>

#define APP_CONFIG_UNUSED (-1)
#define CAN_CS_PIN A3
#define CAN_INT_PIN A1
#define CAN_RST_PIN A2
#define BOOST_EN_PIN D6
#define CAN_INT_PIN A1
#define CAN_SPEED CAN_250KBPS
#define CAN_CLOCK MCP_20MHZ
#define CAN_STBY_PIN APP_CONFIG_UNUSED

const char SLEEP_CMD[] = "SLEEP";

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t  data[8];
} app_msg_log_t;

uint16_t pin_int;
app_msg_log_t msg;
bool is_sleep = false;

MCP_CAN CAN(CAN_CS_PIN);                                    // Set CS pin

void can_boost_enable(bool enable)
{
    digitalWrite(BOOST_EN_PIN, enable ? HIGH : LOW);
}

bool can_init(uint8_t cs_pin, uint8_t int_pin, uint8_t speed, uint8_t clock)
{
    pinMode(BOOST_EN_PIN, OUTPUT);
    can_boost_enable(true);

    // CAN peripherial config
    pinMode(CAN_RST_PIN, OUTPUT);
    if(CAN_STBY_PIN != APP_CONFIG_UNUSED)
    {
        pinMode(CAN_STBY_PIN, OUTPUT);
        digitalWrite(CAN_STBY_PIN, LOW);
    }

    digitalWrite(CAN_RST_PIN, LOW);
    delay(50);
    digitalWrite(CAN_RST_PIN, HIGH);

    if (CAN.begin(MCP_RX_ANY, speed, clock) != CAN_OK)
    {
        Serial.printf("CAN initial failed");
        return false;
    }

    // Set NORMAL mode
    if(CAN.setMode(MODE_NORMAL) == MCP2515_OK) {
        Serial.printf("CAN mode set");
    }
    else {
      Serial.printf("CAN mode fail");
      return false;
    }


    CAN.getCANStatus();
    pinMode(int_pin, INPUT);
    pin_int = int_pin;

    return true;
}

bool can_sleep(uint32_t timeout_ms)
{
    uint8_t  op_mode = 0xFF;
    uint32_t try_time = 0;

    if(CAN_STBY_PIN != APP_CONFIG_UNUSED) {
        digitalWrite(CAN_STBY_PIN, HIGH);
    }

    while(1) {
        // Set sleep mode
        CAN.setMode(MODE_SLEEP);

        // check CAN Operation Mode
        op_mode = CAN.getCANStatus() & 0xE0;

        if((op_mode == MODE_SLEEP) || (try_time >= timeout_ms)) {
            break;
        }
        try_time += 10;
        delay(10);
    }
    return (op_mode == MODE_SLEEP);
}

void setup()
{
    Serial.begin(9600);

    if(can_init(CAN_CS_PIN, CAN_INT_PIN, CAN_SPEED, CAN_CLOCK)) {
        Serial.printf("CAN BUS Shield init ok!");
    }
    else {
        Serial.printf("CAN BUS Shield init failed. Stop program!");
        delay(1000);
        while (1);
    }
}

unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
void loop()
{
    static uint8_t ret = CAN_FAIL;
    if(!is_sleep) {
        // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
        stmp[7] = stmp[7] + 1;
        if (stmp[7] == 100) {
            stmp[7] = 0;
            stmp[6] = stmp[6] + 1;

            if (stmp[6] == 100) {
                stmp[6] = 0;
                stmp[5] = stmp[6] + 1;
            }
        }
        if(CAN.sendMsgBuf(0x00, 0, 8, stmp) == CAN_OK) {
            Serial.printf("Sent CAN message\r\n");
        }
        else {
            Serial.printf("Failed CAN message\r\n");
        }
        delay(1000);                       // send data per 100ms

        if(!digitalRead(pin_int)) {
            ret = CAN.readMsgBufID(&msg.id, &msg.len, msg.data);
            if (ret == CAN_OK) {
                if(strncmp((char*)msg.data, SLEEP_CMD, strlen(SLEEP_CMD)) == 0) {
                    if(can_sleep(1000)) {
                        Serial.printf("CAN module put to sleep\r\n");
                        is_sleep = true;
                    }
                }
                else {
                    Serial.printf("Can Msg Received: ");
                    for(int i = 0; i < msg.len; i++) {
                        Serial.printf("%X ", msg.data[i]);
                    }
                }
                Serial.printf("\r\n");
            }
        }
    }
}
