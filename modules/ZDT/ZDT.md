初步写了关于张大头的modules，一下是介绍，从.h构建实例收起（）

1. 首先是*config*，我们会用*config*一个结构体在*app*以 **ModulesRegister** 函数生成一个 *instance*结构体的实例，并赋初始值。 

2. 其次是*实例*，*instance*中的*config*外的其它参数负责*在app层*被赋值，这是app层做出不同反应的根本

3. 看到.c中，其中的所有**函数**都是*app层*要用到的，其中  **ZdtTransmit**是没用到的（这更感觉是bsp层的任务，但是就一句这个放在modules层也没事），函数的所有参数都是*instance*，方便*app*调用函数无需输入参数（*）

4. app层使用说明： 
  - 1.在*app*写一个 *config*的变量，并给其成员赋值，接着的**ModulesRegister** 会返回一个 *ZdtInstance*变量指针
  - 2.需在*app*生成一个任意名字的*instance*变量，并把上面的 **ModulesRegister**返回值赋值给 该变量
  - 3.依据其它数据及算法得到的dir和distance 修改实例中的dir和distance，并调用运动函数做出反应
  - 4.循环进行第三步
当然，具体操作还是有各种差异的，当前运动模式控制还没写，只写了位置模式控制，而位置控制模式中*position*基本上用的是  ZDT_POSITION_RELATIVE_TO_TARGET,*snF 同步标志模式*也可以进行开关，在app层的config 配置更改就ok了，**ZdtTriggerMotion**函数就是给snF==1准备的

5. 构建思路介绍：
  - 1.最开始，在.h确定Zdt的instance（实例）需要什么参数，以及其中什么参数放到config里面
  - 2.在.c写注册函数 `ZdtRegister`，使用 `malloc(sizeof(ZdtInstance))` 从堆中分配一个实例，检查分配结果，再清零并复制配置。原先固定8个实例的静态数组和计数器已移除，实例数量由可用堆空间决定。配置无效或分配失败时返回 `NULL`。
  - 3.这时Zdt的实例创建好了，开始写控制运动的函数，遇到了一个问题是transmit需要数组，这个数组不好放在实例里，而数组作为static变量放在.c里又会出现所有运动函数都会调用这一个数组的问题，因此额外做了一个参数为 所用uart和对应数组的bus总线
  - 4.bus总线，在app层同样需要注册函数，这里就比较简单了，不做介绍。重要的是由于bus的注册是独立的，因此多个instance可以公用一个bus
  - 5.对于运动函数，其参数只有instance指针，具体实现根据官方文档的函数，将其参数修改，数组改为instace->bus->tx_buffer，且数组的元素 由 instance的成员参数来赋值,
  这也是在app进行操作时只需修改 instance部分成员参数的原因
  - 6.目前的串口函数用的是**HAL_UART_Transmit_DMA**dma理应说要判断下状态，看是否堵塞等，但这里都没做，只写了主干的部分，记得在cubemx开启dma

## 动态内存的使用与释放

实际注册接口为 `ZdtRegister`（上文的 ModulesRegister 指这个接口）。`ZdtConfig` 的字段被复制到实例中，因此配置可以是局部变量；`bus` 只复制指针，总线及UART必须保持有效。

```c
static ZdtBus bus;  /* 多个电机共享，DMA发送期间必须保持有效 */
static ZdtInstance *motor;

/* 在UART初始化后执行一次，重复注册前应先释放旧实例。 */
ZdtBusInit(&bus, &huart1);
ZdtConfig config = {
  .bus = &bus,
  .addr = 1,
  .vel = 100,
  .acc = 10,
  .position_mode = ZDT_POSITION_RELATIVE_TO_TARGET,
  .snF = false,
  .trans = 1.0f  /* 按实际机械参数设置 */
};
motor = ZdtRegister(&config);
if (motor == NULL)
{
  Error_Handler();
  return;  /* 不得将NULL传入运动控制函数 */
}

/* 后续正常使用motor；确定所有使用者不再访问该实例时释放。 */
ZdtUnregister(motor);
motor = NULL;
```

`ZdtUnregister(NULL)` 可以安全调用。释放只回收实例，不会关闭电机、释放共享总线或终止DMA；其它指向该实例的指针也会失效，不能继续使用或再次释放。总线缓冲区仍须保留到DMA发送完成，且不能在发送期间被下一条命令覆盖。

建议在初始化阶段分配，在模块退出时释放，避免在控制循环或中断中频繁调用 `malloc/free`。当前工程链接脚本的 `_Min_Heap_Size` 为 `0x200`（512字节），这是链接时预留的最小堆空间，并非实例数量限制；实际可分配容量还取决于C库的堆实现和RAM占用。

当前 Makefile 未将 `modules/ZDT/ZDT.c` 加入 `C_SOURCES`，也未添加其头文件目录。接入应用时需补充这两项，并确认所用C库的堆扩展接口（如 `_sbrk`）可用且有正确的边界检查；当前工程未提供自定义 `_sbrk`。

## ZdtInstance成员说明

- `bus`：实例所属的ZDT总线，包含UART和DMA发送缓冲区。
- `addr`：电机通信地址。
- `vel`：电机运行速度。
- `acc`：电机加速度。
- `position_mode`：位置运动方式，可选相对目标、绝对位置或相对当前位置。
- `snF`：多机同步标志，开启后需调用`ZdtTriggerMotion`触发运动。
- `dir`：运动方向。
- `distance`：本次运动距离，由App层根据运动学结果更新。
- `trans`：距离到脉冲数的换算系数，脉冲数为`distance * trans`。
