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

要能运行U-Boot，DDR或者叫DRAM，串口，SD、EMMC、NAND等必须要初始化并成功运行。

NXP官方的U-boot在ALPHA开发板上：

1. uboot能正常启动
2. LCD驱动要根据所使用的屏幕修改。
3. NET初始化失败。

所以需要对NXP官方提供的U-Boot进行修改。

### 修改NXP官方U-boot的步骤：

#### 1、添加开发板默认配置文件：

- 先在 configs 目录下创建默认配置文件，复制 mx6ull_14x14_evk_emmc_defconfig，然后重命名为 mx6ull_alientek_emmc_defconfig
- 修改文件，主要是开发板名称，见开发指南P867

#### 2、添加开发板对应的头文件

- 在 目 录 include/configs 下 添 加 I.MX6ULL-ALPHA 开 发 板 对 应 的 头 文 件 ， 复 制 include/configs/mx6ullevk.h，并重命名为 mx6ull_alientek_emmc.h
- 注意修改文件中防止重导入部分的代码
- mx6ull_alientek_emmc.h 里面有很多宏定义，这些宏定义基本用于配置 uboot，也有一些 I.MX6ULL 的配置项目。如果我们自己要想使能或者禁止 uboot 的某些功能，那就在 mx6ull_alientek_emmc.h 里面做修改即可
- 见开发指南P870

#### 3、添加开发板对应的板级文件夹

- uboot 中每个板子都有一个对应的文件夹来存放板级文件，比如开发板上外设驱动文件等 等。
- NXP 的 I.MX 系列芯片的所有板级文件夹都存放在 board/freescale 目录下，在这个目录下 有个名为 mx6ullevk 的文件夹，这个文件夹就是 NXP 官方 I.MX6ULL EVK 开发板的板级文件 夹。复制 mx6ullevk，将其重命名为 mx6ull_alientek_emmc
- 进 入 mx6ull_alientek_emmc 目 录 中 ， 将 其 中 的 mx6ullevk.c 文 件 重 命 名 为 mx6ull_alientek_emmc.c
- mx6ull_alientek_emmc 目录下的 Makefile 文件（目标文件）。见开发指南P876
- 修改 mx6ull_alientek_emmc 目录下的 imximage.cfg 文件。见开发指南P876
- 修改 mx6ull_alientek_emmc 目录下的 Kconfig 文件。见开发指南P876
- 修改 mx6ull_alientek_emmc 目录下的 MAINTAINERS 文件。见开发指南P877

#### 4、修改U-Boot 图形界面配置文件

见开发指南P877

#### 5、使用新添加的板子配置编译 uboot

见开发指南P878

### 开发板驱动的修改

一般 uboot 中修改驱动基本都是在 xxx.h 和 xxx.c 这两个文件中进行的，xxx 为板子名称， 比如 mx6ull_alientek_emmc.h 和 mx6ull_alientek_emmc.c 这两个文件

#### 1、LCD驱动修改

一般修改 LCD 驱动重点注意以下几点：

- LCD 所使用的 GPIO，查看 uboot 中 LCD 的 IO 配置是否正确。
- LCD 背光引脚 GPIO 的配置。
- LCD 配置参数是否正确。

这里由于使用的开发板与NXP官方 I.MX6ULL 开发板的原理图一致，那么LCD 的 IO 和背光 IO 都一样的，所以 IO 部分就不用修改了。只需要修改LCD参数即可。

具体修改见开发指南P879

#### 2、底板网络驱动修改

使用开发板的PHY芯片为：SR8201F。

修改 ENET1 网络驱动的话重点就三点：

- ENET1 复位引脚初始化。
- SR8201F 的器件 ID。
- SR8201F 驱动

具体修改见开发指南P894

### 其他需要修改的地方

在 uboot 启动信息中会有“Board: MX6ULL 14x14 EVK”这一句，也就是说板子名字为 “MX6ULL 14x14 EVK”，要将其改为我们所使用的板子名字，比如“MX6ULL ALIENTEK  EMMC”或者“MX6ULL ALIENTEK NAND”。打开文件 mx6ull_alientek_emmc.c，找到函数 checkboard，将其中输出的部分修改即可。

