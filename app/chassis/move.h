#ifndef MOVE_H
#define MOVE_H

#include "ZDT.h"
#include "stdio.h"
#include "usart.h"
#include <string.h>

#define chass_uart &huart10
#define scale 1.0
typedef int ElemType;

void chassisInit();
void move_front(ElemType distance);
void move_left(ElemType distance);
void move_right(ElemType distance);
void move_back(ElemType distance);
// void move_rota();



#endif /* MOVE_H */