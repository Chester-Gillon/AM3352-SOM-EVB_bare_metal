/*
 * @file sdram_test_main.c
 * @date 31 Aug 2015
 * @author Chester Gillon
 * @brief Test of using software to pass Ethernet packets between the two ethernet ports on the AM3352-SOM
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <AM3352_SOM.h>
#include <soc_AM335x.h>
#include <uartStdio.h>
#include <consoleUtils.h>
#include <mdio.h>
#include <phy.h>
#include <cpsw.h>
#include <rtc.h>
#include <hw/hw_types.h>

/* Copies of macros from drivers/rtc.c which are not part of the API */
#define MASK_HOUR            (0xFF000000u)
#define MASK_MINUTE          (0x00FF0000u)
#define MASK_SECOND          (0x0000FF00u)

#define HOUR_SHIFT           (24)
#define MINUTE_SHIFT         (16)
#define SECOND_SHIFT         (8)

#define LEN_MAC_ADDRESS 6

/* MDIO input and output frequencies in Hz */
#define MDIO_FREQ_INPUT                          125000000
#define MDIO_FREQ_OUTPUT                         1000000

#define NUM_PHY_ADDRESSES 32u

#define BMSR_ESTATEN        0x0100  /* Extended Status in R15 */

/** Offsets for CPSW Statistics - which are accumulated across all ports */
#define CPSW_STAT_GOOD_RX_FRAMES 0x0
#define CPSW_STAT_BROADCAST_RX_FRAMES 0x4
#define CPSW_STAT_MULTICAST_RX_FRAMES 0x8
#define CPSW_STAT_PAUSE_RX_FRAMES 0xc
#define CPSW_STAT_RX_CRC_ERRORS 0x10
#define CPSW_STAT_RX_ALIGN_CODE_ERRORS 0x14
#define CPSW_STAT_OVERSIZE_RX_FRAMES 0x18
#define CPSW_STAT_RX_JABBERS 0x1c
#define CPSW_STAT_UNDERSIZE_RX_FRAMES 0x20
#define CPSW_STAT_RX_FRAGMENTS 0x24
#define CPSW_STAT_RX_START_OF_FRAME_OVERRUNS 0x84
#define CPSW_STAT_RX_MIDDLE_OF_FRAME_OVERRUNS 0x88
#define CPSW_STAT_RX_DMA_OVERRUNS 0x8c
#define CPSW_STAT_RX_OCTETS 0x30
#define CPSW_STAT_NET_OCTETS 0x80
#define CPSW_STAT_GOOD_TX_FRAMES 0x34
#define CPSW_STAT_BROADCAST_TX_FRAMES 0x38
#define CPSW_STAT_MULTICAST_TX_FRAMES 0x3c
#define CPSW_STAT_PAUSE_TX_FRAMES 0x40
#define CPSW_STAT_COLLISIONS 0x48
#define CPSW_STAT_SINGLE_COLLISION_TX_FRAMES 0x4c
#define CPSW_STAT_MULTIPLE_COLLISION_TX_FRAMES 0x50
#define CPSW_STAT_EXCESSIVE_COLLISIONS 0x54
#define CPSW_STAT_LATE_COLLISIONS 0x58
#define CPSW_STAT_TX_UNDERRUNS 0x5c
#define CPSW_STAT_DEFERRED_TX_FRAMES 0x44
#define CPSW_STAT_CARRIER_SENSE_ERRORS 0x60
#define CPSW_STAT_TX_OCTETS 0x64

/** Collection of CPSW statistics accumulated across all ports */
typedef struct
{
    unsigned int good_rx_frames;
    unsigned int broadcast_rx_frames;
    unsigned int multicast_rx_frames;
    unsigned int pause_rx_frames;
    unsigned int rx_crc_errors;
    unsigned int rx_align_code_errors;
    unsigned int oversize_rx_frames;
    unsigned int rx_jabbers;
    unsigned int undersize_rx_frames;
    unsigned int rx_fragments;
    unsigned int rx_start_of_frame_overruns;
    unsigned int rx_middle_of_frame_overruns;
    unsigned int rx_dma_overruns;
    unsigned int rx_octets;
    unsigned int net_octets;
    unsigned int good_tx_frames;
    unsigned int broadcast_tx_frames;
    unsigned int multicast_tx_frames;
    unsigned int pause_tx_frames;
    unsigned int collisions;
    unsigned int single_collision_tx_frames;
    unsigned int multiple_collision_tx_frames;
    unsigned int excessive_collisions;
    unsigned int tx_underruns;
    unsigned int deferred_tx_frames;
    unsigned int carrier_sense_errors;
    unsigned int tx_octets;
} cpsw_statistics_t;

