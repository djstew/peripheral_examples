/***************************************************************************//**
 * @file main_s0_hg.c
 * @brief Demonstrates USART0 as SPI master.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable 
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_dma.h"
#include "dmactrl.h"

#define TX_BUFFER_SIZE   10
#define RX_BUFFER_SIZE   TX_BUFFER_SIZE

uint8_t TxBuffer[TX_BUFFER_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
uint8_t RxBuffer[RX_BUFFER_SIZE];

/**************************************************************************//**
 * @brief callback function that will trigger after a completed DMA transfer
 *****************************************************************************/
void refreshRxTransfer(uint32_t channelNum,
                     bool isPrimaryDescriptor,
                     void *userPtr)
{
  bool isUseBurst = false;
  DMA_ActivateBasic(channelNum,
                    isPrimaryDescriptor,
                    isUseBurst,
                    (void *) RxBuffer,         // Destination address to transfer to
                    (void *) &USART0->RXDATA,  // Source address to transfer from
                    RX_BUFFER_SIZE - 1);       // Number of DMA transfers minus 1
}


/**************************************************************************//**
 * @brief callback function that will trigger after a completed DMA transfer
 *****************************************************************************/
void refreshTxTransfer(uint32_t channelNum,
                     bool isPrimaryDescriptor,
                     void *userPtr)
{
  bool isUseBurst = false;
  DMA_ActivateBasic(channelNum,
                    isPrimaryDescriptor,
                    isUseBurst,
                    (void *) &USART0->TXDATA,  // Destination address to transfer to
                    (void *) TxBuffer,         // Source address to transfer from
                    RX_BUFFER_SIZE - 1);       // Number of DMA transfers minus 1
}

/**************************************************************************//**
 * @brief
 *    Initialize the DMA module to transfer packets via USART whenever there
 *    is room in the TX Register
 *
 * @note
 *    The callback object needs to at least have static scope persistence so
 *    that the reference to the object is valid beyond its first use in
 *    initialization. This is because the handler needs access to the callback
 *    function. If reference isn't valid anymore, then all dma transfers after
 *    the first one will fail.
 *****************************************************************************/
void initTransferDma(void)
{
  // Callback configuration for TX
  static DMA_CB_TypeDef callbackTX;
  callbackTX.cbFunc = (DMA_FuncPtr_TypeDef) refreshTxTransfer;
  callbackTX.userPtr = NULL;
 
  // Channel configuration for TX transmission
  DMA_CfgChannel_TypeDef channelConfigTX;
  channelConfigTX.highPri   = false;                // Set high priority for the channel
  channelConfigTX.enableInt = true;                 // Interrupt used to reset the transfer
  channelConfigTX.select    = DMAREQ_USART0_TXBL;   // Select DMA trigger
  channelConfigTX.cb        = &callbackTX;
  uint32_t channelNumTX     = 1;
  DMA_CfgChannel(channelNumTX, &channelConfigTX);

  // Channel descriptor configuration for TX transmission
  static DMA_CfgDescr_TypeDef descriptorConfigTX;
  descriptorConfigTX.dstInc  = dmaDataIncNone;  // Destination doesn't move
  descriptorConfigTX.srcInc  = dmaDataInc1;     // Source doesn't move
  descriptorConfigTX.size    = dmaDataSize1;    // Transfer 8 bits each time
  descriptorConfigTX.arbRate = dmaArbitrate1;   // Arbitrate after every DMA transfer
  descriptorConfigTX.hprot   = 0;               // Access level/protection not an issue
  bool isPrimaryDescriptor = true;
  DMA_CfgDescr(channelNumTX, isPrimaryDescriptor, &descriptorConfigTX);

  bool isUseBurst = false;
  DMA_ActivateBasic(channelNumTX,
                    isPrimaryDescriptor,
                    isUseBurst,
                    (void *) &USART0->TXDATA,  // Destination address to transfer to
                    (void *) TxBuffer,         // Source address to transfer from
                    TX_BUFFER_SIZE - 1);       // Number of DMA transfers minus 1
}

/**************************************************************************//**
 * @brief
 *    Initialize the DMA module to receive packets via USART and transfer 
 *    them to a buffer
 *****************************************************************************/