## Uboot启动配置

uboot 中有两个非常重要的环境变量 bootcmd 和 bootargs，接下来看一下这两个环境变量。 bootcmd 和 bootagrs 是采用类似 shell 脚本语言编写的，里面有很多的变量引用，这些变量其实 都是环境变量，有很多是 NXP 自 己 定 义 的 。 文 件 mx6ull_alientek_emmc.h 中的宏 CONFIG_EXTRA_ENV_SETTINGS 保存着这些环境变量的默认值。 见开发指南P902

### 环境变量 bootcmd

bootcmd 保存着 uboot 默认命令，uboot 倒计时结束以 后就会执行 bootcmd 中的命令。这些命令一般都是用来启动 Linux 内核的，比如读取 EMMC 或 者 NAND Flash 中的 Linux 内核镜像文件和设备树文件到 DRAM 中，然后启动 Linux 内核。可 以在 uboot 启动以后进入命令行设置 bootcmd 环境变量的值。如果 EMMC 或者 NAND 中没有 保存 bootcmd 的值，那么 uboot 就会使用默认的值，板子第一次运行 uboot 的时候都会使用默 认值来设置 bootcmd 环境变量。

可以在文件include/env_default.h中修改bootcmd的默认值，官方bootcmd分析见开发指南P903

NXP 官方将 CONFIG_BOOTCOMMAND 写的这么复杂只有一个目的：为了兼容多个板子， 所以写了个很复杂的脚本。当我们明确知道我们所使用的板子的时候就可以大幅简化宏 CONFIG_BOOTCOMMAND 的 设 置 ， 比 如 我 们 要 从 EMMC 启动，那么宏 CONFIG_BOOTCOMMAND 就可简化为：

```c
#define CONFIG_BOOTCOMMAND \
 		"mmc dev 1;" \
 		"fatload mmc 1:1 0x80800000 zImage;" \
 		"fatload mmc 1:1 0x83000000 imx6ull-alientek-emmc.dtb;" \
 		"bootz 0x80800000 - 0x83000000;"

```

### 环境变量 bootargs

bootargs 保存着 uboot 传递给 Linux 内核的参数。bootargs 环境变量是由 mmcargs 设置的，mmcargs 环境变量如下：

```
mmcargs=setenv bootargs console=${console},${baudrate} root=${mmcroot}
```

其中 console=ttymxc0，baudrate=115200，mmcroot=/dev/mmcblk1p2 rootwait rw，因此将 mmcargs 展开以后就是：

```
mmcargs=setenv bootargs console= ttymxc0, 115200 root= /dev/mmcblk1p2 rootwait rw
```

可以看出环境变量 mmcargs 就是设置 bootargs 的值为“console= ttymxc0, 115200 root=  /dev/mmcblk1p2 rootwait rw”，bootargs 就是设置了很多的参数的值，这些参数 Linux 内核会使 用到，常用的参数有：

#### console

console 用来设置 linux 终端(或者叫控制台)，也就是通过什么设备来和 Linux 进行交互，是 串口还是 LCD 屏幕？如果是串口的话应该是串口几等等。一般设置串口作为 Linux 终端，这样 我们就可以在电脑上通过 SecureCRT 来和 linux 交互了。这里设置 console 为 ttymxc0，因为 linux 启动以后 I.MX6ULL 的串口 1 在 linux 下的设备文件就是/dev/ttymxc0，在 Linux 下，一切皆文 件。

ttymxc0 后面有个“,115200”，这是设置串口的波特率，console=ttymxc0,115200 综合起来就是设置 ttymxc0（也就是串口 1）作为 Linux 的终端，并且串口波特率设置为 115200。

#### root

root 用来设置根文件系统的位置，root=/dev/mmcblk1p2 用于指明根文件系统存放在 mmcblk1 设备的分区 2 中。EMMC 版本的核心板启动 linux 以后会存在/dev/mmcblk0、 /dev/mmcblk1、/dev/mmcblk0p1、/dev/mmcblk0p2、/dev/mmcblk1p1和/dev/mmcblk1p2 这样的文 件，其中/dev/mmcblkx(x=0~n)表示 mmc 设备，而/dev/mmcblkxpy(x=0~n,y=1~n)表示 mmc 设备 x 的分区 y。在 I.MX6U-ALPHA 开发板中/dev/mmcblk1 表示 EMMC，而/dev/mmcblk1p2 表示 EMMC 的分区 2。

