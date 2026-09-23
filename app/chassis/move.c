#include "move.h"

static ZdtBus chass_bus;/* 没有* */
static ZdtInstance *motors[4]; /* 顺序：左前、右前、左后、右后 */

void chassisInit(void)
{
  ZdtBusInit(&chass_bus,chass_uart);
  ZdtConfig config = {
    .bus = &chass_bus,
    .vel = 100,
    .acc = 10,
    .position_mode = ZDT_POSITION_RELATIVE_TO_CURRENT,
    .snF = true,
    .trans = scale  /* 按实际参数修改 */
  };
  for(int i=0 ; i<4 ; i++)
  {
    config.addr = i+1;
    motors[i] = ZdtRegister(&config);
    ZdtSetCurrentPositionZero(motors[i]);
    ZdtEnable(motors[i]);
  }
};

void move_front(ElemType distance)
{
  const uint8_t dir[4] = {0, 0, 0, 0};

  for (int i = 0; i < 4; i++)
  {
      motors[i]->distance = distance;
      motors[i]->dir = dir[i];

      ZdtPositionControl(motors[i]);
  }

  ZdtTriggerMotion(motors[0]);
}

void move_left(ElemType distance)
{
  const uint8_t dir[4] = {1, 0, 0, 1};

  for (int i = 0; i < 4; i++)
  {
      motors[i]->distance = distance;
      motors[i]->dir = dir[i];

      ZdtPositionControl(motors[i]);
  }

  ZdtTriggerMotion(motors[0]);
}

void move_right(ElemType distance)
{
  const uint8_t dir[4] = {0, 1, 1, 0};

  for (int i = 0; i < 4; i++)
  {
      motors[i]->distance = distance;
      motors[i]->dir = dir[i];

      ZdtPositionControl(motors[i]);
  }

  ZdtTriggerMotion(motors[0]);
}

void move_back(ElemType distance)
{
  const uint8_t dir[4] = {1, 1, 1, 1};

  for (int i = 0; i < 4; i++)
  {
      motors[i]->distance = distance;
      motors[i]->dir = dir[i];

      ZdtPositionControl(motors[i]);
  }

  ZdtTriggerMotion(motors[0]);
}


