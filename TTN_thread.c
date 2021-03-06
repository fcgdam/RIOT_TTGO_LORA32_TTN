#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "xtimer.h"

#include "thread.h"
#include "fmt.h"
#include "periph/rtc.h"
#include "net/loramac.h"
#include "semtech_loramac.h"

#include "TTN_thread.h"
#include "OLED_thread.h"
#include "Message.h"

/* Messages are sent every 20s to respect the duty cycle on each channel */
#define PERIOD              (20U)

semtech_loramac_t loramac;

static const char *message = "This is RIOT!";
unsigned long frame = 0 ;

/* Information for OTAA activation */
uint8_t deveui[LORAMAC_DEVEUI_LEN] ;
uint8_t appeui[LORAMAC_APPEUI_LEN] ;
uint8_t appkey[LORAMAC_APPKEY_LEN];

/* Information for ABP activation */
uint8_t devaddr[LORAMAC_DEVADDR_LEN];
uint8_t appskey[LORAMAC_APPSKEY_LEN];
uint8_t nwkskey[LORAMAC_NWKSKEY_LEN];

// A message queue is declared globally for this thread.
#define MSG_QLEN 8
msg_t msg_queue[MSG_QLEN];

/*static void print_time(const char *label, const struct tm *time)
{
    printf("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
        time->tm_year + 1900,
        time->tm_mon + 1,
        time->tm_mday,
        time->tm_hour,
        time->tm_min,
        time->tm_sec);
}*/

/*
 * Message functions to send data to the OLED.
 *
 */

OLed_msg olmsg;
msg_t m_msg;

void OL_joining(void) {

    olmsg.cmd = 1;
    olmsg.x = 10;
    olmsg.y = 40;

    if ( nodeactivation ) 
        strcpy( olmsg.str , "Joining OTAA...");
    else
        strcpy( olmsg.str , "Joining ABP...");

    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid ); 
}

void OL_joined(void) {

    olmsg.cmd = 2;
    olmsg.x = 10;
    olmsg.y = 40;

    if ( nodeactivation ) 
    strcpy( olmsg.str , "Joined OTAA!");
    else
    strcpy( olmsg.str , "Joined ABP!");

    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid ); 
}

void OL_sending(void) {
    olmsg.cmd = 3;
    olmsg.x = 10;
    olmsg.y = 40;

    strcpy( olmsg.str , "Sending Data...");

    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid );  
}

void OL_rxwindow(void) {
    olmsg.cmd = 4;
    olmsg.x = 10;
    olmsg.y = 40;

    strcpy( olmsg.str , "Waiting Data...");

    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid );  
}

void OL_done(void) {
    olmsg.cmd = 5;
    olmsg.x = 10;
    olmsg.y = 40;

    strcpy( olmsg.str , "Done!");

    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid );  
}

void OL_sendframecnt( void ) {
    char buffer[64];
    
    sprintf( buffer , "Frame #: %ld " , frame );
    strcpy( olmsg.str, buffer);

    olmsg.cmd = 6;
    m_msg.content.ptr = &olmsg;
    msg_send( &m_msg , OLED_thread_pid );    
}


void rtc_cb(void *arg)
{
    (void) arg;
    printf("[RTC Alarm] Triggered!\n");
    msg_t triggermsg;
        
    msg_send(&triggermsg, TTN_thread_pid);
    //msg_send_to_self( &msg );
}

void _prepare_next_alarm(void)
{
    struct tm time;
    rtc_get_time(&time);
   
    //set initial alarm 
    time.tm_sec += PERIOD;
    
    mktime(&time);
    
    rtc_set_alarm(&time, rtc_cb, NULL);
}