root 后面有“rootwait rw”，rootwait 表示等待 mmc 设备初始化完成以后再挂载，否则的话 mmc 设备还没初始化完成就挂载根文件系统会出错的。rw 表示根文件系统是可以读写的，不加 rw 的话可能无法在根文件系统中进行写操作，只能进行读操作。

#### rootfstype

此选项一般配置 root 一起使用，rootfstype 用于指定根文件系统类型，如果根文件系统为 ext 格式的话此选项无所谓。如果根文件系统是 yaffs、jffs 或 ubifs 的话就需要设置此选项，指 定根文件系统的类型。

### uboot启动Linux

#### 从 EMMC 启动 Linux 系统

从 EMMC 启动也就是将编译出来的 Linux 镜像文件 zImage 和设备树文件保存在 EMMC 中，uboot 从 EMMC 中读取这两个文件并启动，这个是我们产品最终的启动方式。但是我们目 前还没有讲解如何移植 linux 和设备树文件，以及如何将 zImage 和设备树文件保存到 EMMC 中。不过大家拿到手的 I.MX6U-ALPHA 开发板(EMMC 版本)已经将 zImage 文件和设备树文件 烧写到了 EMMC 中，所以我们可以直接读取来测试。先检查一下 EMMC 的分区 1 中有没有 zImage 文件和设备树文件，输入命令“ls mmc 1:1”

设置 bootargs 和 bootcmd 这两个环境变量，设置如下：

```
setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
setenv bootcmd 'mmc dev 1; fatload mmc 1:1 80800000 zImage; fatload mmc 1:1 83000000 imx6ull-alientek-emmc.dtb; bootz 80800000 - 83000000;'
saveenv
```

然后输入boot 或者 run bootcmd即可启动Linux内核

#### 从网络启动 Linux 系统

从网络启动 linux 系统的唯一目的就是为了调试！不管是为了调试 linux 系统还是 linux 下 的驱动。每次修改 linux 系统文件或者 linux 下的某个驱动以后都要将其烧写到 EMMC 中去测 试，这样太麻烦了。我们可以设置 linux 从网络启动，也就是将 linux 镜像文件和根文件系统都 放到 Ubuntu 下某个指定的文件夹中，这样每次重新编译 linux 内核或者某个 linux 驱动以后只 需要使用 cp 命令将其拷贝到这个指定的文件夹中即可，这样就不用需要频繁的烧写 EMMC， 这样就加快了开发速度。我们可以通过 nfs 或者 tftp 从 Ubuntu 中下载 zImage 和设备树文件， 根文件系统的话也可以通过 nfs 挂载，不过本小节我们不讲解如何通过 nfs 挂载根文件系统，这 个在讲解根文件系统移植的时候再讲解。这里我们使用 tftp 从 Ubuntu 中下载 zImage 和设备树 文件，前提是要将 zImage 和设备树文件放到 Ubuntu 下的 tftp 目录中

设置 bootargs 和 bootcmd 这两个环境变量，设置如下：

```
setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
setenv bootcmd 'tftp 80800000 zImage; tftp 83000000 imx6ull-alientek-emmc.dtb; bootz 80800000 - 83000000'
saveenv
```

一开始是通过tftp下载zImage和imx6ull-alientek-emmc.dtb这两个文件，下载完成以后就是启动 Linux 内核

### 总结

