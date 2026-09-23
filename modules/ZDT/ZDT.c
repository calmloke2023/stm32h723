#include "ZDT.h"
#include <stdlib.h>
#include <string.h>

void ZdtBusInit(ZdtBus *bus, UART_HandleTypeDef *uart)
{
  if (bus == NULL || uart == NULL)
  {
    return;
  }

  memset(bus, 0, sizeof(ZdtBus));
  bus->uart = uart;
}

/**
 * @brief Zdt的注册函数
 * 
 * @param config ZdtConfig配置，把bus总线，addr，vel，acc，pos,snF,trans不改变的值初始赋值 
 * @return ZdtInstance* 返回实例，实例中的 上述 已经在配置时赋值
 */
ZdtInstance *ZdtRegister(const ZdtConfig *config)
{
  if (config == NULL ||
      config->bus == NULL ||
      config->bus->uart == NULL)
  {
    return NULL;
  }

  ZdtInstance *instance = malloc(sizeof(*instance));
  if (instance == NULL)
  {
    return NULL;
  }

  memset(instance,0,sizeof(ZdtInstance));  /*清空实例*/

  instance->bus = config->bus;
  instance->addr = config->addr;

  instance->vel = config->vel;
  instance->acc = config->acc;
  instance->position_mode = config->position_mode;
  instance->snF = config->snF;
  instance->trans = config->trans;

  instance->dir = 0;
  instance->distance = 0.0f;

  return instance;  
}

/* 仅释放实例；共享总线及其DMA缓冲区仍由调用方管理。 */
void ZdtUnregister(ZdtInstance *instance)
{
  free(instance);
}

/**
 * @brief 
 * 
 * @param instance 实例
 * @param length transmit需要的参数
 */
void  ZdtTransmit(ZdtInstance *instance,uint16_t length)
{
  HAL_UART_Transmit(instance->bus->uart,instance->bus->tx_buffer,length,HAL_MAX_DELAY);

}


/**
 * @brief 使能函数，给实例使能
 * 
 * @param instance 
 */
void ZdtEnable(ZdtInstance *instance)
{
  uint8_t *zdt_tx_buffer = instance->bus->tx_buffer;

  zdt_tx_buffer[0] = instance->addr;
  zdt_tx_buffer[1] = 0xF3;
  zdt_tx_buffer[2] = 0xAB;
  zdt_tx_buffer[3] = 1; /*使能*/
  zdt_tx_buffer[4] = 0; /*同步*/
  zdt_tx_buffer[5] = 0x6B;

  ZdtTransmit(instance,6);
}


/**
 * @brief 将位置设为0
 * 
 * @param instance 
 */
void ZdtSetCurrentPositionZero(ZdtInstance *instance)
{
  uint8_t *zdt_tx_buffer = instance->bus->tx_buffer;

  zdt_tx_buffer[0] = instance->addr;
  zdt_tx_buffer[1] = 0x0A;
  zdt_tx_buffer[2] = 0x6D;
  zdt_tx_buffer[3] = 0x6B;

  ZdtTransmit(instance,4);
}

/**
 * @brief 位置控制进行运动
 * 
 * @param instance  
 */
void ZdtPositionControl(ZdtInstance *instance)
{
  uint8_t *zdt_tx_buffer = instance->bus->tx_buffer;
  uint32_t pulses = (uint32_t)(instance->distance * instance->trans);

  zdt_tx_buffer[0] = instance->addr;
  zdt_tx_buffer[1] = 0xFD;
  zdt_tx_buffer[2] = instance->dir;

  zdt_tx_buffer[3] = (uint8_t)(instance->vel >> 8);
  zdt_tx_buffer[4] = (uint8_t)(instance->vel);

  zdt_tx_buffer[5] = instance->acc;

  zdt_tx_buffer[6] = (uint8_t)(pulses >> 24);
  zdt_tx_buffer[7] = (uint8_t)(pulses >> 16);
  zdt_tx_buffer[8] = (uint8_t)(pulses >> 8);
  zdt_tx_buffer[9] = (uint8_t)pulses;

  zdt_tx_buffer[10] = (uint8_t)instance->position_mode;
  zdt_tx_buffer[11] = instance->snF;
  zdt_tx_buffer[12] = 0x6B;

  ZdtTransmit(instance, 13);
}

/**
 * @brief 同步运动触发
 * 
 * @param instance 
 */
void ZdtTriggerMotion(ZdtInstance *instance)
{
  uint8_t *zdt_tx_buffer = instance->bus->tx_buffer;

  zdt_tx_buffer[0] = 0x00;
  zdt_tx_buffer[1] = 0xFF;
  zdt_tx_buffer[2] = 0x66;
  zdt_tx_buffer[3] = 0x6B;

  ZdtTransmit(instance, 4);
}


float ZdtRealLocation(ZdtInstance *instance)
{
    ZdtBus *bus = instance->bus;
    uint8_t *rx = bus->rx_buffer;

    bus->tx_buffer[0] = instance->addr;
    bus->tx_buffer[1] = 0x36;
    bus->tx_buffer[2] = 0x6B;

    HAL_UART_Receive_DMA(bus->uart, rx, 8);
    HAL_UART_Transmit(bus->uart, bus->tx_buffer, 3,HAL_MAX_DELAY);

    HAL_Delay(5);

    /* 5ms后仍未接满8字节，表示超时 */
    if (__HAL_DMA_GET_COUNTER(bus->uart->hdmarx) != 0)
    {
        HAL_UART_AbortReceive(bus->uart);
        return NAN;
    }

    uint32_t raw =
        ((uint32_t)rx[3] << 24) |
        ((uint32_t)rx[4] << 16) |
        ((uint32_t)rx[5] << 8)  |
        ((uint32_t)rx[6]);

    float position = (float)raw * 360.0f / 65536.0f;

    return rx[2] == 0x01 ? -position : position;
}