void _send_message(void)
{
    uint8_t res;
    
    printf("[Sender Thread] Sending message: %s\n", message);
    OL_sending();
    /* The send call returns immediatly (is non blocking) */
    res = semtech_loramac_send(&loramac, (uint8_t *)message, strlen(message));
    
    switch (res) {
        case SEMTECH_LORAMAC_TX_OK:
            puts("Lorawan TX OK.");
            break;
        case SEMTECH_LORAMAC_TX_ERROR:
            puts("Lorawan TX failed.");
            break;            
        case SEMTECH_LORAMAC_BUSY:
            puts("Lorawan stack BUSY.");
            break;
        case SEMTECH_LORAMAC_NOT_JOINED:
            puts("Lorawan stack not joined.");
            break;
        case SEMTECH_LORAMAC_TX_SCHEDULE:
            puts("Lorawan stack TX scheduled.");
            break;
        default:
            printf("Lorawan send result: %d\n", res);
            break;           
    }
    /* Wait until the send cycle has completed */
    puts("Waiting for TX completion/rx data");
    OL_rxwindow();
    
    if ( (res = semtech_loramac_recv(&loramac)) == SEMTECH_LORAMAC_DATA_RECEIVED) {
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("------------------------------------------------------------\n");
        printf("Data received: %s, port: %d\n",    (char *)loramac.rx_data.payload, loramac.rx_data.port);
    }
    //printf(" ---------> [Sender Thread] Recv exit code: %d\n" , res);
    printf("[Sender Thread] Sending done.\n");
    
    OL_done();

    frame++;
    OL_sendframecnt();
}

/*
 * TTN Sender thread - Thread that sends periodically data to the Lorawan network.
 *
 */
void *TTN_thread(void *arg) 
{
    (void)   arg;
    uint8_t  joined = 0;
    msg_t    msg;
    
    msg_init_queue(msg_queue, MSG_QLEN);

    xtimer_sleep(2);   /* Wait for the console output to settle. */
    
    /* Create the Loramac LORAWAN stack. */
    semtech_loramac_init(&loramac);

    /* TTN sends uplink data on RX2 with SF9. 
       Without this no data is received back from TTN.
    */
    semtech_loramac_set_rx2_dr(&loramac , LORAMAC_DR_2);

    //semtech_loramac_set_tx_mode(&loramac ,  LORAMAC_TX_UNCNF );

    /* Convert identifiers and application key */
    fmt_hex_bytes(deveui, DEVEUI);
    fmt_hex_bytes(appeui, APPEUI);
    fmt_hex_bytes(appkey, APPKEY);

    fmt_hex_bytes(devaddr, DEVADDR);
    fmt_hex_bytes(appskey, APPSKEY);
    fmt_hex_bytes(nwkskey, NWKSKEY);

    /* Initialize the loramac stack */
    if ( nodeactivation ) {
        semtech_loramac_set_deveui(&loramac, deveui);
        semtech_loramac_set_appeui(&loramac, appeui);
        semtech_loramac_set_appkey(&loramac, appkey);
    } else {
        semtech_loramac_set_devaddr(&loramac, devaddr);
        semtech_loramac_set_appskey(&loramac, appskey);
        semtech_loramac_set_nwkskey(&loramac, nwkskey);
    }

    /* Use a medium datarate, e.g. BW125/SF9 in EU868 */
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_3); 
           
    OL_joining();  // Update OLed

    while ( !joined ) {
        /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
         * generated device address and to get the network and application session
         * keys.
         */
        joined = 1;   /* Let's assume success for now, and it fails we reset the variable. */
        if ( nodeactivation ) {
            puts("[Sender Thread] Starting join procedure...");
            if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
                puts("Join procedure failed");
                joined = 0;
            }
        } else {
            puts("[Sender Thread] Starting ABP node activation...");
            if (semtech_loramac_join(&loramac, LORAMAC_JOIN_ABP) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
                puts("Join procedure failed");
                joined = 0;
            }
        }

        if ( !joined ) {
            /* Should only reach this if the join failed. 
                Let's sleep for a while and try again. */
            puts("[Sender Thread] Joined failed... Sleeping 60s...");
            xtimer_sleep(60);
            puts("[Sender Thread] Let's try again to join...");
        }
    }
    
    puts("[Sender Thread] Join/ABP procedure succeeded"); 
    OL_joined();
    
    /* We are Joined so we can loop now forever. */
    while ( 1 ) {
        
        /* Trigger the message send */
        //puts("[Sender Thread] Sending message...");
        _send_message();

        /* Schedule the next wake-up alarm */
        //puts("[Sender Thread] Setup next alarm...");
        _prepare_next_alarm();        

        puts("[Sender Thread] Waiting for trigger message...");
        msg_receive(&msg);  
         
       
    }

    /* this should never be reached */
    return NULL;
}
