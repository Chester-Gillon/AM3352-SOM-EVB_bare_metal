/*
 * @file uart_console_interrupts.c
 * @date 3 Nov 2017
 * @author Chester Gillon
 * @brief Contains UART console output functions which use interrupts to transmit from a buffer
 * @details This allows the output of characters to be queued without having to block waiting for the previous
 */

#include <stdint.h>
#include <stdlib.h>

#include "interrupt.h"
#include "uart_irda_cir.h"
#include "soc_AM335x.h"
#include "AM3352_SOM.h"
#include "hw_types.h"

/* Select constants for the specified UART console port */
#if UART_CONSOLE_PORT == 0
    #define UART_CONSOLE_BASE                (SOC_UART_0_REGS)
    #define UART_CONSOLE_INT                 (SYS_INT_UART0INT)
#elif UART_CONSOLE_PORT == 1
    #define UART_CONSOLE_BASE                (SOC_UART_1_REGS)
    #define UART_CONSOLE_INT                 (SYS_INT_UART1INT)
#elif UART_CONSOLE_PORT == 2
    #define UART_CONSOLE_BASE                (SOC_UART_2_REGS)
    #define UART_CONSOLE_INT                 (SYS_INT_UART2INT)
#elif UART_CONSOLE_PORT == 4
    #define UART_CONSOLE_BASE                (SOC_UART_4_REGS)
    #define UART_CONSOLE_INT                 (SYS_INT_UART4INT)
#else
    #error "Unknown UART_CONSOLE_PORT"
#endif

/** The baud rate used for the console port */
#define BAUD_RATE                            (115200)

#define UART_MODULE_INPUT_CLK                (48000000)

/** Allows buffering of data which takes approx one second to transmit */
#define UART_BUFFER_SIZE (BAUD_RATE / 10)

/** Circular buffer used to store characters waiting to be transmitted by the console port */
static uint8_t *uart_tx_buffer;
static uint32_t uart_tx_buffer_num_chars;
static uint32_t uart_tx_buffer_read_index;
static uint32_t uart_tx_buffer_write_index;

/*
** A wrapper function performing FIFO configurations.
*/
static void UartFIFOConfigure(unsigned int txTrigLevel,
                              unsigned int rxTrigLevel)
{
    unsigned int fifoConfig = 0;

    /* Setting the TX and RX FIFO Trigger levels as 1. No DMA enabled. */
    fifoConfig = UART_FIFO_CONFIG(UART_TRIG_LVL_GRANULARITY_1,
                                  UART_TRIG_LVL_GRANULARITY_1,
                                  txTrigLevel,
                                  rxTrigLevel,
                                  1,
                                  1,
                                  UART_DMA_EN_PATH_SCR,
                                  UART_DMA_MODE_0_ENABLE);

    /* Configuring the FIFO settings. */
    UARTFIFOConfig(UART_CONSOLE_BASE, fifoConfig);
}

/*
** A wrapper function performing Baud Rate settings.
*/
static void UartBaudRateSet(unsigned int baudRate)
{
    unsigned int divisorValue = 0;

    /* Computing the Divisor Value. */
    divisorValue = UARTDivisorValCompute(UART_MODULE_INPUT_CLK,
                                         baudRate,
                                         UART16x_OPER_MODE,
                                         UART_MIR_OVERSAMPLING_RATE_42);

    /* Programming the Divisor Latches. */
    UARTDivisorLatchWrite(UART_CONSOLE_BASE, divisorValue);
}

/**
 * \brief   This function initializes the specified UART instance for use.
 *          This does the following:
 *          - Enables the FIFO and performs the FIFO settings\n
 *          - Performs the Baud Rate settings\n
 *          - Configures the Line Characteristics to 8-N-1 format\n
 *
 * \param   baudRate     The baud rate to be used for communication
 * \param   rxTrigLevel  The receiver FIFO trigger level
 * \param   txTrigLevel  The transmitter FIFO trigger level
 *
 * \return  None
 *
 * \note    By default, a granularity of 1 will be selected for transmitter
 *          and receiver FIFO trigger levels.
 */
static void UARTStdioInitExpClk(unsigned int baudRate,
                                unsigned int rxTrigLevel,
                                unsigned int txTrigLevel)
{
    /* Performing a module reset. */
    UARTModuleReset(UART_CONSOLE_BASE);

    /* Performing FIFO configurations. */
    UartFIFOConfigure(txTrigLevel, rxTrigLevel);

    /* Performing Baud Rate settings. */
    UartBaudRateSet(baudRate);

    /* Switching to Configuration Mode B. */
    UARTRegConfigModeEnable(UART_CONSOLE_BASE, UART_REG_CONFIG_MODE_B);

    /* Programming the Line Characteristics. */
    UARTLineCharacConfig(UART_CONSOLE_BASE,
                         (UART_FRAME_WORD_LENGTH_8 | UART_FRAME_NUM_STB_1),
                         UART_PARITY_NONE);

    /* Disabling write access to Divisor Latches. */
    UARTDivisorLatchDisable(UART_CONSOLE_BASE);

    /* Disabling Break Control. */
    UARTBreakCtl(UART_CONSOLE_BASE, UART_BREAK_COND_DISABLE);

    /* Switching to UART16x operating mode. */
    UARTOperatingModeSelect(UART_CONSOLE_BASE, UART16x_OPER_MODE);
}

