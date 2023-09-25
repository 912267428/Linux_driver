## 初次编译U-Boot

1. 首先安装ncurses：

   ```shell
   sudo apt-get install libncurses5-dev
   ```

2. 将uboot压缩包放入Linux系统并解压

   ```shell
   tar -vxjf uboot-imx-2016.03-xxxx.tar.bz2
   ```

3. 可以添加一个脚本来编译：

   ```shell
   #!/bin/bash
   make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean
   make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mx6ull_14x14_ddr512_emmc_defconfig
   make V=1 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j12
   ```

   但是这样有一个问题：第二行使用了 make 命令用于清理工程，这样不好。所以在make文件中改动，将ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-直接加入makefile文件，编译时直接make即可

4. 将 uboot 烧写到 SD 卡中，然后通过 SD 卡来启动来运行 uboot。

   ```shell
   ./imxdownload u-boot.bin /dev/sdb
   ```

## U-Boot 命令使用

进入 uboot 的命令行模式以后输入“help”或者“？”，然后按下回车即可查看当前 uboot 所
支持的命令

输入“help(或?) 命令名”既可以查看命令的详细用法

### 常用的U-Boot命令

#### 信息查询命令

常用的和信息查询有关的命令有 3 个：bdinfo、printenv 和 version。

##### bdinfo命令

![image-20230920215914745](image\1.png)

如图，bdinfo命令可以查看DRAM 的起始地址和大小、启动参数保存起始地址、波特率、 sp(堆栈指针)起始地址等信息。

##### printenv命令

![image-20230920220147346](image\2.png)

printenv命令用于输出环境变量信息，uboot 也支持 TAB 键自动补全功能，输入“print” 然后按下 TAB 键就会自动补全命令，直接输入“print”也可以

图中可以看到有很多环境变量，uboot 中的环境变量都是字符串，既然叫做环境变量，那么它的作用就和“变量”一样。
bootdelay 这个环境变量就表示 uboot 启动延时时间，默认 bootdelay=3，也就默认延时 3 秒。

##### version命令

![image-20230920220347730](image\3.png)

命令 version 用于查看 uboot 的版本号

#### 环境变量操作命令

##### 修改环境变量

###### setenv命令

用于设置或者修改环境变量的值。

格式：**setenv 环境变量 value**
环境变量可以是已经存在的也可以是目前不存在的，如果是已经存在的环境变量此命名是修改。如果是目前不存在的则是新建然后命名。
value一定是字符串，如果字符串中包含空格则需要用单引号''将字符串包围，否则不需要

如果需要删除环境变量，只需要将其值设置为空即可

###### saveenv命令

用于保存修改后的环境变量

![image-20230922090955685](image\4.png)

一般环境变量都是存放在外部flash中的，uboot启动的时候会将环境变量从flash读取到DRAM中。setenv是修改和设置DRAM中的环境变量值，所以要保存修改后的内容，**一定要使用saveenv命令**将修改后的环境变量保存到flash中。

#### 内存操作命令

可以直接对DRAM进行读写操作的命令。md、nm、mm、mw、cp、cmp

##### md命令

