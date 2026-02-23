/*
 * FreeRTOS Intel 8080 demo - Two tasks writing to memory markers.
 *
 * Run with i8085-trace:
 *   i8085-trace -e 0x0000 -l 0x0000 -n 500000 -d 0xFE00:8 \
 *       --timer=7:20000 build/freertos_basic.bin
 *
 * The --timer=7:20000 flag fires RST 7 every 20000 cycles (100 Hz
 * at 2 MHz), simulating an external 8253/8259 timer interrupt.
 *
 * Success: both MARKER_A (0xFE00) and MARKER_B (0xFE02) are non-zero
 * in the memory dump, proving both tasks ran under preemptive scheduling.
 */

#include "FreeRTOS.h"
#include "task.h"

/* Memory-mapped markers visible in i8085-trace dump.
 * These are in the test dump region at 0xFE00+. */
#define MARKER_A    ( *( volatile unsigned int * ) 0xFE00 )
#define MARKER_B    ( *( volatile unsigned int * ) 0xFE02 )

/* Task stack + TCB storage (static allocation) */
static StaticTask_t xTaskA_TCB;
static StackType_t  xTaskA_Stack[ configMINIMAL_STACK_SIZE ];

static StaticTask_t xTaskB_TCB;
static StackType_t  xTaskB_Stack[ configMINIMAL_STACK_SIZE ];

/* Idle task storage (required by configSUPPORT_STATIC_ALLOCATION) */
static StaticTask_t xIdleTaskTCB;
static StackType_t  xIdleTaskStack[ configMINIMAL_STACK_SIZE ];

/*-----------------------------------------------------------
 * Idle task memory callback (required for static allocation)
 *-----------------------------------------------------------*/
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE * pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------
 * Task A - increment marker at 0xFE00
 *-----------------------------------------------------------*/
void vTaskA( void * pvParameters )
{
    ( void ) pvParameters;

    for( ;; )
    {
        MARKER_A++;

        /* Busy-wait loop to burn some cycles */
        for( volatile int i = 0; i < 50; i++ )
        {
        }
    }
}

/*-----------------------------------------------------------
 * Task B - increment marker at 0xFE02
 *-----------------------------------------------------------*/
void vTaskB( void * pvParameters )
{
    ( void ) pvParameters;

    for( ;; )
    {
        MARKER_B++;

        for( volatile int i = 0; i < 50; i++ )
        {
        }
    }
}

/*-----------------------------------------------------------
 * main
 *-----------------------------------------------------------*/
int main( void )
{
    /* Clear markers */
    MARKER_A = 0;
    MARKER_B = 0;

    /* Create tasks at equal priority (time-sliced) */
    xTaskCreateStatic( vTaskA, "TaskA", configMINIMAL_STACK_SIZE,
                       ( void * ) 0, 1,
                       xTaskA_Stack, &xTaskA_TCB );

    xTaskCreateStatic( vTaskB, "TaskB", configMINIMAL_STACK_SIZE,
                       ( void * ) 0, 1,
                       xTaskB_Stack, &xTaskB_TCB );

    /* Start scheduler - never returns */
    vTaskStartScheduler();

    /* Should never get here */
    for( ;; )
    {
    }

    return 0;
}
