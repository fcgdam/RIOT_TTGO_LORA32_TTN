#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "msg.h"
#include "thread.h"
#include "fmt.h"

#include "periph/rtc.h"

#include "TTN_thread.h"
#include "OLED_thread.h"
/*
 *  TTN_Thread - TTN Thread supporting variables.
 * 
 */
#define TTN_THREAD_PRIORITY         (THREAD_PRIORITY_MAIN - 2)
kernel_pid_t TTN_thread_pid;
static char TTN_thread_stack[THREAD_STACKSIZE_MAIN / 2];

/*
 * OLED_Thread - OLED Thread supporting variables.
 * 
 */
#define OLED_THREAD_PRIORITY         (THREAD_PRIORITY_MAIN - 1)
kernel_pid_t OLED_thread_pid;
static char OLED_thread_stack[THREAD_STACKSIZE_MAIN / 2];


uint8_t nodeactivation = NODEACTIVATION;

int main(void)
{
    puts("LoRaWAN Class A low-power application");
    puts("=====================================");
    printf(" -> Node activation by: ");
    if ( nodeactivation ) {
        puts("OTAA");
    } else {
        puts("ABP");
    }

     /* Start the TTN thread */
    TTN_thread_pid = thread_create( TTN_thread_stack, sizeof( TTN_thread_stack ),  TTN_THREAD_PRIORITY, 0,  TTN_thread , NULL, "TTN");
    
    /* Start the OLED thread */
    OLED_thread_pid = thread_create( OLED_thread_stack, sizeof( OLED_thread_stack ),  OLED_THREAD_PRIORITY, 0,  OLED_thread , NULL, "OLED");

    return 0;
}