/** Possible values for the link speed of one Ethernet phy */
typedef enum
{
    /** As Auto negotation is not complete, the link speed is not known */
    LINK_SPEED_AUTO_NEGOTIATION_NOT_COMPLETE,

    /** Auto negotation is complete, but failed to determine the link speed */
    LINK_SPEED_UNKNOWN,

    /** Valid link speeds determined from the phy */
    LINK_SPEED_1000M_FULL_DUPLEX,
    LINK_SPEED_1000M_HALF_DUPLEX, /* @todo the AM3352 doesn't support this */
    LINK_SPEED_100M_FULL_DUPLEX,
    LINK_SPEED_100M_HALF_DUPLEX,
    LINK_SPEED_10M_FULL_DUPLEX,
    LINK_SPEED_10M_HALF_DUPLEX
} phy_derived_link_speed;

/** Contains the status for one Ethernet phy, to be able to detect changes */
typedef struct
{
    /** True if the link is up */
    unsigned long link_status;
    /** The Basic Status register value, to determine if auto-negotiation is complete */
    unsigned short basic_status;
    /** Ability of the partner, from the Auto Negotiation Link Partner Ability Register */
    unsigned short partner_ability;
    /** Gigabit ability of the partner, from the 1000BASE-T Status Register register */
    unsigned short gpbs_partner_ability;
    /** The link speed of the phy derived from the other registers */
    phy_derived_link_speed link_speed;
} phy_status_t;

/**
 * @brief This function is used to initialize and configure UART Module.
 */
