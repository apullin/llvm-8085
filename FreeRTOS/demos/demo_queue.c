/*
 * FreeRTOS Intel 8085 demo - Queue-based inter-task communication.
 *
 * Producer task sends incrementing values through a queue.
 * Consumer task reads values and writes running sum to a marker.
 *
 * Run with i8085-trace:
 *   i8085-trace --timer=65:30720 -n 500000 -S -d 0xFE00:8 \
 *       build/freertos_queue.bin
 *
 * Success: MARKER_SENT (0xFE00) shows items sent,
 *          MARKER_RECV (0xFE02) shows items received,
 *          MARKER_SUM  (0xFE04) shows running sum of received values.
 *          All three should be non-zero and SENT == RECV.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Memory-mapped markers */
#define MARKER_SENT  ( *( volatile unsigned int * ) 0xFE00 )
#define MARKER_RECV  ( *( volatile unsigned int * ) 0xFE02 )
#define MARKER_SUM   ( *( volatile unsigned int * ) 0xFE04 )

/* Queue: holds up to 4 unsigned int items */
#define QUEUE_LENGTH   4
#define ITEM_SIZE      sizeof( unsigned int )

static StaticQueue_t xQueueBuffer;
static unsigned char  ucQueueStorage[ QUEUE_LENGTH * ITEM_SIZE ];
static QueueHandle_t  xQueue;

/* Task storage */
static StaticTask_t xProducerTCB;
static StackType_t  xProducerStack[ configMINIMAL_STACK_SIZE ];

static StaticTask_t xConsumerTCB;
static StackType_t  xConsumerStack[ configMINIMAL_STACK_SIZE ];

/* Idle task storage */
static StaticTask_t xIdleTaskTCB;
static StackType_t  xIdleTaskStack[ configMINIMAL_STACK_SIZE ];

void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE * pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------
 * Producer - sends incrementing values into the queue
 *-----------------------------------------------------------*/
void vProducer( void * pvParameters )
{
    unsigned int uValue = 1;
    ( void ) pvParameters;

    for( ;; )
    {
        /* Block up to 10 ticks waiting for space */
        if( xQueueSend( xQueue, &uValue, 10 ) == pdPASS )
        {
            MARKER_SENT++;
            uValue++;
        }
    }
}

/*-----------------------------------------------------------
 * Consumer - reads values, accumulates running sum
 *-----------------------------------------------------------*/
void vConsumer( void * pvParameters )
{
    unsigned int uReceived;
    ( void ) pvParameters;

    for( ;; )
    {
        /* Block up to 10 ticks waiting for data */
        if( xQueueReceive( xQueue, &uReceived, 10 ) == pdPASS )
        {
            MARKER_RECV++;
            MARKER_SUM += uReceived;
        }
    }
}

/*-----------------------------------------------------------
 * main
 *-----------------------------------------------------------*/
int main( void )
{
    MARKER_SENT = 0;
    MARKER_RECV = 0;
    MARKER_SUM  = 0;

    /* Create the queue */
    xQueue = xQueueCreateStatic( QUEUE_LENGTH, ITEM_SIZE,
                                  ucQueueStorage, &xQueueBuffer );

    /* Producer at priority 1, Consumer at priority 2 (higher) */
    xTaskCreateStatic( vProducer, "Prod", configMINIMAL_STACK_SIZE,
                       ( void * ) 0, 1,
                       xProducerStack, &xProducerTCB );

    xTaskCreateStatic( vConsumer, "Cons", configMINIMAL_STACK_SIZE,
                       ( void * ) 0, 2,
                       xConsumerStack, &xConsumerTCB );

    vTaskStartScheduler();

    for( ;; ) { }
    return 0;
}
