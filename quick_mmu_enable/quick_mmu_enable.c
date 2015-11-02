/*
 * @file quick_mmu_enable.c
 * @date 2 Augr 2015
 * @author Chester Gillon
 * @brief Enable the MMU as quickly as possible
 * @todo When this program is placed on a bootable SD card, it prevents the CCS debugger from being able
 *       to download a program unless the SD card is removed.
 */

#include <stdint.h>

#include <hw/hw_types.h>
#include <hw/soc_AM335x.h>
#include <AM3352_SOM.h>
#include <hw/hw_cm_wkup.h>
#include <uartStdio.h>
#include <consoleUtils.h>
#include <mmu.h>

/* The MMU regions used */
#define START_ADDR_DEV             (0x44000000)
#define START_ADDR_IRAM            (0x40300000)
#define NUM_SECTIONS_DEV           (960)
#define NUM_SECTIONS_IRAM          (1)

/* page tables start must be aligned in 16K boundary */
#pragma DATA_ALIGN(pageTable, 16384);
static volatile unsigned int pageTable[4*1024];

/*
** Function to setup MMU. This function Maps three regions (1. DDR
** 2. OCMC and 3. Device memory) and enables MMU.
*/
static void MMUConfigAndEnable(void)
{
    /*
    ** Define IRAM memory region of AM335x. IRAM can be configured as Normal
    ** memory with R/W access in user/privileged modes. The cache attributes
    ** specified here are,
    ** Inner - Write through, No Write Allocate
    ** Outer - Write Back, Write Allocate
    */
    REGION regionIram = {
                         MMU_PGTYPE_SECTION, START_ADDR_IRAM, NUM_SECTIONS_IRAM,
                         MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                          MMU_CACHE_WB_WA),
                         MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                         (unsigned int*)pageTable
                        };

    /*
    ** Define Device Memory Region. The region between OCMC and DDR is
    ** configured as device memory, with R/W access in user/privileged modes.
    ** Also, the region is marked 'Execute Never'.
    */
    REGION regionDev = {
                        MMU_PGTYPE_SECTION, START_ADDR_DEV, NUM_SECTIONS_DEV,
                        MMU_MEMTYPE_DEVICE_SHAREABLE,
                        MMU_REGION_NON_SECURE,
                        MMU_AP_PRV_RW_USR_RW  | MMU_SECTION_EXEC_NEVER,
                        (unsigned int*)pageTable
                       };

    /* Initialize the page table and MMU */
    MMUInit((unsigned int*)pageTable);

    /* Map the defined regions */
    MMUMemRegionMap(&regionIram);
    MMUMemRegionMap(&regionDev);

    /* Now Safe to enable MMU */
    MMUEnable((unsigned int*)pageTable);
}

/**
 * @brief This function is used to initialize and configure UART Module.
 */
static void UART_setup (void)
{
    volatile unsigned int regVal;

    UARTStdioInit();
}

int main(void)
{
    MMUConfigAndEnable ();
    UART_setup ();

    UARTprintf ("MMU enabled\n");
    for (;;)
    {
    }

    return 0;
}
