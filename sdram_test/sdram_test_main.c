/*
 * @file sdram_test_main.c
 * @date 28 Mar 2015
 * @author Chester Gillon
 * @brief SDRAM test which executes in the AM3352 on-chip SRAM, to be able to test all of the SDRAM
 * @details This version writes and then reads back a test pattern to the entire SDRAM, with progress reported via the CCS debugger vCIO
 */

#include <stdio.h>
#include <stdint.h>

#define SDRAM_SIZE_BYTES (512 * 1024 * 1024)
#define SDRAM_SIZE_WORDS (SDRAM_SIZE_BYTES / sizeof (uint32_t))

int main (void)
{
    uint32_t *const sdram_base = (uint32_t *) 0x80000000;
    uint32_t index;
    uint32_t num_errors;

    printf ("Write test starting\n");
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

    printf ("After index write num_errors=%u\n", num_errors);

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

    printf ("After ~index write num_errors=%u\n", num_errors);

    return 0;
}

