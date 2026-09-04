#pragma once
#include <string.h>
#include "RobotArmCommon.h"

// ============================================================================
// Giao thức UART giữa ESP32 Wroom (Main) <-> ESP32 C3 Super Mini (Sensor)
//
// Frame: [SYNC1][SYNC2][LEN][TYPE][PAYLOAD...LEN bytes...][CRC8]
//   - SYNC1/SYNC2 : 0xAA 0x55, đánh dấu đầu frame để dễ resync khi mất byte.
//   - LEN         : số byte payload (không tính TYPE/CRC).
//   - TYPE        : UartMsgType.
//   - CRC8        : Dallas/Maxim (poly 0x31) tính trên [TYPE][PAYLOAD...].
// ============================================================================

static const uint8_t UART_SYNC1 = 0xAA;
static const uint8_t UART_SYNC2 = 0x55;
static const uint8_t UART_MAX_PAYLOAD = 64;

enum UartMsgType : uint8_t {
  UART_MSG_ANGLE_REPORT = 0x01,  // C3 -> Wroom: góc 4 trục
  UART_MSG_PING = 0x02,          // Wroom -> C3: kiểm tra sống
  UART_MSG_PONG = 0x03           // C3 -> Wroom: phản hồi ping
};

// Payload cho UART_MSG_ANGLE_REPORT.
// angleDeg: góc đọc trực tiếp từ AS5600 (độ, 0-360), theo thứ tự X,Y,Z,A
// (channel TCA9548A 0..3). sensorOk: bit i = 1 nếu AS5600 kênh i đọc hợp lệ
// (phát hiện được từ trường/mức tín hiệu magnitude/AGC hợp lý).
struct __attribute__((packed)) AngleReportPayload {
  uint32_t seq;
  float angleDeg[AXIS_STEPPER_COUNT];
  uint8_t sensorOk;
};

inline uint8_t uartCrc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

// Đóng gói 1 frame vào outBuf, trả về tổng số byte đã ghi (0 nếu payloadLen
// vượt quá UART_MAX_PAYLOAD). outBuf phải có tối thiểu payloadLen + 5 byte.
inline uint8_t uartEncodeFrame(uint8_t type, const uint8_t *payload, uint8_t payloadLen, uint8_t *outBuf) {
  if (payloadLen > UART_MAX_PAYLOAD) return 0;
  uint8_t crcBuf[UART_MAX_PAYLOAD + 1];
  crcBuf[0] = type;
  if (payloadLen > 0) memcpy(&crcBuf[1], payload, payloadLen);

  uint8_t idx = 0;
  outBuf[idx++] = UART_SYNC1;
  outBuf[idx++] = UART_SYNC2;
  outBuf[idx++] = payloadLen;
  outBuf[idx++] = type;
  if (payloadLen > 0) {
    memcpy(&outBuf[idx], payload, payloadLen);
    idx += payloadLen;
  }
  outBuf[idx++] = uartCrc8(crcBuf, (uint8_t)(payloadLen + 1));
  return idx;
}

// Bộ giải mã frame theo kiểu streaming: nạp từng byte nhận được từ Serial,
// feed() trả về true khi vừa hoàn thành 1 frame hợp lệ (CRC đúng); khi đó
// type/payload/payloadLen chứa nội dung frame để xử lý ngay trong lời gọi đó.
class UartFrameDecoder {
 public:
  bool feed(uint8_t byteIn) {
    switch (state_) {
      case WAIT_SYNC1:
        if (byteIn == UART_SYNC1) state_ = WAIT_SYNC2;
        break;
      case WAIT_SYNC2:
        state_ = (byteIn == UART_SYNC2) ? WAIT_LEN : WAIT_SYNC1;
        break;
      case WAIT_LEN:
        if (byteIn > UART_MAX_PAYLOAD) {
          state_ = WAIT_SYNC1;
        } else {
          payloadLen = byteIn;
          state_ = WAIT_TYPE;
        }
        break;
      case WAIT_TYPE:
        type = byteIn;
        rxIdx_ = 0;
        state_ = (payloadLen == 0) ? WAIT_CRC : WAIT_PAYLOAD;
        break;
      case WAIT_PAYLOAD:
        payload[rxIdx_++] = byteIn;
        if (rxIdx_ >= payloadLen) state_ = WAIT_CRC;
        break;
      case WAIT_CRC: {
        uint8_t crcBuf[UART_MAX_PAYLOAD + 1];
        crcBuf[0] = type;
        if (payloadLen > 0) memcpy(&crcBuf[1], payload, payloadLen);
        bool ok = (uartCrc8(crcBuf, (uint8_t)(payloadLen + 1)) == byteIn);
        state_ = WAIT_SYNC1;
        return ok;
      }
    }
    return false;
  }

  uint8_t type = 0;
  uint8_t payload[UART_MAX_PAYLOAD] = {0};
  uint8_t payloadLen = 0;

 private:
  enum State { WAIT_SYNC1, WAIT_SYNC2, WAIT_LEN, WAIT_TYPE, WAIT_PAYLOAD, WAIT_CRC };
  State state_ = WAIT_SYNC1;
  uint8_t rxIdx_ = 0;
};