static void UART_setup (void)
{
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

    /* Selecting External Clock source for RTC. */
    RTC32KClkSourceSelect(SOC_RTC_0_REGS, RTC_EXTERNAL_CLK_SRC_SELECT);

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

/**
 * @brief Convert one BCD time field from the RTC to an integer
 * @param[in] rtc_bcd RTC BCD field to convert
 * @return Returns rtc_bcd converted to integer
 */
static unsigned int bcd_to_int (unsigned int rtc_bcd)
{
    const unsigned int lsn =  rtc_bcd & 0x0f;
    const unsigned int msn = (rtc_bcd & 0xf0) >> 4;

    return (msn * 10) + lsn;
}

/**
 * @brief Get the integer seconds field from a RTC time value
 * @param[in] rtc_time The time read from the RTC
 * @return Returns the seconds from rtc_time as an integer
 */
static unsigned int get_rtc_seconds (unsigned int rtc_time)
{
    return bcd_to_int ((rtc_time & MASK_SECOND) >> SECOND_SHIFT);
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

/**
 * @brief Read the status of all phys which are indicates as alive
 * @todo Assumes that the Ethernet phy defaults to auto-negotiate at reset
 * @param[out] phys_status Where to store the status of the phys, indexed by the phy address
 */
static void read_phy_status (phy_status_t phys_status[NUM_PHY_ADDRESSES])
{
    const unsigned int phys_alive_status = MDIOPhyAliveStatusGet (SOC_CPSW_MDIO_REGS);
    unsigned int phy_address;
    bool status_available;

    for (phy_address = 0; phy_address < NUM_PHY_ADDRESSES; phy_address++)
    {
        phy_status_t *const status = &phys_status[phy_address];

        status_available = (phys_alive_status & (1u << phy_address)) != 0u;
        if (status_available)
        {
            status->link_status = PhyLinkStatusGet (SOC_CPSW_MDIO_REGS, phy_address, 0);
            status_available = MDIOPhyRegRead (SOC_CPSW_MDIO_REGS, phy_address, PHY_BSR, &status->basic_status);
            if (status_available)
            {
                /* Only attempt to read the gigabit parter ability if the phy supports Gigabit */
                status->gpbs_partner_ability = (status->basic_status & BMSR_ESTATEN) != 0;
                status_available = PhyPartnerAbilityGet (SOC_CPSW_MDIO_REGS, phy_address,
                                                         &status->partner_ability, &status->gpbs_partner_ability);
            }
        }

        if (status_available)
        {
            /* Determine the link speed of the phy */
            if ((status->basic_status & PHY_AUTONEG_COMPLETE) != 0)
            {
                if ((status->gpbs_partner_ability & PHY_LINK_PARTNER_1000BT_FD) != 0)
                {
                    status->link_speed = LINK_SPEED_1000M_FULL_DUPLEX;
                }
                else if ((status->gpbs_partner_ability & PHY_LINK_PARTNER_1000BT_HD) != 0)
                {
                    status->link_speed = LINK_SPEED_1000M_HALF_DUPLEX;
                }
                else if ((status->partner_ability & PHY_100BTX_FD) != 0)
                {
                    status->link_speed = LINK_SPEED_100M_FULL_DUPLEX;
                }
                else if ((status->partner_ability & PHY_100BTX) != 0)
                {
                    status->link_speed = LINK_SPEED_100M_HALF_DUPLEX;
                }
                else if ((status->partner_ability & PHY_10BT_FD) != 0)
                {
                    status->link_speed = LINK_SPEED_10M_FULL_DUPLEX;
                }
                else if ((status->partner_ability & PHY_10BT) != 0)
                {
                    status->link_speed = LINK_SPEED_10M_HALF_DUPLEX;
                }
                else
                {
                    status->link_speed = LINK_SPEED_UNKNOWN;
                }
            }
            else
            {
                status->link_speed = LINK_SPEED_AUTO_NEGOTIATION_NOT_COMPLETE;
            }
        }
        else
        {
            status->link_status = false;
            status->partner_ability = 0;
            status->gpbs_partner_ability = 0;
            status->link_speed = LINK_SPEED_AUTO_NEGOTIATION_NOT_COMPLETE;
        }
    }
}

/**
 * @brief Set the transfer mode of a CPSW port to match the link speed / duplex in the Ethernet phy
 * @details To be called upon detecting a change in the link speed / duplex.
 *          Since the Ethernet phys are using MII mode the actual link speed doesn't need to be set,
 *          but the duplex is important to make sure the CPSW considers the transmission to be successful.
 * @param[in] phy_address Identifies the Ethernet phy
 * @param[in] status The status of the Ethernet phy
 */
static void set_cpsw_transfer_mode (const unsigned int phy_address, const phy_status_t *const status)
{
    unsigned int base_address;
    unsigned int transfer_mode = 0;
    bool enable = false;

    switch (phy_address)
    {
    case 0:
        /* Phy address 0 is connected to CPSW port 1. Ethernet connected is LAN2 */
        base_address = SOC_CPSW_SLIVER_1_REGS;
        break;

    case 1:
        /* Phy address 1 is connected to CPSW port 2. Ethernet connected is LAN1 */
        base_address = SOC_CPSW_SLIVER_2_REGS;
        break;

    default:
        base_address = 0;
        break;
    }

    if (base_address != 0)
    {
        switch (status->link_speed)
        {
        case LINK_SPEED_AUTO_NEGOTIATION_NOT_COMPLETE:
        case LINK_SPEED_UNKNOWN:
            enable = false;
            break;

        case LINK_SPEED_1000M_HALF_DUPLEX:
            /* This link speed isn't supported by the CPSW */
            enable = false;
            break;

        case LINK_SPEED_1000M_FULL_DUPLEX:
        case LINK_SPEED_100M_FULL_DUPLEX:
        case LINK_SPEED_10M_FULL_DUPLEX:
            transfer_mode = CPSW_SL_MACCONTROL_FULLDUPLEX;
            enable = true;
            break;

        case LINK_SPEED_100M_HALF_DUPLEX:
        case LINK_SPEED_10M_HALF_DUPLEX:
            transfer_mode = 0;
            enable = true;
            break;
        }

        if (enable)
        {
            CPSWSlTransferModeSet (base_address, transfer_mode);
            CPSWSlGMIIEnable (base_address);
        }
        else
        {
            HWREG(base_address + CPSW_SL_MACCONTROL) &= ~CPSW_SL_MACCONTROL_GMII_EN;
        }
    }
}

/**
 * @brief Read the current statistics from the CPSW
 * @param[out] stats Where to store the current statistics
 */
static void get_cpsw_statistics (cpsw_statistics_t *const stats)
{
    stats->good_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_GOOD_RX_FRAMES);
    stats->broadcast_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_BROADCAST_RX_FRAMES);
    stats->multicast_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_MULTICAST_RX_FRAMES);
    stats->pause_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_PAUSE_RX_FRAMES);
    stats->rx_crc_errors = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_CRC_ERRORS);
    stats->rx_align_code_errors = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_ALIGN_CODE_ERRORS);
    stats->oversize_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_OVERSIZE_RX_FRAMES);
    stats->rx_jabbers = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_JABBERS);
    stats->undersize_rx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_UNDERSIZE_RX_FRAMES);
    stats->rx_fragments = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_FRAGMENTS);
    stats->rx_start_of_frame_overruns = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_START_OF_FRAME_OVERRUNS);
    stats->rx_middle_of_frame_overruns = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_MIDDLE_OF_FRAME_OVERRUNS);
    stats->rx_dma_overruns = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_DMA_OVERRUNS);
    stats->rx_octets = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_RX_OCTETS);
    stats->net_octets = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_NET_OCTETS);
    stats->good_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_GOOD_TX_FRAMES);
    stats->broadcast_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_BROADCAST_TX_FRAMES);
    stats->multicast_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_MULTICAST_TX_FRAMES);
    stats->pause_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_PAUSE_TX_FRAMES);
    stats->collisions = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_COLLISIONS);
    stats->single_collision_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_SINGLE_COLLISION_TX_FRAMES);
    stats->multiple_collision_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_MULTIPLE_COLLISION_TX_FRAMES);
    stats->excessive_collisions = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_EXCESSIVE_COLLISIONS);
    stats->tx_underruns = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_TX_UNDERRUNS);
    stats->deferred_tx_frames = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_DEFERRED_TX_FRAMES);
    stats->carrier_sense_errors = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_CARRIER_SENSE_ERRORS);
    stats->tx_octets = CPSWStatisticsGet (SOC_CPSW_STAT_REGS, CPSW_STAT_TX_OCTETS);
}

