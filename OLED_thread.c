#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "xtimer.h"

#include "thread.h"
#include "fmt.h"

#include "xtimer.h"

#include "Message.h"

#include "periph/gpio.h"
#include "periph/i2c.h"
#include "u8g2.h"

/**
 * @brief   RIOT-OS pin maping of U8g2 pin numbers to RIOT-OS GPIO pins.
 * @note    To minimize the overhead, you can implement an alternative for
 *          u8x8_gpio_and_delay_riotos.
 *          The TTGO SSD1306 OLED screen uses I2C and has the reset pin
 *          at GPIO 16.
 */
static gpio_t pins[] = {
    [U8X8_PIN_RESET] = OLED_RESET_PIN     // See the board.h for the TTGO ESP32 Lora Oled pin reset definition
};

/**
 * @brief   Bit mapping to indicate which pins are set.
 */
static uint32_t pins_enabled = (
    (1 << U8X8_PIN_RESET)
);

u8g2_t u8g2;

/* For receiving data from the other threads. */
OLed_msg ol_in_msg;
msg_t m_in_msg;

static const uint8_t logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x3C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x1E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x70, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xF0, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x1E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xF0, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0xF8,
    0x30, 0x3C, 0x3F, 0xC0, 0x00, 0x0C, 0x77, 0xF0, 0x38, 0x7E, 0x3F, 0xC0,
    0x00, 0x7E, 0x73, 0xC0, 0x38, 0xE7, 0x06, 0x00, 0x00, 0xFC, 0x71, 0x00,
    0x38, 0xE3, 0x06, 0x00, 0x01, 0xF0, 0x70, 0x00, 0x38, 0xE3, 0x06, 0x00,
    0x01, 0xC0, 0x70, 0x00, 0x38, 0xE3, 0x06, 0x00, 0x03, 0x80, 0x70, 0xC0,
    0x38, 0xE3, 0x06, 0x00, 0x03, 0x80, 0x71, 0xE0, 0x38, 0xE3, 0x06, 0x00,
    0x03, 0x80, 0x70, 0xE0, 0x38, 0xE3, 0x06, 0x00, 0x03, 0x80, 0x70, 0xF0,
    0x38, 0xE3, 0x06, 0x00, 0x03, 0x80, 0x70, 0x70, 0x38, 0xE3, 0x06, 0x00,
    0x03, 0x80, 0xF0, 0x78, 0x38, 0xE3, 0x06, 0x00, 0x03, 0xC1, 0xE0, 0x3C,
    0x38, 0xE7, 0x06, 0x00, 0x01, 0xE3, 0xE0, 0x3C, 0x38, 0x7E, 0x06, 0x00,
    0x01, 0xFF, 0xC0, 0x1C, 0x30, 0x3C, 0x06, 0x00, 0x00, 0x7F, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

uint32_t screen = 0;

/*
 * Initialize the OLED screen.
 * Note that the OLED parameters come from the board definition.
 */

void OLed_Init(void) {
    puts("Initializing OLED");

    printf("Initializing I2C display at address 0x%02x.\n", OLED_I2C_ADDR);
    u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_riotos_hw_i2c, u8x8_gpio_and_delay_riotos);

    u8g2_SetPins(&u8g2, pins, pins_enabled);
    u8g2_SetDevice(&u8g2, I2C_DEV(0));
    u8g2_SetI2CAddress(&u8g2, OLED_I2C_ADDR);

    /* initialize the display */
    puts("Initializing display.");

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    puts("OLED initialized");
}

void TTN_screen(void) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 40 ,  "TTN Node");
}

void TTN_joining(OLed_msg *olmsg) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 10 ,  olmsg->str);
}

void TTN_joined(void) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 10 ,  "Joined!");
}

void TTN_sending(void) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 10 ,  "Sending...");    
}

void TTN_rxwindow(void) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 20 ,  "RX Window...");    
}

void TTN_done(void) {
    u8g2_SetFont( &u8g2,  u8g2_font_crox1cb_tf);
    u8g2_DrawStr( &u8g2, 5, 30 ,  "Done!");    
}

/*
 * OLED thread - Thread that controls the OLED display
 *
 */
void *OLED_thread(void *arg) 
{
    OLed_Init();

    while (1) {
        u8g2_FirstPage(&u8g2);

        do {
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_SetFont(&u8g2, u8g2_font_helvB12_tf);

            switch (screen) {
                case 0:
                    //u8g2_DrawStr(&u8g2, 12, 22, "THIS");
                    TTN_screen();
                    break;
                case 1:
                    //u8g2_DrawStr(&u8g2, 24, 22, "IS");
                    TTN_joining(&ol_in_msg);
                    break;
                case 2:
                    //u8g2_DrawBitmap(&u8g2, 0, 0, 8, 32, logo);
                    TTN_joined();
                    break;
                case 3:
                    TTN_sending();
                    break;
                case 4:
                    TTN_sending();
                    TTN_rxwindow();
                    break;
                case 5:
                    TTN_sending();
                    TTN_rxwindow();
                    TTN_done();
                    break;                   
            }
        } 
        while (u8g2_NextPage(&u8g2));

        /* show screen in next iteration */
        //screen = (screen + 1) % 3;

        puts("Waiting for message for OLED control");
        msg_receive( &m_in_msg);
        memcpy( &ol_in_msg , m_in_msg.content.ptr , sizeof(OLed_msg) );

        screen = ol_in_msg.cmd;
        printf("[OLED] -> %s\n", ol_in_msg.str );
        /* sleep a little */
        xtimer_sleep(1);
     
    }

    /* this should never be reached */
    return NULL;
}