1. 不管是购买的开发板还是自己做的开发板，基本都是参考半导体厂商的 dmeo 板，而 半导体厂商会在他们自己的开发板上移植好 uboot、linux kernel 和 rootfs 等，最终制作好 BSP 包提供给用户。我们可以在官方提供的 BSP 包的基础上添加我们的板子，也就是俗称的移植。
2. 我们购买的开发板或者自己做的板子一般都不会原封不动的照抄半导体厂商的 demo 板，都会根据实际的情况来做修改，既然有修改就必然涉及到 uboot 下驱动的移植。
3. 一般 uboot 中需要解决串口、NAND、EMMC 或 SD 卡、网络和 LCD 驱动，因为 uboot 的主要目的就是启动 Linux 内核，所以不需要考虑太多的外设驱动。
4. 在 uboot 中添加自己的板子信息，根据自己板子的实际情况来修改 uboot 中的驱动。
5. 

## U-boot图形化界面配置

make menuconfig以后会匹配到顶层 Makefile 的如下代码：

```makefile
%config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@
```

匹配这条规则之后将读取 scripts/kconfig/Makefile 中的内容，然后运行脚本：
menuconfig: scripts/kconfig/mconf 
	scripts/kconfig/mconf Kconfig

mconf 会调用 uboot 根目录下的 Kconfig 文件开始构建图形配置界面。

详见开发指南P918.

### Kconfig语法简介

scripts/kconfig/mconf 会调用 uboot 根目录下的 Kconfig 文件开始 构建图形化配置界面。

为后面学习 Linux 驱动开发的时候也会涉及到修改 Kconfig。但是不需要要太深入的去研究

Kconfig 的详细语法介绍，可以参考 linux 内核源码

#### 1、mainmenu

mainmenu 就是主菜单，也就是输入“make menuconfig”以后打开的默认界面，顶层代码为：

```
mainmenu "U-Boot $UBOOTVERSION Configuration"
```

定义了一个名为“U-Boot $UBOOTVERSION Configuration”的主菜单，其中UBOOTVERSION=2016.03，因此主菜单名为“U-Boot 2016.03 Configuration。

#### 2、调用其他目录下的 Kconfig 文件

和 makefile 一样，Kconfig 也可以调用其他子目录中的 Kconfig 文件，调用方法如下：

```
source "xxx/Kconfig" //xxx 为具体的目录名，相对路径
```

顶层 Kconfig 文件调用了很多其他子目录下的 Kcofig 文 件，这些子目录下的 Kconfig 文件在主菜单中生成各自的菜单项。

#### 3、menu/endmenu 条目

menu 用于生成菜单，endmenu 就是菜单结束标志，这两个一般是成对出现的。

在“General setup”菜单上面还有 “Architecture select (ARM architecture)”和“ARM  architecture”这两个子菜单，但是在顶层 Kconfig 中并没有看到这两个子菜单对应的 menu/endmenu 代码块，那这两个子菜单是怎么来的呢？这两个子菜单就是 arch/Kconfig 文件生 成的。包括主界面中的“Boot timing”、“Console recording”等等这些子菜单，都是分别由顶层 Kconfig 所调用的 common/Kconfig、cmd/Kconfig 等这些子 Kconfig 文件来创建的

#### 4、config条目

“General setup”子菜单内容如下：