用于显示内存值：
**md[.b, .w, .l] address [# of objects]**

.b，.w，.l分别以1个字节、2个字节和4个字节显示内存值，
address为要查看内存的起始地址
[# of objects]表示要查看数据的长度

**注意uboot命令中的数字都是十六进制的**

![image-20230922092044017](image\5.png)

##### nm命令

用于修改指定内存地址中的值：

**nm [.b, .w, .l] address**

[.b, .w, .l]：指定操作格式，同md
address：指定修改的内存地址

![image-20230922092433600](image\6.png)

按下**q**退出

##### mm命令

也是修改指定内存地址中的值，但是它在修改内存值之后会自增。

![image-20230922093035837](image\7.png)

##### mw命令

使用一个指定的数据填充一段内存：
**mw [.b, .w, .l] address value [count]**

[.b, .w, .l]：指定操作格式，同md
address ：指定修改的内存地址
value：要填充的数据
[count]：填充的长度

![image-20230922094058852](image\8.png)

##### cp命令

数据拷贝命令，将DRAM 中的数据从一段内存拷贝到另一段内存中，或者把 Nor  Flash 中的数据拷贝到 DRAM 中：

**cp [.b, .w, .l] source target count**

![image-20230922102546508](image\9.png)

##### cmp命令

cmp 是比较命令，用于比较两段内存的数据是否相等：

**cmp [.b, .w, .l] addr1 addr2 count**

![image-20230922103241827](image\10.png)

#### 网络操作命令

uboot是支持网络的，我们在移植 uboot 的时候一般都要调通网络功能，因为在移植 linux kernel 的时候需要使用到 uboot 的网络功能做调试。uboot 支持大量的网络相关命令，比如 dhcp、 ping、nfs 和 tftpboot。

| 环境变量  | 描述                                                         |
| --------- | ------------------------------------------------------------ |
| ipaddr    | 开发板 ip 地址，可以不设置，使用 dhcp 命令来从路由器获取 IP 地址。 |
| ethaddr   | 开发板的 MAC 地址，一定要设置。                              |
| gatewayip | 网关地址。                                                   |
| netmask   | 子网掩码。                                                   |
| serverip  | 服务器 IP 地址，也就是 Ubuntu 主机 IP 地址，用于调试代码。   |

在ping前需要设置上表中的环境变量。

**使用网线直连主机，需要将VMware的虚拟机网络选项的桥接模式选择连接开发板的网口并添加网络适配器**

##### ping命令

可以验证开发板的网络能否使用；和和服务器(Ubuntu 主机)的通信是否正常

![image-20230922122116420](image\11.png)

**注意这个ping命令只有uboot用来ping其他机器**

##### dhcp命令

dhcp 用于从路由器获取 IP 地址，前提得开发板连接到路由器上的，如果开发板是和电脑 直连的，那么 dhcp 命令就会失效。
直接输入 dhcp 命令即可通过路由器获取到 IP 地址

##### nfs命令

使用nfs的目的是为了调试程序。

nfs(Network File System)网络文件系统，通过 nfs 可以在计算机之间通过网络来分享资源， 比如我们将 linux 镜像和设备树文件放到 Ubuntu 中，然后在 uboot 中使用 nfs 命令将Ubuntu 中的 linux 镜像和设备树下载到开发板的 DRAM 中。这样做的目的是为了方便调试 linux 镜像和 设备树，也就是网络调试，通过网络调试是 Linux 开发中最常用的调试方法。

我们一般使用 uboot 中的 nfs 命令将 Ubuntu 中的文件下载到开发板的 DRAM 中，在使用 之前需要开启 Ubuntu 主机的 NFS 服务，并且要新建一个 NFS 使用的目录，以后所有要通过 NFS 访问的文件都需要放到这个 NFS 目录中。

![image-20230922123850805](image\12.png)

loadAddress 	是要保存的 DRAM 地址
[[hostIPaddr:]bootfilename]是要下载的文件地址

##### tftp命令

ftp 命令的作用和 nfs 命令一样，都是用于通过网络下载东西到 DRAM 中，只是 tftp 命令 使用的 TFTP 协议。

Ubuntu 主机作为 TFTP 服务器，所以需要在Ubuntu上搭建TFTP服务器
需要安装tftp-hpa 和 tftpd-hpa：

```shell
sudo apt-get install tftp-hpa tftpd-hpa
sudo apt-get install xinetd
```

其他配置TFTP的内容见开发指南P717

**tftpboot [loadAddress] [[hostIPaddr:]bootfilename]**

和 nfs 命令的区别在于，tftp 命令不需要输入文件在 Ubuntu 中的完整路径，只需要输入文件名即可

![image-20230922140919382](image\13.png)

#### EMMC 和 SD 卡操作命令

uboot 支持 EMMC 和 SD 卡，因此也要提供 EMMC 和 SD 卡的操作命令。一般认为 EMMC 和 SD 卡是同一个东西，所以使用 MMC 来代指 EMMC 和 SD 卡。

##### mmc命令

mmc 是一系列的命令，其后可以跟不同的参数

![image-20230922155722642](image\14.png)

|      命令       |                     描述                      |
| :-------------: | :-------------------------------------------: |
|    mmc info     |               输出 MMC 设备信息               |
|    mmc read     |              读取 MMC 中的数据。              |
|    mmc wirte    |             向 MMC 设备写入数据。             |
|   mmc rescan    |                扫描 MMC 设备。                |
|    mmc part     |             列出 MMC 设备的分区。             |
|     mmc dev     |                切换 MMC 设备。                |
|    mmc list     |         列出当前有效的所有 MMC 设备。         |
| mmc hwpartition |             设置 MMC 设备的分区。             |
|  mmc bootbus……  |  设置指定 MMC 设备的 BOOT_BUS_WIDTH 域的值。  |
| mmc bootpart……  | 设置指定 MMC 设备的 boot 和 RPMB 分区的大小。 |
| mmc partconf……  | 设置指定 MMC 设备的 PARTITION_CONFG 域的值。  |
|     mmc rst     |                 复位 MMC 设备                 |
|   mmc setdsr    |             设置 DSR 寄存器的值。             |

#### FAT 格式文件系统操作命令

有时候需要在 uboot 中对 SD 卡或者 EMMC 中存储的文件进行操作，这时候就要用到文件 操作命令，跟文件操作相关的命令有：fatinfo、fatls、fstype、fatload 和 fatwrite，但是这些文件 操作命令只支持 FAT 格式的文件系统

##### fatinfo命令

fatinfo 命令用于查询指定 MMC 设备分区的文件系统信息

![image-20230922171428625](image\16.png)

##### fayls命令

**fatls   <interface> [<dev[:part]>] [directory]**

interface 是要查询的接口，比如 mmc
dev 是要查询的设备号，part 是要查询的分区
directory 是要查询的目录

##### fstype 命令

用于查看 MMC 设备某个分区的文件系统格式

fstype <interface> <dev>:<part>

##### fatload命令

fatload 命令用于将指定的文件读取到 DRAM 中

fatload <interface> [<dev[:part]> [<addr> [<filename> [bytes [pos]]]]]

![image-20230922180921284](image\15.png)

##### fatwrite 命令

uboot 默认没有使能 fatwrite 命令，需要修改板子配置头文件，比如 mx6ullevk.h、 mx6ull_alientek_emmc.h 等等，板子不同，其配置头文件也不同。找到自己开发板对应的配置头 文件然后添加如下一行宏定义来使能 fatwrite 命令：

```c
#define CONFIG_FAT_WRITE /* 使能 fatwrite 命令 */
```

fatwrite <interface> <dev[:part]> <addr> <filename> <bytes>

interface 为接口，比如 mmc，
dev 是设备号，part 是分区，
addr 是要写入的数据在 DRAM 中的起始地址，
filename 是写入的数据文件名字，
bytes 表示要写入多少字节的数据。

#### EXT 格式文件系统操作命令

uboot 有 ext2 和 ext4 这两种格式的文件系统的操作命令，常用的就四个命令，分别为： ext2load、ext2ls、ext4load、ext4ls 和 ext4write。
这些命令的含义和使用与 fatload、fatls 和 fatwrite 一样，只是 ext2 和 ext4 都是针对 ext 文件系统的。

#### BOOT 操作命令

uboot 的本质工作是引导 Linux，所以 uboot 肯定有相关的 boot(引导)命令来启动 Linux。常 用的跟 boot 有关的命令有：bootz、bootm 和 boot。

##### bootz 命令

要启动 Linux，需要先将 Linux 镜像文件拷贝到 DRAM 中，如果使用到设备树的话也需要 将设备树拷贝到 DRAM 中。可以从 EMMC 或者 NAND 等存储设备中将 Linux 镜像和设备树文 件拷贝到 DRAM，也可以通过 nfs 或者 tftp 将 Linux 镜像文件和设备树文件下载到 DRAM 中。 不管用那种方法，只要能将 Linux 镜像和设备树文件存到 DRAM 中就行，然后使用 bootz 命令 来启动

bootz 命令用于启动 zImage 镜像文件:

**bootz [addr [initrd[:size]] [fdt]]**

addr 是 Linux 镜像文件在 DRAM 中的位置
initrd 是 initrd 文件在 DRAM 中的地址，如果不使用 initrd 的话使用‘-’代替即可
fdt 就是设备树文件在 DRAM 中 的地址

##### bootm命令

bootm 和 bootz 功能类似，但是 bootm 用于启动 uImage 镜像文件。如果不使用设备树的话 启动 Linux 内核的命令如下：

**bootm addr**

addr 是 uImage 镜像在 DRAM 中的首地址。



如果要使用设备树，那么 bootm 命令和 bootz 一样，命令格式如下：

**bootm [addr [initrd[:size]] [fdt]]**

其中 addr 是 **uImage** 在 DRAM 中的首地址，initrd 是 initrd 的地址，fdt 是设备树(.dtb)文件 在 DRAM 中的首地址，如果 initrd 为空的话，同样是用“-”来替代。

##### boot 命令

boot 命令也是用来启动 Linux 系统的，只是 boot 会读取环境变量 bootcmd 来启动 Linux 系 统，bootcmd 是一个很重要的环境变量！其名字分为“boot”和“cmd”，也就是“引导”和“命 令”，说明这个环境变量保存着引导命令，其实就是启动的命令集合，具体的引导命令内容是可 以修改的。比如我们要想使用 tftp 命令从网络启动 Linux 那么就可以设置 bootcmd 为“tftp  80800000 zImage; tftp 83000000 imx6ull-14x14-emmc-4.3-800x480-c.dtb; bootz 80800000 - 83000000”，然后使用 saveenv 将 bootcmd 保存起来。然后直接输入 boot 命令即可从网络启动 Linux 系统:

```
setenv bootcmd 'tftp 80800000 zImage; tftp 83000000 imx6ull-14x14-emmc-4.3-800x480-c.dtb; bootz 80800000 - 83000000'
saveenv
boot
```

#### 其他常用命令

uboot 中还有其他一些常用的命令，比如 reset、go、run 和 mtest 等。

##### reset 命令

顾名思义就是复位的，输入“reset”即可复位重启

##### go 命令

go 命令用于跳到指定的地址处执行应用，命令格式如下：

**go addr [arg ...]**

addr 是应用在 DRAM 中的首地址，我们可以编译一下裸机例程的实验 13_printf，然后将编 译出来的 printf.bin 拷贝到 Ubuntu 中的 tftpboot 文件夹里面，注意，这里要拷贝 printf.bin 文件， 不需要在前面添加 IVT 信息，因为 uboot 已经初始化好了 DDR 了。使用 tftp 命令将 printf.bin 下载到开发板 DRAM 的 0X87800000 地址处，因为裸机例程的链接首地址就是 0X87800000， 最后使用 go 命令启动 printf.bin 这个应用，命令如下：

tftp 87800000 printf.bin 
go 87800000

![image-20230922185555060](D:\Program Files(x86)\Linux\Linux_driver\image\17.png)

##### run命令

run 命令用于运行环境变量中定义的命令，比如可以通过“run bootcmd”来运行 bootcmd 中 的启动命令，但是 run 命令最大的作用在于运行我们自定义的环境变量。在后面调试 Linux 系 统的时候常常要在网络启动和 EMMC/NAND 启动之间来回切换，而 bootcmd 只能保存一种启 动方式，如果要换另外一种启动方式的话就得重写 bootcmd，会很麻烦。这里我们就可以通过 自定义环境变量来实现不同的启动方式，比如定义环境变量 mybootemmc 表示从 emmc 启动， 定义 mybootnet 表示从网络启动，定义 mybootnand 表示从 NAND 启动。如果要切换启动方式 的话只需要运行“run mybootxxx(xxx 为 emmc、net 或 nand)”即可。

```
setenv mybootemmc 'fatload mmc 1:1 80800000 zImage; fatload mmc 1:1 83000000 imx6ull-14x14-emmc-4.3-800x480-c.dtb;bootz 80800000 - 83000000'

setenv mybootnet 'tftp 80800000 zImage; tftp 83000000 imx6ull-14x14-emmc-4.3-800x480-c.dtb; bootz 80800000 - 83000000'

saveenv
```

##### mtest 命令

mtest 命令是一个简单的内存读写测试命令，可以用来测试自己开发板上的 DDR，命令格 式如下：

**mtest [start [end [pattern [iterations]]]]**

start 是要测试的 DRAM 开始地址，
end 是结束地址，
比如我们测试 0X80000000~0X80001000 这段内存，输入“mtest 80000000 80001000”

## Uboot启动流程

### 链接脚本和_start的处理

#### 链接脚本：

uboot入口地址为_start  （u-boot.lds中的ENTRY）
__image_copy_start -》 0x87800000
.vetcors  ->  0x87800000 存放中断向量表
arch/arm/cpu/armv7/start.o start.c
_ _image_copy_end -》 0x8785dc6c

__rel_dyn_start ->  0x8785dc6c     rel段
_ _rel_dyn_end -> 0x878668a4

__end -》 0x878668a4

_image_binary_end -》  0x878668a4

__bss_start   -》 0x8785dc6c bss段。_
 _bss_end -》 0x878a8d74

#### reset函数：

跳转到save_boot_params，然后再跳转到save_boot_params_ret：
bicne=bic + ne	如果前面判断相等的语句不相等，那么进行bic运算

1. reset函数目的是将处理器设置为SVC模式，并且关闭FIQ和IRQ.
2. 设置中断向量。
3. 初始化CP15
4. 跳转到lowlevel_init函数

#### lowlevel_init函数

初始化sp为：CONFIG_SYS_INIT_SP_ADDR
在include/configs/ux6ullevk.h中定义：\#define CONFIG_SYS_INIT_SP_ADDR \ (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

\#define CONFIG_SYS_INIT_RAM_ADDR   IRAM_BASE_ADDR

\#define IRAM_BASE_ADDR      0x00900000   (6ULL内部OCRAN地址)

\#define CONFIG_SYS_INIT_SP_OFFSET \
  (CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)

\#define CONFIG_SYS_INIT_RAM_SIZE   IRAM_SIZE
\#define IRAM_SIZE           0x00020000

\#define GENERATED_GBL_DATA_SIZE 256 

0x00900000 + CONFIG_SYS_INIT_SP_OFFSE =>
0x00900000 + CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE =>
0x00900000 **+** 0x00020000 – 256 = 0x0091ff00

设置SP指针、R9寄存器

跳转到s_init函数

#### s_init函数

此函数不是汇编函数，是存放在arch\arm\cpu\armv7\mx6\soc.c中的C语言函数

我们的芯片将直接返回，相当于空函数

将返回lowlevel_init，然后出栈在返回到cpu_init_crit，再返回save_boot_params_ret，然后跳转到_main函数。

#### _main函数

设置 sp 指针为 CONFIG_SYS_INIT_SP_ADDR，也就是 sp 指向 0X0091FF00。

sp 做 8 字节对齐。

读取 sp 到寄存器 r0 里面，此时 r0=0X0091FF00。

调用函数 board_init_f_alloc_reserve，此函数有一个参数，参数为 r0 中的值，也 就是 0X0091FF00，此函数定义在文件 common/init/board_init.c 中

将 r0 写入到 sp 里面，r0 保存着函数 board_init_f_alloc_reserve 的返回值，所以这一句也就是设置 sp=0X0091FA00。主要目的是留出早期的 malloc 内存区域和 gd 内存区域，其中 CONFIG_SYS_MALLOC_F_LEN=0X400( 在 文 件 include/generated/autoconf.h 中定义 ) ， sizeof(struct global_data)=248(GD_SIZE 值)，有返回值的，返回值为新的 top 值

第 89 行，将 r0 寄存器的值写到寄存器 r9 里面，因为 r9 寄存器存放着全局变量 gd 的地址， 在文件 arch/arm/include/asm/global_data.h

uboot 中定义了一个指向 gd_t 的指针 gd，gd 存放在寄存器 r9 里面 的，因此 gd 是个全局变量。gd_t 是个结构体，在 include/asm-generic/global_data.h 里面有定义

因此89行就是设置 gd 所指向的位置，也就是 gd 指向 0X0091FA00。

调用函数 board_init_f_init_reserve，此函数在文件 common/init/board_init.c 中有定义，用于初始化 gd，其实就是清零处理。另外，此函数还设置了 gd->malloc_base 为 gd 基地址+gd 大小=0X0091FA00+248=0X0091FAF8，在做 16 字节对齐，最 终 gd->malloc_base=0X0091FB00，这个也就是 early malloc 的起始地址。

调用 board_init_f 函数，此函数定义在文件 common/board_f.c 中。主要用来初始 化 DDR，定时器，完成代码拷贝等等

重新设置环境(sp 和 gd)、获取 gd->start_addr_sp 的值赋给 sp，在函数 board_init_f 中会初始化 gd 的所有成员变量，其中 gd->start_addr_sp=0X9EF44E90， 所以这里相当于设置 sp=gd->start_addr_sp=0X9EF44E90。0X9EF44E90 是 DDR 中的地址，说明新的 sp 和 gd 将会存 放到 DDR 中，而不是内部的 RAM 了。

获取 gd->bd 的地址赋给 r9，此时 r9 存放的是老的 gd，这里通过获取 gd->bd 的 地址来计算出新的 gd 的位置。GD_BD=0

新的 gd 在 bd 下面，所以 r9 减去 gd 的大小就是新的 gd 的位置，获取到新的 gd 的位置以后赋值给 r9。

设置 lr 寄存器为 here，这样后面执行其他函数返回的时候就返回到了第 122 行 的 here 位置处。

详细参考开发指南P810

#### board_init_f函数

作用：

1. 初始化一系列外设，比如串口、定时器，或者打印一些消息等。
2. 初始化 gd 的各个成员变量，uboot 会将自己重定位到 DRAM 最后面的地址区域，也就 是将自己拷贝到 DRAM 最后面的内存区域中。这么做的目的是给 Linux 腾出空间，防止 Linux kernel 覆盖掉 uboot，将 DRAM 前面的区域完整的空出来。在拷贝之前肯定要给 uboot 各部分 分配好内存位置和大小，比如 gd 应该存放到哪个位置，malloc 内存池应该存放到哪个位置等 等。这些信息都保存在 gd 的成员变量中，因此要对 gd 的这些成员变量做初始化。最终形成一 个完整的内存“分配图”，在后面重定位 uboot 的时候就会用到这个内存“分配图”。

详细见开发指南P812

#### relocate_code 函数

用于代码拷贝，此函数定义在文件 arch/arm/lib/relocate.S。
见开发指南P819

#### relocate_vectors 函数

用于重定位向量表，此函数定义在文件 relocate.S

见开发指南P825

#### board_init_r 函数

在board_init_f函数里面会调用一系列的函数来初始化一些外设和 gd 的成员变量。但是 board_init_f 并没有初始化所有的外设，还需要做一些后续工作，这 些后续工作就是由函数 board_init_r 来完成的，board_init_r 函数定义在文件 common/board_r.c

见开发指南P826

####  run_main_loop 函数

uboot 启动以后会进入 3 秒倒计时，如果在 3 秒倒计时结束之前按下按下回车键，那么就 会进入 uboot 的命令模式，如果倒计时结束以后都没有按下回车键，那么就会自动启动 Linux 内 核 ， 这 个功 能 就 是由 run_main_loop 函 数 来 完成 的 。 run_main_loop 函 数 定义 在 文件 common/board_r.c 中

见开发指南P830

#### cli_loop 函数

cli_loop 函数是 uboot 的命令行处理函数，我们在 uboot 中输入各种命令，进行各种操作就 是有 cli_loop 来处理的，此函数定义在文件 common/cli.c 中

见开发指南P836

#### cmd_process 函数

uboot 使用宏 U_BOOT_CMD 来定义命令，宏 U_BOOT_CMD 定义在文件 include/command.h 中

U_BOOT_CMD 是 U_BOOT_CMD_COMPLETE 的特例，将 U_BOOT_CMD_COMPLETE 的 最 后 一 个 参 数 设 置 成 NULL 就 是 U_BOOT_CMD 。

见开发指南P839

### bootz 启动 Linux 内核过程

#### images 全局变量

不管是 bootz 还是 bootm 命令，在启动 Linux 内核的时候都会用到一个重要的全局变量： images，images 在文件 cmd/bootm.c

```c
bootm_headers_t images; /* pointers to os/initrd/fdt images */
```

images 是 bootm_headers_t 类型的全局变量，bootm_headers_t 是个 boot 头结构体，在文件 include/image.h 中定义。

#### do_bootz 函数

bootz 命令的执行函数为 do_bootz，在文件 cmd/bootm.c中定义

见开发指南P845

#### bootz_start 函数

bootz_srart 函数也定义在文件 cmd/bootm.c 中

见开发指南P846

#### do_bootm_states 函数

do_bootz 最 后 调 用 的 就 是 函 数 do_bootm_states ，而且在 bootz_start 中 也 调 用 了 do_bootm_states 函数.此函数 定义 在文 件 common/bootm.c 中

函数 do_bootm_states 根据不同的 BOOT 状态执行不同的代码段

P开发指南P852.

#### bootm_os_get_boot_func 函数

do_bootm_states 会调用 bootm_os_get_boot_func 来查找对应系统的启动函数，此函数定义 在文件 common/bootm_os.c 中

开发指南P854

#### do_bootm_linux 函数

do_bootm_linux 就是最终启动 Linux 内核的函数，此函数定 义在文件 arch/arm/lib/bootm.c

开发指南P855

![image-20230925160755482](D:\Program Files(x86)\Linux\Linux_driver\image\18.png)

## U-Boot移植

