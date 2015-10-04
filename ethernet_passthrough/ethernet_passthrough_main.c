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
#include <mdio.h>
#include <phy.h>

#define LEN_MAC_ADDRESS 6

/* MDIO input and output frequencies in Hz */
#define MDIO_FREQ_INPUT                          125000000
#define MDIO_FREQ_OUTPUT                         1000000

#define NUM_PHY_ADDRESSES 32u

#define BMSR_ESTATEN        0x0100  /* Extended Status in R15 */

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

int main (void)
{
    uint8_t port1_mac_addr[LEN_MAC_ADDRESS];
    uint8_t port2_mac_addr[LEN_MAC_ADDRESS];
    unsigned int phys_alive_status;
    unsigned int phy_address;
    unsigned int phy_id;
    phy_status_t current_phys_status[NUM_PHY_ADDRESSES];
    phy_status_t previous_phys_status[NUM_PHY_ADDRESSES];

    memset (current_phys_status, 0, sizeof (current_phys_status));
    memset (previous_phys_status, 0, sizeof (previous_phys_status));

    UART_setup ();
    CPSWClkEnable ();
    CPSWPinMuxSetup ();
    MDIOInit (SOC_CPSW_MDIO_REGS, MDIO_FREQ_INPUT, MDIO_FREQ_OUTPUT);
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

    /* Poll for changes in the status of the Ethernet phys */
    read_phy_status (current_phys_status);
    for (;;)
    {
        read_phy_status (current_phys_status);
        for (phy_address = 0; phy_address < NUM_PHY_ADDRESSES; phy_address++)
        {
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
    }

    return 0;
}