void initReceiveDma(void)
{
  // Callback configuration for RX
  static DMA_CB_TypeDef callbackRX;
  callbackRX.cbFunc = (DMA_FuncPtr_TypeDef) refreshRxTransfer;
  callbackRX.userPtr = NULL;
 
  // Channel configuration for RX transmission
  DMA_CfgChannel_TypeDef channelConfigRX;
  channelConfigRX.highPri   = false;                    // Set high priority for the channel
  channelConfigRX.enableInt = true;                     // Interrupt used to reset the transfer
  channelConfigRX.select    = DMAREQ_USART0_RXDATAV;    // Select DMA trigger
  uint32_t channelNumRX     = 0;
  DMA_CfgChannel(channelNumRX, &channelConfigRX);

  // Channel descriptor configuration for RX transmission
  static DMA_CfgDescr_TypeDef descriptorConfigRX;
  descriptorConfigRX.dstInc  = dmaDataInc1;     // Destination moves one spot in the buffer every transmit
  descriptorConfigRX.srcInc  = dmaDataIncNone;  // Source doesn't move
  descriptorConfigRX.size    = dmaDataSize1;    // Transfer 8 bits each time
  descriptorConfigRX.arbRate = dmaArbitrate1;   // Arbitrate after every DMA transfer
  channelConfigRX.cb        = &callbackRX;
  descriptorConfigRX.hprot   = 0;               // Access level/protection not an issue
  bool isPrimaryDescriptor = true;
  DMA_CfgDescr(channelNumRX, isPrimaryDescriptor, &descriptorConfigRX);

  // Activate basic DMA cycle (used for memory-peripheral transfers)
  bool isUseBurst = false;
  DMA_ActivateBasic(channelNumRX,
                    isPrimaryDescriptor,
                    isUseBurst,
                    (void *) RxBuffer,         // Destination address to transfer to
                    (void *) &USART0->RXDATA,  // Source address to transfer from
                    RX_BUFFER_SIZE - 1);       // Number of DMA transfers minus 1
}
/**************************************************************************//**
 * @brief Initialize USART0
 *****************************************************************************/
void initUSART0 (void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  // Configure GPIO mode
  GPIO_PinModeSet(gpioPortE, 12, gpioModePushPull, 0); // US0_CLK is push pull
  GPIO_PinModeSet(gpioPortE, 13, gpioModePushPull, 1); // US0_CS is push pull
  GPIO_PinModeSet(gpioPortE, 10, gpioModePushPull, 1); // US0_TX (MOSI) is push pull
  GPIO_PinModeSet(gpioPortE, 11, gpioModeInput, 1);    // US0_RX (MISO) is input

  // Start with default config, then modify as necessary
  USART_InitSync_TypeDef config = USART_INITSYNC_DEFAULT;
  config.master       = true;            // master mode
  config.baudrate     = 1000000;         // CLK freq is 1 MHz
  config.autoCsEnable = true;            // CS pin controlled by hardware, not firmware
  config.clockMode    = usartClockMode0; // clock idle low, sample on rising/first edge
  config.msbf         = true;            // send MSB first
  config.enable       = usartDisable;    // making sure USART isn't enabled until we set it up
  USART_InitSync(USART0, &config);

  // Set and enable USART pin locations
  USART0->ROUTE = USART_ROUTE_CLKPEN | USART_ROUTE_CSPEN | USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_LOCATION_LOC0;

  // Enable USART0
  USART_Enable(USART0, usartEnable);
}

/**************************************************************************//**
 * @brief Main function
 *****************************************************************************/
int main(void)
{
  // Initialize chip
  CHIP_Init();

  // Initialize USART0 as SPI slave
  initUSART0();

  // Initializing the DMA
  DMA_Init_TypeDef init;
  init.hprot = 0;                      // Access level/protection not an issue
  init.controlBlock = dmaControlBlock; // Make sure control block is properly aligned
  DMA_Init(&init);

  // Setup LDMA channels for transfer across SPI
  initReceiveDma();
  initTransferDma();

  // Place breakpoint here and observe RxBuffer
  // RxBuffer should contain 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9
  while(1);
}

