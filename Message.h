#ifndef _MESSAGE_H_
#define _MESSAGE_H_

typedef struct {
  uint8_t cmd;
  uint8_t x,y;
  char str[64];
} OLed_msg, *pOLed_msg;

#endif
