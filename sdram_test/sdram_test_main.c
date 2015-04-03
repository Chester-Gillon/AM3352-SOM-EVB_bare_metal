/*
 * @file sdram_test_main.c
 * @date 28 Mar 2015
 * @author Chester Gillon
 * @brief SDRAM test which executes in the AM3352 on-chip SRAM, to be able to test all of the SDRAM
 * @details This version writes and then reads back a test pattern to the entire SDRAM, with progress reported via the UART0
 */

#include <stdint.h>

#include <hw/hw_types.h>
#include <hw/soc_AM335x.h>
#include <hw/hw_cm_wkup.h>
#include <uartStdio.h>

#define SDRAM_SIZE_BYTES (512 * 1024 * 1024)
#define SDRAM_SIZE_WORDS (SDRAM_SIZE_BYTES / sizeof (uint32_t))

/**
 * @brief This function is used to initialize and configure UART Module.
 */
static void UART_setup (void)
{
    volatile unsigned int regVal;

    /* Enable clock for UART0 */
    regVal = (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_UART0_CLKCTRL) &
                    ~(CM_WKUP_UART0_CLKCTRL_MODULEMODE));

    regVal |= CM_WKUP_UART0_CLKCTRL_MODULEMODE_ENABLE;

    HWREG(SOC_CM_WKUP_REGS + CM_WKUP_UART0_CLKCTRL) = regVal;

    UARTStdioInit();
}

int main (void)
{
    uint32_t *const sdram_base = (uint32_t *) 0x80000000;
    uint32_t index;
    uint32_t num_errors;

    UART_setup ();

    UARTprintf ("Write test starting\n");
    for (index = 0; index < SDRAM_SIZE_WORDS; index++)
    {
        sdram_base[index] = index;
    }

    num_errors = 0;
    for (index = 0; index < SDRAM_SIZE_WORDS; index++)
    {
        if (sdram_base[index] != index)
        {
            num_errors++;
        }
    }

    UARTprintf ("After index write num_errors=%u\n", num_errors);

    for (index = 0; index < SDRAM_SIZE_WORDS; index++)
    {
        sdram_base[index] = ~index;
    }
    for (index = 0; index < SDRAM_SIZE_WORDS; index++)
    {
        if (sdram_base[index] != ~index)
        {
            num_errors++;
        }
    }

    UARTprintf ("After ~index write num_errors=%u\n", num_errors);

    return 0;
}