/**
 * @brief Display one CPSW statistic
 * @details Only display the statistic if the current value is non-zero or has changed.
 *          This means the error statistics will only be displayed once an error has occurred
 * @param[in] stat_name The name of the statistic
 * @param[in] current_value The current value of the statistic
 * @param[in] previous_value The statistic value from the previous iteration, used to detect change
 */
static void display_one_cpsw_statistic (const char *stat_name, const unsigned int current_value, const unsigned int previous_value)
{
    if ((current_value != 0) || (current_value != previous_value))
    {
        UARTprintf ("%s = %u", stat_name, current_value);
        if (current_value != previous_value)
        {
            UARTprintf (" (+%u)", current_value - previous_value);
        }
        UARTprintf ("\n");
    }
}

/**
 * @brief Display the CPSW statistics
 * @param[in] current_stats The current statistics
 * @param[in] previous_stats The statistics from the previous call to this function, used to report changes
 */
static void display_cpsw_statistics (const cpsw_statistics_t *const current_stats, const cpsw_statistics_t *const previous_stats)
{
    display_one_cpsw_statistic ("Net octets                  ", current_stats->net_octets, previous_stats->net_octets);
    display_one_cpsw_statistic ("RX octets                   ", current_stats->rx_octets, previous_stats->rx_octets);
    display_one_cpsw_statistic ("TX octets                   ", current_stats->tx_octets, previous_stats->tx_octets);
    display_one_cpsw_statistic ("Good RX frames              ", current_stats->good_rx_frames, previous_stats->good_rx_frames);
    display_one_cpsw_statistic ("Good TX frames              ", current_stats->good_tx_frames, previous_stats->good_tx_frames);
    display_one_cpsw_statistic ("Multicast TX frames         ", current_stats->multicast_tx_frames, previous_stats->multicast_tx_frames);
    display_one_cpsw_statistic ("Multicast RX frames         ", current_stats->multicast_rx_frames, previous_stats->multicast_rx_frames);
    display_one_cpsw_statistic ("Broadcast TX frames         ", current_stats->broadcast_tx_frames, previous_stats->broadcast_tx_frames);
    display_one_cpsw_statistic ("Broadcast RX frames         ", current_stats->broadcast_rx_frames, previous_stats->broadcast_rx_frames);
    display_one_cpsw_statistic ("Pause RX frames             ", current_stats->pause_rx_frames, previous_stats->pause_rx_frames);
    display_one_cpsw_statistic ("RX CRC errors               ", current_stats->rx_crc_errors, previous_stats->rx_crc_errors);
    display_one_cpsw_statistic ("RX align/code errors        ", current_stats->rx_align_code_errors, previous_stats->rx_align_code_errors);
    display_one_cpsw_statistic ("Oversize RX frames          ", current_stats->oversize_rx_frames, previous_stats->oversize_rx_frames);
    display_one_cpsw_statistic ("RX jabbers                  ", current_stats->rx_jabbers, previous_stats->rx_jabbers);
    display_one_cpsw_statistic ("Undersize RX frames         ", current_stats->undersize_rx_frames, previous_stats->undersize_rx_frames);
    display_one_cpsw_statistic ("RX fragments                ", current_stats->rx_fragments, previous_stats->rx_fragments);
    display_one_cpsw_statistic ("RX start of frame overruns  ", current_stats->rx_start_of_frame_overruns, previous_stats->rx_start_of_frame_overruns);
    display_one_cpsw_statistic ("RX middle of frame overruns ", current_stats->rx_middle_of_frame_overruns, previous_stats->rx_middle_of_frame_overruns);
    display_one_cpsw_statistic ("RX DMA overruns             ", current_stats->rx_dma_overruns, previous_stats->rx_dma_overruns);
    display_one_cpsw_statistic ("Pause TX frames             ", current_stats->pause_tx_frames, previous_stats->pause_tx_frames);
    display_one_cpsw_statistic ("Collisions                  ", current_stats->collisions, previous_stats->collisions);
    display_one_cpsw_statistic ("Single collision TX frames  ", current_stats->single_collision_tx_frames, previous_stats->single_collision_tx_frames);
    display_one_cpsw_statistic ("Multiple collision TX frames", current_stats->multiple_collision_tx_frames, previous_stats->multiple_collision_tx_frames);
    display_one_cpsw_statistic ("Excessive collisions        ", current_stats->excessive_collisions, previous_stats->excessive_collisions);
    display_one_cpsw_statistic ("TX underruns                ", current_stats->tx_underruns, previous_stats->tx_underruns);
    display_one_cpsw_statistic ("Deferred TX frames          ", current_stats->deferred_tx_frames, previous_stats->deferred_tx_frames);
    display_one_cpsw_statistic ("Carrier sense errors        ", current_stats->carrier_sense_errors, previous_stats->carrier_sense_errors);

}