```
menu "General setup"

config LOCALVERSION
	string "Local version - append to U-Boot release"
	help
	  Append an extra string to the end of your U-Boot version.
	  This will show up on your boot log, for example.
	  The string you set here will be appended after the contents of
	  any files with a filename matching localversion* in your
	  object and source tree, in that order.  Your total string can
	  be a maximum of 64 characters.

config LOCALVERSION_AUTO
	bool "Automatically append version information to the version string"
	default y
	help
	  This will try to automatically determine if the current tree is a
	  release tree by looking for git tags that belong to the current
	  top of tree revision.

	  A string of the format -gxxxxxxxx will be added to the localversion
	  if a git-based tree is found.  The string generated by this will be
	  appended after any matching localversion* files, and after the value
	  set in CONFIG_LOCALVERSION.

	  (The actual string used here is the first eight characters produced
	  by running the command:

	    $ git rev-parse --verify HEAD

	  which is done within the script "scripts/setlocalversion".)

config CC_OPTIMIZE_FOR_SIZE
	bool "Optimize for size"
	default y
	help
	  Enabling this option will pass "-Os" instead of "-O2" to gcc
	  resulting in a smaller U-Boot image.

	  This option is enabled by default for U-Boot.

config SYS_MALLOC_F
	bool "Enable malloc() pool before relocation"
	default y if DM
	help
	  Before relocation memory is very limited on many platforms. Still,
	  we can provide a small malloc() pool if needed. Driver model in
	  particular needs this to operate, so that it can allocate the
	  initial serial device and any others that are needed.

config SYS_MALLOC_F_LEN
	hex "Size of malloc() pool before relocation"
	depends on SYS_MALLOC_F
	default 0x400
	help
	  Before relocation memory is very limited on many platforms. Still,
	  we can provide a small malloc() pool if needed. Driver model in
	  particular needs this to operate, so that it can allocate the
	  initial serial device and any others that are needed.

menuconfig EXPERT
	bool "Configure standard U-Boot features (expert users)"
	default y
	help
	  This option allows certain base U-Boot options and settings
	  to be disabled or tweaked. This is for specialized
	  environments which can tolerate a "non-standard" U-Boot.
	  Only use this if you really know what you are doing.

if EXPERT
	config SYS_MALLOC_CLEAR_ON_INIT
	bool "Init with zeros the memory reserved for malloc (slow)"
	default y
	help
	  This setting is enabled by default. The reserved malloc
	  memory is initialized with zeros, so first malloc calls
	  will return the pointer to the zeroed memory. But this
	  slows the boot time.

	  It is recommended to disable it, when CONFIG_SYS_MALLOC_LEN
	  value, has more than few MiB, e.g. when uses bzip2 or bmp logo.
	  Then the boot time can be significantly reduced.
	  Warning:
	  When disabling this, please check if malloc calls, maybe
	  should be replaced by calloc - if expects zeroed memory.
endif
endmenu		# General setup
```

可以看出，在 menu/endmenu 代码块中有大量的“config xxxx”的代码块，也就是 config 条 目。config 条目就是“General setup”菜单的具体配置项

以 “ config LOCALVERSION ” 和 “ config  LOCALVERSION_AUTO”这两个为例来分析一下 config 配置项的语法：

```
config LOCALVERSION
	string "Local version - append to U-Boot release"
	help
	  Append an extra string to the end of your U-Boot version.
	  This will show up on your boot log, for example.
	  The string you set here will be appended after the contents of
	  any files with a filename matching localversion* in your
	  object and source tree, in that order.  Your total string can
	  be a maximum of 64 characters.

config LOCALVERSION_AUTO
	bool "Automatically append version information to the version string"
	default y
	help
	  This will try to automatically determine if the current tree is a
	  release tree by looking for git tags that belong to the current
	  top of tree revision.

	  A string of the format -gxxxxxxxx will be added to the localversion
	  if a git-based tree is found.  The string generated by this will be
	  appended after any matching localversion* files, and after the value
	  set in CONFIG_LOCALVERSION.

	  (The actual string used here is the first eight characters produced
	  by running the command:

	    $ git rev-parse --verify HEAD

	  which is done within the script "scripts/setlocalversion".)
```

- 以 config 关键字开头，后面跟着 LOCALVERSION 和 LOCALVERSION_AUTO，这两个就是配置项名字。
- 如果使能了 LOCALVERSION_AUTO 这个功能，那么就会在.config 文件中生成CONFIG_LOCALVERSION_AUTO
- .config 文件中的“CONFIG_xxx” (xxx 就是具体的配置项名字)就是Kconfig 文件中config关键字后面的配置项名字加上“CONFIG_”前缀。
- config 关键字下面的这几行是配置项属性，属性里面描述了配置项的类型、输入提示、依赖关系、帮 助信息和默认值等。
- string 是变量类型，也就是“CONFIG_ LOCALVERSION”的变量类型。可以为： bool、tristate、string、hex 和 int，一共 5 种。最常用的是 bool、tristate 和 string 这三种，bool 类 型有两种值：y 和 n，当为 y 的时候表示使能这个配置项，当为 n 的时候就禁止这个配置项。 tristate 类型有三种值：y、m 和 n，其中 y 和 n 的涵义与 bool 类型一样，m 表示将这个配置项编 译为模块。string 为字符串类型，所以 LOCALVERSION 是个字符串变量，用来存储本地字符 串，选中以后即可输入用户定义的本地版本号
- string 后面的“Local version - append to U-Boot release”就是这个配置项在图形界面上的显 示出来的标题。
- help 表示帮助信息，告诉我们配置项的含义，当我们按下“h”或“?”弹出来的 帮助界面就是 help 的内容。
- “default y”表示 CONFIG_LOCALVERSION_AUTO 的默认值就是 y，所以这一 行默认会被选中。

