#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  uart_dev_t uart;
  timer_dev_t timer;
  uint32_t frame_count;
} chip_state_t;

static void on_timer_done(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint8_t frame[30];
  
  // Header
  frame[0] = 0xAA;
  frame[1] = 0xFF;
  frame[2] = 0x03;
  frame[3] = 0x00;

  float t = chip->frame_count * 0.1f; // 10Hz frames
  
  // Target 1: Circling (Anchor 1)
  int16_t x1 = 1000 + (int16_t)(500 * cos(t));
  int16_t y1 = 2000 + (int16_t)(500 * sin(t));
  
  // Target 2: Static (Anchor 2)
  int16_t x2 = -1500;
  int16_t y2 = 3000;
  
  // Target 3: Inactive
  int16_t x3 = 0;
  int16_t y3 = 0;

  // Encode values (Bit 15 is sign bit: 1=Pos, 0=Neg)
  auto encode = [](int16_t v) -> uint16_t {
    uint16_t mag = abs(v) & 0x7FFF;
    return (v >= 0) ? (mag | 0x8000) : mag;
  };

  uint16_t ex1 = encode(x1);
  uint16_t ey1 = encode(y1);
  uint16_t ex2 = encode(x2);
  uint16_t ey2 = encode(y2);
  uint16_t ex3 = encode(x3);
  uint16_t ey3 = encode(y3);

  // Payload (Targets 1, 2, 3)
  // Target 1
  frame[4] = ex1 & 0xFF; frame[5] = (ex1 >> 8) & 0xFF;
  frame[6] = ey1 & 0xFF; frame[7] = (ey1 >> 8) & 0xFF;
  frame[8] = 0x00; frame[9] = 0x80; // 0 speed
  frame[10] = 0x00; frame[11] = 0x00; // Res

  // Target 2
  frame[12] = ex2 & 0xFF; frame[13] = (ex2 >> 8) & 0xFF;
  frame[14] = ey2 & 0xFF; frame[15] = (ey2 >> 8) & 0xFF;
  frame[16] = 0x00; frame[17] = 0x80;
  frame[18] = 0x00; frame[19] = 0x00;

  // Target 3
  frame[20] = ex3 & 0xFF; frame[21] = (ex3 >> 8) & 0xFF;
  frame[22] = ey3 & 0xFF; frame[23] = (ey3 >> 8) & 0xFF;
  frame[24] = 0x00; frame[25] = 0x80;
  frame[26] = 0x00; frame[27] = 0x00;

  // Tail
  frame[28] = 0x55;
  frame[29] = 0xCC;

  uart_write(chip->uart, frame, 30);
  chip->frame_count++;
}

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->frame_count = 0;

  const uart_config_t uart_config = {
    .tx = pin_init("TX", PULLUP),
    .rx = pin_init("RX", INPUT),
    .baud_rate = 256000,
  };
  chip->uart = uart_init(&uart_config);

  const timer_config_t timer_config = {
    .callback = on_timer_done,
    .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 100000, true); // 100ms = 10Hz
}