int main (void)
{
    uint8_t port1_mac_addr[LEN_MAC_ADDRESS];
    uint8_t port2_mac_addr[LEN_MAC_ADDRESS];
    unsigned int phys_alive_status;
    unsigned int phy_address;
    unsigned int phy_id;
    phy_status_t current_phys_status[NUM_PHY_ADDRESSES];
    phy_status_t previous_phys_status[NUM_PHY_ADDRESSES];
    cpsw_statistics_t current_stats;
    cpsw_statistics_t previous_stats;
    unsigned int current_rtc_time;
    unsigned int current_rtc_seconds;
    unsigned int seconds_of_last_statistics;

    memset (current_phys_status, 0, sizeof (current_phys_status));
    memset (previous_phys_status, 0, sizeof (previous_phys_status));
    memset (&current_stats, 0, sizeof (cpsw_statistics_t));
    memset (&previous_stats, 0, sizeof (cpsw_statistics_t));

    UART_setup ();
    RTC_setup ();
    CPSWClkEnable ();
    CPSWPinMuxSetup ();
    EVMPortGMIIModeSelect ();
    CPSWSSReset (SOC_CPSW_SS_REGS);
    CPSWCPDMAReset (SOC_CPSW_CPDMA_REGS);
    CPSWWrReset (SOC_CPSW_WR_REGS);
    MDIOInit (SOC_CPSW_MDIO_REGS, MDIO_FREQ_INPUT, MDIO_FREQ_OUTPUT);
    CPSWALEInit (SOC_CPSW_ALE_REGS);
    CPSWALEPortStateSet (SOC_CPSW_ALE_REGS, 0, CPSW_ALE_PORT_STATE_FWD);
    CPSWALEPortStateSet (SOC_CPSW_ALE_REGS, 1, CPSW_ALE_PORT_STATE_FWD);
    CPSWALEPortStateSet (SOC_CPSW_ALE_REGS, 2, CPSW_ALE_PORT_STATE_FWD);
    CPSWStatisticsEnable (SOC_CPSW_SS_REGS);
    CPSWSlReset (SOC_CPSW_SLIVER_1_REGS);
    CPSWSlReset (SOC_CPSW_SLIVER_2_REGS);
    EVMMACAddrGet (0, port1_mac_addr);
    EVMMACAddrGet (1, port2_mac_addr);
    UARTprintf ("Port 1 MAC address = %02X:%02X:%02X:%02X:%02X:%02X\n",
                port1_mac_addr[5], port1_mac_addr[4], port1_mac_addr[3], port1_mac_addr[2], port1_mac_addr[1], port1_mac_addr[0]);
    UARTprintf ("Port 2 MAC address = %02X:%02X:%02X:%02X:%02X:%02X\n",
                port2_mac_addr[5], port2_mac_addr[4], port2_mac_addr[3], port2_mac_addr[2], port2_mac_addr[1], port2_mac_addr[0]);
    phys_alive_status = MDIOPhyAliveStatusGet (SOC_CPSW_MDIO_REGS);
    UARTprintf ("MDIOPhyAliveStatusGet() = 0x%x\n", phys_alive_status);
    for (phy_address = 0; phy_address < NUM_PHY_ADDRESSES; phy_address++)
    {
        if ((phys_alive_status & (1u << phy_address)) != 0u)
        {
            phy_id = PhyIDGet (SOC_CPSW_MDIO_REGS, phy_address);
            UARTprintf ("Phy %u ID = 0x%x\n", phy_address, phy_id);
        }
    }

    seconds_of_last_statistics = get_rtc_seconds (RTCTimeGet (SOC_RTC_0_REGS));

    read_phy_status (current_phys_status);
    for (;;)
    {
        /* Poll for changes in the status of the Ethernet phys */
        read_phy_status (current_phys_status);
        for (phy_address = 0; phy_address < NUM_PHY_ADDRESSES; phy_address++)
        {
            if (current_phys_status[phy_address].link_speed != previous_phys_status[phy_address].link_speed)
            {
                set_cpsw_transfer_mode (phy_address, &current_phys_status[phy_address]);
            }

            if (memcmp (&current_phys_status[phy_address], &previous_phys_status[phy_address], sizeof (phy_status_t)) != 0)
            {
                UARTprintf ("Phy %u link %s  link speed ", phy_address, current_phys_status[phy_address].link_status ? "Up  " : "Down");
                switch (current_phys_status[phy_address].link_speed)
                {
                case LINK_SPEED_AUTO_NEGOTIATION_NOT_COMPLETE:
                    UARTprintf ("Auto negotiation not complete");
                    break;
                case LINK_SPEED_UNKNOWN:
                    UARTprintf ("Unknown");
                    break;
                case LINK_SPEED_1000M_FULL_DUPLEX:
                    UARTprintf ("1000 Full");
                    break;
                case LINK_SPEED_1000M_HALF_DUPLEX:
                    UARTprintf ("1000M Half");
                    break;
                case LINK_SPEED_100M_FULL_DUPLEX:
                    UARTprintf ("100M Full");
                    break;
                case LINK_SPEED_100M_HALF_DUPLEX:
                    UARTprintf ("100M Half");
                    break;
                case LINK_SPEED_10M_FULL_DUPLEX:
                    UARTprintf ("10M Full");
                    break;
                case LINK_SPEED_10M_HALF_DUPLEX:
                    UARTprintf ("10M Half");
                    break;
                }
                UARTprintf ("\n");
            }
        }
        memcpy (previous_phys_status, current_phys_status, sizeof (previous_phys_status));

        /* Report CPSW statistics every 10 seconds */
        current_rtc_time = RTCTimeGet (SOC_RTC_0_REGS);
        current_rtc_seconds = get_rtc_seconds (current_rtc_time);
        if (current_rtc_seconds == ((seconds_of_last_statistics + 10) % 60))
        {
            get_cpsw_statistics (&current_stats);
            UARTprintf ("\n");
            time_resolve (current_rtc_time);
            UARTprintf (" CPSW Statistics for all ports:\n");
            display_cpsw_statistics (&current_stats, &previous_stats);

            previous_stats = current_stats;
            seconds_of_last_statistics = current_rtc_seconds;
        }
    }

    return 0;
}