#### 5、depends on 和 select

arch/Kconfig 文件，在里面有这如下代码：

```
config SYS_GENERIC_BOARD
	bool
	depends on HAVE_GENERIC_BOARD

choice
	prompt "Architecture select"
	default SANDBOX
	
config ARM
	bool "ARM architecture"
	select CREATE_ARCH_SYMLINK
	select HAVE_PRIVATE_LIBGCC if !ARM64
	select HAVE_GENERIC_BOARD
	select SYS_GENERIC_BOARD
	select SUPPORT_OF_CONTROL
```

- “depends on”说明“SYS_GENERIC_BOARD”项依赖于“HAVE_GENERIC_BOARD”, 也就是说“HAVE_GENERIC_BOARD”被选中以后“SYS_GENERIC_BOARD”才能被选中。
- “select”表示方向依赖，当选中“ARC”以后，“HAVE_PRIVATE_LIBGCC”、 “HAVE_GENERIC_BOARD”、“SYS_GENERIC_BOARD”和“SUPPORT_OF_CONTROL”这 四个也会被选中。

#### 6、choice/endchoice

在 arch/Kconfig 文件中有如下代码：

```
choice
	prompt "Architecture select"
	default SANDBOX

config ARC
	bool "ARC architecture"
	
...

endchoice
```

- choice/endchoice 代码段定义了一组可选择项，将多个类似的配置项组合在一起，供用户单选或者多选。
- 前文代码就是选择处理器架构，可以从 ARC、ARM、AVR32 等这些架构 中选择，这里是单选。

#### 7、menuconfig

menuconfig 和 menu 很类似，但是 menuconfig 是个带选项的菜单，其一般用法为：

```
menuconfig MODULES 
bool "菜单"
if MODULES
...
endif # MODULES
```

- 第 1 行，定义了一个可选的菜单 MODULES，只有选中了 MODULES 第 3~5 行 if 到 endif 之间的内容才会显示

#### 8、comment

comment 用 于 注 释 ， 也 就 是 在 图 形 化 界 面 中 显 示 一 行 注 释 ， 打 开 文 件drivers/mtd/nand/Kconfig，有如下所示代码：

```
config NAND_ARASAN
	bool "Configure Arasan Nand"
	help
	  This enables Nand driver support for Arasan nand flash
	  controller. This uses the hardware ECC for read and
	  write operations.

comment "Generic NAND options"
```

注释内容为：“Generic NAND options”，这行注释 在配置项 NAND_ARASAN 的下面

#### 9、source

source 用于读取另一个 Kconfig，比如

```
source "arch/Kconfig"
```

#### 添加自定义菜单

图形化配置工具的主要工作就是在.config 下面生成前缀为“CONFIG_”的变量，这些变量 一般都要值，为 y，m 或 n，在 uboot 源码里面会根据这些变量来决定编译哪个文件。

自定义菜单要求如下：

1. 在主界面中添加一个名为“My test menu”，此菜单内部有一个配置项。
2. 配置项为“MY_TESTCONFIG”，此配置项处于菜单“My test menu”中。
3. 配置项的为变量类型为 bool，默认值为 y。
4. 配置项菜单名字为“This is my test config”
5. 配置项的帮助内容为“This is a empty config, just for tset!”。

```
menu "My test menu"

config MY_TESTCONFIG
bool "This is my test config"
default y
help
This is a empty config, just for test!

endmenu # my test menu
```

配置项 MY_TESTCONFIG 默认也是被选中的， 因此在.config 文件中肯定会有 “CONFIG_MY_TESTCONFIG=y”这一行

最主要的是记住：**Kconfig 文件的最终目的就是在.config 文件中生成以“CONFIG_”开头的变量。**