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

#ifdef __cplusplus
}
#endif

#endif /* AM3352_SOM_H_ */
