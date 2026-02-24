/*
 * FreeRTOS Intel 8085 demo - Queue + Event Group Rendezvous
 *
 * Port of the TMS9900 demo pattern (main_queue.c):
 *   4 tasks:
 *     Manager (priority 2): generates 3 random work items, pushes to queue,
 *         then syncs at rendezvous with workers
 *     Worker A/B/C (priority 1): receive work from queue, delay for random
 *         ticks (simulating work), then sync at rendezvous
 *
 * When all 4 tasks reach the rendezvous, one "round" is complete.
 *
 * Verification via i8085-trace memory dump:
 *   0xFE00  ROUNDS   - completed round count (all 4 tasks synced)
 *   0xFE02  WORKER_A - worker A completion count
 *   0xFE04  WORKER_B - worker B completion count
 *   0xFE06  WORKER_C - worker C completion count
 *
 * Success: ROUNDS > 0, and WORKER_A == WORKER_B == WORKER_C == ROUNDS
 *
 * Run:
 *   i8085-trace --timer=65:30720 -n 2000000 -S -d 0xFE00:8 \
 *       build/freertos_eventgroup.bin
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

/* Memory-mapped markers */
#define MARKER_ROUNDS   ( *( volatile unsigned int * ) 0xFE00 )
#define MARKER_WORKER_A ( *( volatile unsigned int * ) 0xFE02 )
#define MARKER_WORKER_B ( *( volatile unsigned int * ) 0xFE04 )
#define MARKER_WORKER_C ( *( volatile unsigned int * ) 0xFE06 )

/* Event bits for rendezvous (4-way sync) */
#define WORKER_A_BIT    ( ( EventBits_t ) ( 1 << 0 ) )
#define WORKER_B_BIT    ( ( EventBits_t ) ( 1 << 1 ) )
#define WORKER_C_BIT    ( ( EventBits_t ) ( 1 << 2 ) )
#define MANAGER_BIT     ( ( EventBits_t ) ( 1 << 3 ) )
#define ALL_SYNC_BITS   ( WORKER_A_BIT | WORKER_B_BIT | WORKER_C_BIT | MANAGER_BIT )

/*-----------------------------------------------------------
 * 16-bit Galois LFSR (maximal period 65535)
 * Polynomial: x^16 + x^14 + x^13 + x^11 + 1
 *-----------------------------------------------------------*/
static unsigned int lfsr_state = 0xACE1;

static unsigned int lfsr_next( void )
{
    unsigned int lsb = lfsr_state & 1;
    lfsr_state >>= 1;
    if( lsb )
    {
        lfsr_state ^= 0xB400; /* taps at 16,14,13,11 */
    }
    return lfsr_state;
}

/* Random delay: 1-6 ticks (at 100Hz, that's 10-60ms) */
static unsigned int random_ticks( void )
{
    return 1 + ( lfsr_next() % 6 );
}

/*-----------------------------------------------------------
 * Static allocations
 *-----------------------------------------------------------*/

/* Tasks - 256 bytes stack each (i8085 needs more due to verbose codegen) */
static StaticTask_t xManager_TCB;
static StackType_t  xManager_Stack[ 256 ];

static StaticTask_t xWorkerA_TCB;
static StackType_t  xWorkerA_Stack[ 256 ];

static StaticTask_t xWorkerB_TCB;
static StackType_t  xWorkerB_Stack[ 256 ];

static StaticTask_t xWorkerC_TCB;
static StackType_t  xWorkerC_Stack[ 256 ];

static StaticTask_t xIdleTaskTCB;
static StackType_t  xIdleTaskStack[ 256 ];

/* Event group for rendezvous */
static StaticEventGroup_t xEventGroupBuffer;
static EventGroupHandle_t xEvents;

/* Queue: 8 slots of uint16_t work items (tick counts) */
static StaticQueue_t xQueueBuffer;
static unsigned char ucQueueStorage[ 8 * sizeof( unsigned int ) ];
static QueueHandle_t xWorkQueue;

