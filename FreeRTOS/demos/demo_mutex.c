/*
 * FreeRTOS Intel 8085 demo - Mutex contention test
 *
 * 3 tasks at equal priority compete for a shared counter protected by
 * a mutex. Each task takes the mutex, delays for a random number of
 * ticks (simulating work), increments the counter, and gives the mutex.
 *
 * Markers:
 *   0xFE00: SHARED_COUNTER (mutex-protected)
 *   0xFE02: TASK_A_COUNT
 *   0xFE04: TASK_B_COUNT
 *   0xFE06: TASK_C_COUNT
 *   0xFE08: ROUNDS (total mutex acquisitions across all tasks)
 *
 * Success: SHARED_COUNTER == TASK_A + TASK_B + TASK_C (no corruption)
 *          and all per-task counts are > 0 (all tasks got the mutex)
 *
 * Run:
 *   i8085-trace --timer=65:30720 -n 2000000 -S -d 0xFE00:10 \
 *       build/freertos_mutex.bin
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Memory-mapped markers */
#define MARKER_SHARED   ( *( volatile unsigned int * ) 0xFE00 )
#define MARKER_TASK_A   ( *( volatile unsigned int * ) 0xFE02 )
#define MARKER_TASK_B   ( *( volatile unsigned int * ) 0xFE04 )
#define MARKER_TASK_C   ( *( volatile unsigned int * ) 0xFE06 )
#define MARKER_ROUNDS   ( *( volatile unsigned int * ) 0xFE08 )

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

/* Random delay: 1-4 ticks (at 100Hz, that's 10-40ms) */
static unsigned int random_ticks( void )
{
    return 1 + ( lfsr_next() % 4 );
}

/*-----------------------------------------------------------
 * Static allocations
 *-----------------------------------------------------------*/

/* Tasks - 256 bytes stack each */
static StaticTask_t xTaskA_TCB;
static StackType_t  xTaskA_Stack[ 256 ];

static StaticTask_t xTaskB_TCB;
static StackType_t  xTaskB_Stack[ 256 ];

static StaticTask_t xTaskC_TCB;
static StackType_t  xTaskC_Stack[ 256 ];

static StaticTask_t xIdleTaskTCB;
static StackType_t  xIdleTaskStack[ 256 ];

/* Mutex */
static StaticSemaphore_t xMutexBuffer;
static SemaphoreHandle_t xMutex;

/*-----------------------------------------------------------
 * Worker task parameters
 *-----------------------------------------------------------*/
typedef struct
{
    volatile unsigned int * pxMarker;
} WorkerParams_t;

static WorkerParams_t xTaskA_Params = { ( volatile unsigned int * ) 0xFE02 };
static WorkerParams_t xTaskB_Params = { ( volatile unsigned int * ) 0xFE04 };
static WorkerParams_t xTaskC_Params = { ( volatile unsigned int * ) 0xFE06 };

/*-----------------------------------------------------------
 * Worker task: take mutex, delay, increment shared counter, give mutex
 *-----------------------------------------------------------*/
void vMutexWorker( void * pvParameters )
{
    WorkerParams_t * pxParams = ( WorkerParams_t * ) pvParameters;

    for( ;; )
    {
        /* Take the mutex - block indefinitely */
        if( xSemaphoreTake( xMutex, portMAX_DELAY ) == pdTRUE )
        {
            /* Read shared counter (inside critical section) */
            unsigned int uCount = MARKER_SHARED;

            /* Simulate work while holding the mutex */
            vTaskDelay( random_ticks() );

            /* Increment and write back (proves no corruption) */
            MARKER_SHARED = uCount + 1;

            /* Give back the mutex */
            xSemaphoreGive( xMutex );

            /* Record per-task count (outside mutex - no contention) */
            ( *pxParams->pxMarker )++;

            /* Record total rounds */
            MARKER_ROUNDS++;
        }
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
    MARKER_SHARED = 0;
    MARKER_TASK_A = 0;
    MARKER_TASK_B = 0;
    MARKER_TASK_C = 0;
    MARKER_ROUNDS = 0;

    /* Create mutex (static) */
    xMutex = xSemaphoreCreateMutexStatic( &xMutexBuffer );

    /* Create 3 worker tasks at equal priority (time-sliced) */
    xTaskCreateStatic( vMutexWorker, "MtxA", 256, &xTaskA_Params, 1,
                       xTaskA_Stack, &xTaskA_TCB );

    xTaskCreateStatic( vMutexWorker, "MtxB", 256, &xTaskB_Params, 1,
                       xTaskB_Stack, &xTaskB_TCB );

    xTaskCreateStatic( vMutexWorker, "MtxC", 256, &xTaskC_Params, 1,
                       xTaskC_Stack, &xTaskC_TCB );

    /* Start scheduler - never returns */
    vTaskStartScheduler();

    for( ;; )
    {
    }
}
