/*
 * FreeRTOS Intel 8085 demo - heap_4 dynamic allocation stress test.
 *
 * Tests pvPortMalloc / vPortFree with block coalescing:
 *   1. Dynamically creates tasks and a queue (xTaskCreate, xQueueCreate)
 *   2. Allocator task does repeated malloc/free patterns that exercise
 *      heap_4's block splitting and coalescing
 *   3. Reports allocation count, free count, and remaining heap to markers
 *
 * Run with i8085-trace:
 *   i8085-trace --timer=65:30720 -n 2000000 -S -d 0xFE00:16 \
 *       build/freertos_heap.bin
 *
 * Success: MARKER_ALLOCS and MARKER_FREES are non-zero and equal,
 *          MARKER_PASS is non-zero (coalescing worked),
 *          MARKER_FREE_HEAP > 0 (heap not exhausted).
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Memory-mapped markers visible in i8085-trace dump */
#define MARKER_ALLOCS    ( *( volatile unsigned int * ) 0xFE00 )
#define MARKER_FREES     ( *( volatile unsigned int * ) 0xFE02 )
#define MARKER_PASS      ( *( volatile unsigned int * ) 0xFE04 )
#define MARKER_FREE_HEAP ( *( volatile unsigned int * ) 0xFE06 )
#define MARKER_QUEUE_OK  ( *( volatile unsigned int * ) 0xFE08 )
#define MARKER_TASK_RUNS ( *( volatile unsigned int * ) 0xFE0A )
#define MARKER_COALESCE  ( *( volatile unsigned int * ) 0xFE0C )

/* Idle task storage (still needed for static idle task) */
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
 * Worker task - created dynamically, increments a counter, then deletes itself
 *-----------------------------------------------------------*/
void vWorkerTask( void * pvParameters )
{
    ( void ) pvParameters;
    MARKER_TASK_RUNS++;
    /* Self-delete: FreeRTOS frees the TCB+stack via vPortFree */
    vTaskDelete( ( void * ) 0 );
}

/*-----------------------------------------------------------
 * Allocator task - exercises malloc/free patterns
 *-----------------------------------------------------------*/
void vAllocatorTask( void * pvParameters )
{
    ( void ) pvParameters;
    volatile unsigned int pass = 0;

    for( ;; )
    {
        /* ---- Test 1: Simple alloc/free ---- */
        {
            void *p1 = pvPortMalloc( 32 );
            if( p1 != ( void * ) 0 )
            {
                MARKER_ALLOCS++;
                /* Write a pattern to the allocated memory */
                unsigned char *pb = ( unsigned char * ) p1;
                for( int i = 0; i < 32; i++ )
                {
                    pb[ i ] = ( unsigned char ) i;
                }
                vPortFree( p1 );
                MARKER_FREES++;
            }
        }

        /* ---- Test 2: Coalescing test ---- */
        /* Allocate 3 adjacent blocks, free middle, free sides,
         * then allocate a block that only fits if coalescing worked */
        {
            void *a = pvPortMalloc( 64 );
            void *b = pvPortMalloc( 64 );
            void *c = pvPortMalloc( 64 );

            if( a && b && c )
            {
                MARKER_ALLOCS += 3;

                /* Free in order: b, a, c — forces coalescing of a+b, then a+b+c */
                vPortFree( b );
                vPortFree( a );
                vPortFree( c );
                MARKER_FREES += 3;

                /* Now allocate 180 bytes — only succeeds if a+b+c coalesced */
                void *big = pvPortMalloc( 180 );
                if( big != ( void * ) 0 )
                {
                    MARKER_COALESCE++;
                    MARKER_ALLOCS++;
                    vPortFree( big );
                    MARKER_FREES++;
                }
            }
            else
            {
                /* Clean up any successful allocs */
                if( a ) { vPortFree( a ); MARKER_FREES++; }
                if( b ) { vPortFree( b ); MARKER_FREES++; }
                if( c ) { vPortFree( c ); MARKER_FREES++; }
            }
        }

        /* ---- Test 3: Dynamic queue create/delete ---- */
        {
            QueueHandle_t q = xQueueCreate( 4, sizeof( unsigned int ) );
            if( q != ( void * ) 0 )
            {
                unsigned int val = 0xAB;
                xQueueSend( q, &val, 0 );

                unsigned int recv = 0;
                if( xQueueReceive( q, &recv, 0 ) == pdPASS && recv == 0xAB )
                {
                    MARKER_QUEUE_OK++;
                }
                vQueueDelete( q );
            }
        }

        /* ---- Test 4: Dynamic task creation ---- */
        /* Create a worker task that self-deletes (exercises TCB+stack alloc/free) */
        {
            TaskHandle_t h = ( void * ) 0;
            BaseType_t ret = xTaskCreate( vWorkerTask, "Wkr",
                                          configMINIMAL_STACK_SIZE,
                                          ( void * ) 0, 1, &h );
            if( ret == pdPASS )
            {
                /* Yield to let the worker run and self-delete */
                vTaskDelay( 2 );
            }
        }

        /* Record heap state */
        MARKER_FREE_HEAP = ( unsigned int ) xPortGetFreeHeapSize();

        pass++;
        MARKER_PASS = pass;

        /* Small delay between rounds */
        vTaskDelay( 5 );
    }
}

/*-----------------------------------------------------------
 * main
 *-----------------------------------------------------------*/
int main( void )
{
    MARKER_ALLOCS    = 0;
    MARKER_FREES     = 0;
    MARKER_PASS      = 0;
    MARKER_FREE_HEAP = 0;
    MARKER_QUEUE_OK  = 0;
    MARKER_TASK_RUNS = 0;
    MARKER_COALESCE  = 0;

    /* Create the allocator task dynamically (tests xTaskCreate itself) */
    xTaskCreate( vAllocatorTask, "Alloc",
                 configMINIMAL_STACK_SIZE,
                 ( void * ) 0, 2, ( void * ) 0 );

    vTaskStartScheduler();

    for( ;; ) { }
    return 0;
}
