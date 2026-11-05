// 物理内存布局

// qemu -machine virt 的硬件布局如下，
// 依据 QEMU 的 hw/riscv/virt.c：
//
// 00001000 -- 启动 ROM，由 QEMU 提供
// 02000000 -- 核心本地中断控制器（CLINT）
// 0C000000 -- 平台级中断控制器（PLIC）
// 10000000 -- 串口 uart0 
// 10001000 -- virtio 磁盘 
// 80000000 -- 启动 ROM 在机器模式下跳转到这里
//             -kernel 选项将内核加载到这里
// 80000000 之后是尚未使用的 RAM。

// 内核按如下方式使用物理内存：
// 80000000 -- entry.S，随后是内核代码段和数据段
// end -- 内核页分配区域的起始地址
// PHYSTOP -- 内核所使用 RAM 的结束地址

// QEMU 将 UART 寄存器放在物理内存的这个位置。
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio 内存映射 I/O（MMIO）接口
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#ifdef LAB_NET
#define E1000_IRQ 33
#endif

// 核心本地中断控制器（CLINT），其中包含定时器。
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // 自启动以来的时钟周期数。

// QEMU 将平台级中断控制器（PLIC）放在这里。
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// 内核假定以下物理地址范围内存在 RAM，
// 供内核和用户页使用：
// 从物理地址 0x80000000 到 PHYSTOP。
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)

// 将 trampoline（跳板）页映射到最高地址处，
// 用户地址空间和内核地址空间均如此。
#define TRAMPOLINE (MAXVA - PGSIZE)

// 将内核栈映射到 trampoline 下方，
// 每个内核栈的周围都有无效的保护页。
#define KSTACK(p) (TRAMPOLINE - (p)*2*PGSIZE - 3*PGSIZE)

// 用户内存布局。
// 从地址 0 开始依次为：
//   代码段
//   初始数据段和 BSS 段
//   固定大小的栈
//   可扩展的堆
//   ...
//   USYSCALL（与内核共享）
//   TRAPFRAME（p->trapframe，供 trampoline 使用）
//   TRAMPOLINE（与内核中的 trampoline 是同一个物理页）
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
//#ifdef LAB_PGTBL
#define USYSCALL (TRAPFRAME - PGSIZE)

struct usyscall {
  int pid;  // 进程 ID
};
//#endif
