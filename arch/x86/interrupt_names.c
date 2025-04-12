#define IRQ_BASE 0x20      // 重映射后的IRQ基址
#define IRQ_TIMER 0x20     // 定时器中断
#define IRQ_KEYBOARD 0x21  // 键盘中断
#define IRQ_CASCADE 0x22   // PIC级联
#define IRQ_COM2 0x23      // 串口2
#define IRQ_COM1 0x24      // 串口1
#define IRQ_LPT2 0x25      // 并口2
#define IRQ_FLOPPY 0x26    // 软盘控制器
#define IRQ_LPT1 0x27      // 并口1
#define IRQ_RTC 0x28      // CMOS时钟
#define IRQ_PS2      0x29    // 保留中断1
#define IRQ_RESV1      0x29    // 保留中断1
#define IRQ_RESV2      0x2A    // 保留中断2
#define IRQ_PS2_AUX    0x2B    // PS2辅助设备
#define IRQ_FPU        0x2C    // 浮点处理器
#define IRQ_PS2        0x2D    // PS2键盘
#define IRQ_ATA1       0x2E    // 主IDE通道
#define IRQ_ATA2       0x2F    // 从IDE通道
#define IRQ_USER_BASE  0x30    // 用户自定义中断基址
#define IRQ_ETH0       0x30    // 以太网接口0
#define IRQ_ETH1       0x31    // 以太网接口1
#define IRQ_IPI        0x3F    // 处理器间中断

// 定义中断号到中断名称的映射表
const char* interrupt_names[256] = {
    [IRQ_TIMER] = "IRQ 0: Timer interrupt",
    [IRQ_KEYBOARD] = "IRQ 1: Keyboard interrupt",
    [IRQ_CASCADE] = "IRQ 2: Cascade interrupt",
    [IRQ_COM2] = "IRQ 3: Serial Port 2 (COM2) interrupt",
    [IRQ_COM1] = "IRQ 4: Serial Port 1 (COM1) interrupt",
    [IRQ_LPT2] = "IRQ 5: Parallel Port 2 (LPT2) interrupt",
    [IRQ_FLOPPY] = "IRQ 6: Floppy Disk Controller interrupt",
    [IRQ_LPT1] = "IRQ 7: Parallel Port 1 (LPT1) interrupt",
    [IRQ_RTC] = "IRQ 8: CMOS Real - Time Clock interrupt",
    [IRQ_PS2] = "IRQ 9: PS/2 Device (Keyboard or Mouse) interrupt",
    [IRQ_FPU] = "IRQ 10: Floating - Point Unit interrupt",
    [IRQ_ATA1] = "IRQ 11: Primary IDE Channel (ATA1) interrupt",
    [IRQ_ATA2] = "IRQ 12: Secondary IDE Channel (ATA2) interrupt",
    [IRQ_ETH0] = "IRQ 13: Ethernet Interface 0 interrupt",
    [IRQ_ETH1] = "IRQ 14: Ethernet Interface 1 interrupt",
    [IRQ_IPI] = "IRQ 15: Inter - Processor Interrupt",
};