/**
 * @brief UART ISR which transfers characters from a software circular buffer to the console UART transmit FIFO
 */
static void UART_isr (void)
{
    unsigned int intId = 0;

    /* Checking ths source of UART interrupt. */
    intId = UARTIntIdentityGet (UART_CONSOLE_BASE);

    switch(intId)
    {
        case UART_INTID_TX_THRES_REACH:
            /* Fill the UART transmit FIFO with characters from the buffer */
            while ((uart_tx_buffer_num_chars > 0) &&
                   (UARTTxFIFOFullStatusGet (UART_CONSOLE_BASE) == UART_TX_FIFO_NOT_FULL))
            {
                UARTFIFOWrite (UART_CONSOLE_BASE, &uart_tx_buffer[uart_tx_buffer_read_index], 1);
                uart_tx_buffer_num_chars--;
                uart_tx_buffer_read_index = (uart_tx_buffer_read_index + 1) % UART_BUFFER_SIZE;
            }

            if (uart_tx_buffer_num_chars == 0)
            {
                /* Disable transmit interrupt when there are no more characters to transmit */
                UARTIntDisable (UART_CONSOLE_BASE, UART_INT_THR);
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Initialise the specified UART instance for the console output, using interrupts
 */
void UARTConsoleInit (void)
{
    /* Initialise the transmit buffer to be empty */
    uart_tx_buffer = malloc (UART_BUFFER_SIZE);
    uart_tx_buffer_num_chars = 0;
    uart_tx_buffer_read_index = 0;
    uart_tx_buffer_write_index = 0;

    /* Configuring the system clocks for UART instance. */
#if UART_CONSOLE_PORT == 0
    UART0ModuleClkConfig();
#elif UART_CONSOLE_PORT == 1
    UART1ModuleClkConfig();
#elif UART_CONSOLE_PORT == 2
    UART2ModuleClkConfig();
#elif UART_CONSOLE_PORT == 4
    UART4ModuleClkConfig();
#else
    #error "Unknown UART_CONSOLE_PORT"
#endif

    /* Performing the Pin Multiplexing for UART instance. */
    UARTPinMuxSetup (UART_CONSOLE_PORT);

    UARTStdioInitExpClk (BAUD_RATE, 1, 1);

    /* Install the UART transmit interrupt handler, but initially no UART interrupt sources enabled */
    IntRegister (UART_CONSOLE_INT, UART_isr);
    IntPrioritySet (UART_CONSOLE_INT, 0, AINTC_HOSTINT_ROUTE_IRQ);
    IntSystemEnable (UART_CONSOLE_INT);
}

/**
 * @brief This function puts a character on the serial console, without blocking the caller
 * @details To avoid blocking the caller performs the following in preference
 *          a) Write the character directly into UART transmit FIFO when there is space in the FIFO
  *            and no characters in the software buffer.
  *         b) Store the character in the software buffer, which will be output by the UART ISR
  *         c) Discard the character if the software buffer is full.
  *
  *         The UART interrupt is temporarily disabled which updating the software buffer.
 * @param[in] data The character to output on the serial console
 */
void UARTConsolePutc (unsigned char data)
{
    IntSystemDisable (UART_CONSOLE_INT);

    if (uart_tx_buffer_num_chars == UART_BUFFER_SIZE)
    {
        /* The software transmit buffer is full, so the character has to be discarded */
    }
    else if ((uart_tx_buffer_num_chars == 0) && (UARTTxFIFOFullStatusGet (UART_CONSOLE_BASE) == UART_TX_FIFO_NOT_FULL))
    {
        /* The software transmit buffer is empty and the UART transmit FIFO is not full,
         * so the character can be written to the UART transmit FIFO */
        UARTFIFOWrite (UART_CONSOLE_BASE, &data, 1);
    }
    else
    {
        /* Add the character to the software transmit buffer */
        uart_tx_buffer[uart_tx_buffer_write_index] = data;
        uart_tx_buffer_write_index = (uart_tx_buffer_write_index + 1) % UART_BUFFER_SIZE;
        uart_tx_buffer_num_chars++;

        if (uart_tx_buffer_num_chars == 1)
        {
            UARTIntEnable (UART_CONSOLE_BASE, UART_INT_THR);
        }
    }

    IntSystemEnable (UART_CONSOLE_INT);
}
