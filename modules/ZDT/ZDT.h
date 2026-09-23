#ifndef ZDT_H
#define ZDT_H

#include "main.h"
#include "usart.h"
#include "stdbool.h"
#include <math.h>


// typedef enum
// {
//     ZDT_OK = 0,
//     ZDT_BUSY,
//     ZDT_ERROR
// } ZdtState;

typedef enum
{
  ZDT_POSITION_RELATIVE_TO_TARGET = 0,
  ZDT_POSITION_ABSOLUTE = 1,
  ZDT_POSITION_RELATIVE_TO_CURRENT = 2
}PosMode;

typedef struct
{
  UART_HandleTypeDef *uart;
  uint8_t tx_buffer[20];
  uint8_t rx_buffer[8];
}ZdtBus;

typedef struct 
{
  ZdtBus *bus;
  uint8_t addr;

  uint16_t vel;
  uint8_t acc;
  PosMode position_mode;
  bool snF;
  float trans;

}ZdtConfig;

typedef struct {
  ZdtBus *bus;
  uint8_t addr;

  uint16_t vel;
  uint8_t acc;
  PosMode position_mode;
  bool snF;

  uint8_t dir;
  float distance;
  float trans;

}ZdtInstance;


void ZdtBusInit(ZdtBus *bus, UART_HandleTypeDef *uart);

/* 从堆分配实例；配置无效或内存不足时返回NULL。bus必须比实例存活更久。 */
ZdtInstance *ZdtRegister(const ZdtConfig *config);

/* 仅接收注册返回的有效实例或NULL；释放后调用方须清空指针，禁止重复释放。 */
void ZdtUnregister(ZdtInstance *instance);

void ZdtEnable(ZdtInstance *instance);

void ZdtSetCurrentPositionZero(ZdtInstance *instance);

void ZdtPositionControl(ZdtInstance *instance);

void ZdtTriggerMotion(ZdtInstance *instance);

float ZdtRealLocation(ZdtInstance *instance);

#endif // !ZDT_H
