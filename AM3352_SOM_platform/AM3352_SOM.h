/*
 * @file AM3352_SOM.h
 * @date 29 Mar 2015
 * @author Chester Gillon
 * @brief This file contains prototype declarations of functions which performs EVM configurations for the AM3352 SOM.
 */

#ifndef AM3352_SOM_H_
#define AM3352_SOM_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
**                    FUNCTION PROTOTYPES
*****************************************************************************/

void UART0ModuleClkConfig(void);
void UARTPinMuxSetup(unsigned int instanceNum);
void RTCModuleClkConfig(void);
void SysPerfTimerSetup(void);
unsigned int SysPerfTimerConfig(unsigned int flag);
void DMTimer2ModuleClkConfig(void);
void DMTimer3ModuleClkConfig(void);
void DMTimer4ModuleClkConfig(void);
void DMTimer6ModuleClkConfig(void);
void DMTimer7ModuleClkConfig(void);
void DMTimer1msModuleClkConfig(unsigned int clkselect);
unsigned int pmu_get_cycle_count (void);
void enable_cycle_count (void);

#ifdef __cplusplus
}
#endif

#endif /* AM3352_SOM_H_ */
