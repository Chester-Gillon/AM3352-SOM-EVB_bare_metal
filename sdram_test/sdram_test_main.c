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
#include <AM3352_SOM.h>
#include <hw/hw_cm_wkup.h>
#include <uartStdio.h>
#include <consoleUtils.h>
#include <rtc.h>
#include <cache.h>

#define SDRAM_SIZE_BYTES (512 * 1024 * 1024)
#define SDRAM_SIZE_WORDS (SDRAM_SIZE_BYTES / sizeof (uint32_t))

#define MASK_HOUR            (0xFF000000u)
#define MASK_MINUTE          (0x00FF0000u)
#define MASK_SECOND          (0x0000FF00u)
#define MASK_MERIDIEM        (0x000000FFu)

#define HOUR_SHIFT           (24)
#define MINUTE_SHIFT         (16)
#define SECOND_SHIFT         (8)

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

/**
 * @brief Initialise the RTC to start counting from an epoch of 00:00:00 for providing timing
 */
static void RTC_setup (void)
{
    /* Performing the System Clock configuration for RTC. */
    RTCModuleClkConfig();

    /* Disabling Write Protection for RTC registers.*/
    RTCWriteProtectDisable(SOC_RTC_0_REGS);

    /* Selecting Internal Clock source for RTC. */
    RTC32KClkSourceSelect(SOC_RTC_0_REGS, RTC_INTERNAL_CLK_SRC_SELECT);

    /* Enabling RTC to receive the Clock inputs. */
    RTC32KClkClockControl(SOC_RTC_0_REGS, RTC_32KCLK_ENABLE);

    /* Enable the RTC module. */
    RTCEnable(SOC_RTC_0_REGS);

    /* Programming epoch in the Calendar registers. */
    RTCTimeSet (SOC_RTC_0_REGS, 0);
    RTCCalendarSet (SOC_RTC_0_REGS, 0);

    /* Set the 32KHz counter to run. */
    RTCRun(SOC_RTC_0_REGS);
}

/*
** This function converts a 8 bit number to its ASCII equivalent value.
** The 8 bit number is passed as a parameter to this function.
*/

static unsigned int intToASCII(unsigned char byte)
{
    unsigned int retVal = 0;
    unsigned char lsn = 0, msn = 0;
    lsn = (byte & 0x0F);
    msn = (byte & 0xF0) >> 0x04;

    retVal = (lsn + 0x30);
    retVal |= ((msn + 0x30) << 0x08);

    return retVal;
}

/*
** This function prints the current time read from the RTC registers.
*/

static void time_resolve (unsigned int timeValue)
{
    unsigned char timeArray[3] = {0};
    unsigned char bytePrint[2] = {0};
    unsigned int count = 0, i = 0;
    unsigned int asciiTime = 0;

    timeArray[0] = (unsigned char)((timeValue & MASK_HOUR) >> HOUR_SHIFT);
    timeArray[1] = (unsigned char)((timeValue & MASK_MINUTE) >> MINUTE_SHIFT);
    timeArray[2] = (unsigned char)((timeValue & MASK_SECOND) >> SECOND_SHIFT);

    while(count < 3)
    {
        i = 0;
        asciiTime = intToASCII(timeArray[count]);
        bytePrint[0] = (unsigned char)((asciiTime & 0x0000FF00) >> 0x08);
        bytePrint[1] = (unsigned char)(asciiTime & 0x000000FF);
        while(i < 2)
        {
            ConsoleUtilsPrintf("%c", (bytePrint[i]));
            i++;
        }
        count++;
        if(count != 3)
        {
            ConsoleUtilsPrintf("%c", ':');
        }
        else
        {
            ConsoleUtilsPrintf("%c", ' ');
        }
    }
}

int main (void)
{
    uint32_t *const sdram_base = (uint32_t *) 0x80000000;
    uint32_t index;
    uint32_t num_errors;

    CacheEnable (CACHE_ALL);
    UART_setup ();
    RTC_setup ();

    time_resolve (RTCTimeGet (SOC_RTC_0_REGS));
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

    time_resolve (RTCTimeGet (SOC_RTC_0_REGS));
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

    time_resolve (RTCTimeGet (SOC_RTC_0_REGS));
    UARTprintf ("After ~index write num_errors=%u\n", num_errors);

    return 0;
}