/*-----------------------------------------------------------
 * Worker task parameters
 *-----------------------------------------------------------*/
typedef struct
{
    EventBits_t xMyBit;
    volatile unsigned int * pxMarker;
} WorkerParams_t;

static WorkerParams_t xWorkerA_Params = { WORKER_A_BIT, ( volatile unsigned int * ) 0xFE02 };
static WorkerParams_t xWorkerB_Params = { WORKER_B_BIT, ( volatile unsigned int * ) 0xFE04 };
static WorkerParams_t xWorkerC_Params = { WORKER_C_BIT, ( volatile unsigned int * ) 0xFE06 };

/*-----------------------------------------------------------
 * Worker task: receive work from queue, delay, rendezvous
 *-----------------------------------------------------------*/
void vWorkerTask( void * pvParameters )
{
    WorkerParams_t * pxParams = ( WorkerParams_t * ) pvParameters;
    unsigned int usTicks;

    for( ;; )
    {
        /* Block until a work item is available */
        if( xQueueReceive( xWorkQueue, &usTicks, portMAX_DELAY ) == pdTRUE )
        {
            /* Simulate work by delaying */
            vTaskDelay( ( TickType_t ) usTicks );

            /* Record completion */
            ( *pxParams->pxMarker )++;
        }

        /* Rendezvous: set our bit and wait for all participants */
        xEventGroupSync( xEvents,
                         pxParams->xMyBit,
                         ALL_SYNC_BITS,
                         portMAX_DELAY );
    }
}

/*-----------------------------------------------------------
 * Manager task: generate work, distribute via queue, rendezvous
 *-----------------------------------------------------------*/
void vManagerTask( void * pvParameters )
{
    ( void ) pvParameters;

    for( ;; )
    {
        /* Generate 3 random work items and push to queue */
        for( int i = 0; i < 3; i++ )
        {
            unsigned int usTicks = random_ticks();
            xQueueSend( xWorkQueue, &usTicks, portMAX_DELAY );
        }

        /* Rendezvous with all workers */
        xEventGroupSync( xEvents,
                         MANAGER_BIT,
                         ALL_SYNC_BITS,
                         portMAX_DELAY );

        /* All tasks synced - round complete */
        MARKER_ROUNDS++;
    }
}

/*-----------------------------------------------------------
 * Required by FreeRTOS static allocation
 *-----------------------------------------------------------*/
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE * pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = 256;
}

/*-----------------------------------------------------------
 * main
 *-----------------------------------------------------------*/
int main( void )
{
    /* Clear markers */
    MARKER_ROUNDS = 0;
    MARKER_WORKER_A = 0;
    MARKER_WORKER_B = 0;
    MARKER_WORKER_C = 0;

    /* Create event group for rendezvous */
    xEvents = xEventGroupCreateStatic( &xEventGroupBuffer );

    /* Create work queue: 8 items of sizeof(unsigned int) */
    xWorkQueue = xQueueCreateStatic( 8,
                                     sizeof( unsigned int ),
                                     ucQueueStorage,
                                     &xQueueBuffer );

    /* Create 3 worker tasks (priority 1) */
    xTaskCreateStatic( vWorkerTask, "WrkA", 256, &xWorkerA_Params, 1,
                       xWorkerA_Stack, &xWorkerA_TCB );

    xTaskCreateStatic( vWorkerTask, "WrkB", 256, &xWorkerB_Params, 1,
                       xWorkerB_Stack, &xWorkerB_TCB );

    xTaskCreateStatic( vWorkerTask, "WrkC", 256, &xWorkerC_Params, 1,
                       xWorkerC_Stack, &xWorkerC_TCB );

    /* Create manager task (priority 2 - higher) */
    xTaskCreateStatic( vManagerTask, "Mgr", 256, ( void * ) 0, 2,
                       xManager_Stack, &xManager_TCB );

    /* Start scheduler - never returns */
    vTaskStartScheduler();

    for( ;; )
    {
    }
}
