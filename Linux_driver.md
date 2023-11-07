# Linux驱动开发与裸机开发的区别

## 逻辑开发

底层，跟寄存器打交道，有些MCU提供了库。

## Linux驱动开发的思维

1. Linux下驱动开发直接操作寄存器不现实。
2. 根据Linux下的各种驱动框架进行开发。一定要满足框架，也就是Linux下各种驱动框架的掌握。
3. 驱动最终表现就是/dev/xxx文件。打开、关闭、读写、。。。
4. 现在新的内核支持设备树，这个一个.dts文件，此文件 描述了板子的设备信息。

## Linux驱动分类

1. 字符设备驱动。
2. 块设备驱动，存储
3. 网络设备驱动

一个设备不说是一定只属于某一个类型。比如USB WIFI,SDIO WIFI，属于网络设备驱动，因为他又有USB和SDIO，因此也属于字符设备驱动。

# Linux驱动开发

## 字符设备驱动开发

以一个虚拟的设备为例，学习如何进行字符设备驱动开发，以及如何编写测试 APP 来测试驱动工作是否正常。

### 字符设备驱动简介

字符设备就是一个一个字节，按照字节流进行读写操作的设备，读写数据是分先后顺序的。常见的点灯、按键、IIC、SPI， LCD 等等都是字符设备，这些设备的驱动就叫做字符设备驱动。

### Linux 应用程序对驱动程序的调用流程

在 Linux 中一切皆为文件，驱动加载成功以后会在“/dev”目录下生成一个相应的文件，应用程序通过对这个名为“/dev/xxx”(xxx 是具体的驱动文件名字)的文件进行相应的操作即可实 现对硬件的操作。应用程序可以通过各种API函数来操作这个驱动文件以完成打开、关闭以及读写等任务。

应用程序运行在用户空间，而 Linux 驱动属于内核的一部分，因此驱动运行于内核空间。 当我们在用户空间想要实现对内核的操作，比如使用 open 函数打开/dev/led 这个驱动，因为用户空间不能直接对内核进行操作，因此必须使用一个叫做“系统调用”的方法来实现从用户空 间“陷入”（软件中断）到内核空间，这样才能实现对底层驱动的操作。open、close、write 和 read 等这些函数是由 C 库提供的，在 Linux 系统中，系统调用作为 C 库的一部分。

![image-20231005162342014](image\19.png)

关于 C 库以及如何通过系统调用“陷入”到内核空间这个我们不用去管，我们重点关注的是应用程序和具体的驱动，应用程序使用到的函数在具体驱动程序中都有与之对应的函数， 比如应用程序中调用了 open 这个函数，那么在驱动程序中也得有一个名为 open 的函数。
每一个系统调用，在驱动中都有与之对应的一个驱动函数，在 Linux 内核文件 **include/linux/fs.h**中有个叫做 **file_operations** 的结构体，此结构体就是Linux内核驱动操作函数集合。

### 字符设备驱动开发步骤

在 Linux 驱动开发中需要按照其规定的框架来编写驱动，所以说学 Linux 驱动开发重点是学习其驱动框架。

#### 1、驱动模块的加载和卸载

Linux 驱动有两种运行方式：

1. 将驱动编译进 Linux 内核中，这样当 Linux 内核启 动的时候就会自动运行驱动程序
2. 将驱动编译成模块(Linux 下模块扩展名为.ko)，在 Linux 内核启动以后使用“insmod”命令加载驱动模块

在调试驱动的时候一般都选择将其编译 为模块，这样我们修改驱动以后只需要编译一下驱动代码即可，不需要编译整个 Linux 代码。而且在调试的时候只需要加载或者卸载驱动模块即可，不需要重启整个系统。

模块有加载和卸载两种操作，我们在编写驱动的时候需要注册这两种操作函数，模块的加载和 卸载注册函数如下：

```
module_init(xxx_init); //注册模块加载函数
module_exit(xxx_exit); //注册模块卸载函数
```

module_init 函数用来向 Linux 内核注册一个模块加载函数，参数 xxx_init 就是需要注册的具体函数，当使用“insmod”命令加载驱动的时候，xxx_init 这个函数就会被调用。module_exit() 函数用来向 Linux 内核注册一个模块卸载函数，参数 xxx_exit 就是需要注册的具体函数，当使 用“rmmod”命令卸载具体驱动的时候 xxx_exit 函数就会被调用。

驱动编译完成以后扩展名为.ko，有两种命令可以加载驱动模块：insmod和modprobe

这两个命令的区别在于：insmod不能解决模块之间的依赖关系，必须逐个加载模块。
modprobe命令可以解决问题，其会分析模块之间的依赖关系，然后会将所有的依赖模块都加载到内核中

modprobe 命令默认会去 /lib/modules/目录中查找模块，比如这里使用的 Linux kernel 的版本号为 4.1.15， 因此 modprobe 命令默认会到/lib/modules/4.1.15 这个目录中查找相应的驱动模块，一般自己制作的根文件系统中是不会有这个目录的，所以需要自己手动创建。

使用 modprobe 命令可以卸载掉驱动模块所依赖的其他模块，前提是这些依赖模块已经没 有被其他模块所使用，否则就不能使用 modprobe 来卸载驱动模块。所以对于模块的卸载，还是推荐使用 rmmod 命令。

#### 2、字符设备注册于注销

对于字符设备驱动而言，当驱动模块加载成功以后需要注册字符设备，同样，卸载驱动模 块的时候也需要注销掉字符设备。字符设备的注册和注销函数原型如下所示:

```c
static inline int register_chrdev(unsigned int major, const char *name,
									const struct file_operations *fops)
static inline void unregister_chrdev(unsigned int major, const char *name)
```

register_chrdev 函数用于注册字符设备，此函数一共有三个参数，这三个参数的含义如下：

- major：主设备号，Linux 下每个设备都有一个设备号，设备号分为主设备号和次设备号两部分
- name：设备名字，指向一串字符串。
- fops：结构体 file_operations 类型指针，指向设备的操作函数集合变量。

unregister_chrdev 函数用户注销字符设备，此函数有两个参数，这两个参数含义如下：

- major：要注销的设备对应的主设备号。
- name：要注销的设备对应的设备名。

一般字符设备的注册在驱动模块的入口函数 xxx_init 中进行，字符设备的注销在驱动模块 的出口函数 xxx_exit 中进行。

#### 3、实现设备的具体操作函数

file_operations 结构体就是设备的具体操作函数。要使用该结构体中的函数需要对其进行初始化，也就是初始化其中的open、 release、read 和 write 等具体的设备操作函数。

假设对 chrtest 这个设备有如下两个要求：

1. **能够对 chrtest 进行打开和关闭操作**

   设备打开和关闭是最基本的要求，几乎所有的设备都得提供打开和关闭的功能。因此我们需要实现 file_operations 中的 open 和 release 这两个函数。

2. **对 chrtest 进行读写操作**

   假设 chrtest 这个设备控制着一段缓冲区(内存)，应用程序需要通过 read 和 write 这两个函数对 chrtest 的缓冲区进行读写操作。所以需要实现 file_operations 中的 read 和 write 这两个函数。

#### 4、添加 LICENSE 和作者信息

最后我们需要在驱动中加入 LICENSE 信息和作者信息，其中 LICENSE 是必须添加的，否则的话编译的时候会报错，作者信息可以添加也可以不添加。LICENSE 和作者信息的添加使用 如下两个函数：

```
MODULE_LICENSE() //添加模块 LICENSE 信息
MODULE_AUTHOR() //添加模块作者信息
```

### Linux设备号

#### 设备号的组成

为了方便管理，Linux 中每个设备都有一个设备号，设备号由主设备号和次设备号两部分组成，主设备号表示某一个具体的驱动，次设备号表示使用这个驱动的各个设备。

Linux 提供了 一个名为 dev_t 的数据类型表示设备号，dev_t 定义在文件 include/linux/types.h 里面，定义如下：

```
typedef __u32 __kernel_dev_t;
......
typedef __kernel_dev_t dev_t;
```

dev_t 是__u32 类型的，而__u32 定义在文件 include/uapi/asm-generic/int-ll64.h 里面：

typedef unsigned int __u32;

dev_t 其实就是 unsigned int 类型，是一个 32 位的数据类型。这 32 位的数据构成了主设备号和次设备号两部分，其中高 12 位为主设备号，低 20 位为次设备号。因此 Linux 系统中主设备号范围为 0~4095，所以在选择主设备号的时候一定不要超过这个范围。在文件 include/linux/kdev_t.h 中提供了几个关于设备号的操作函数(本质是宏)，如下所示：

```
#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)

#define MAJOR(dev) ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))
```

#### 设备号的分配

##### 1、静态分配设备号

本小节讲的设备号分配主要是主设备号的分配。前面讲解字符设备驱动的时候说过了，注 册字符设备的时候需要给设备指定一个设备号，这个设备号可以是驱动开发者静态的指定一个 设备号，比如选择 200 这个主设备号。有一些常用的设备号已经被 Linux 内核开发者给分配掉 了，具体分配的内容可以查看文档 Documentation/devices.txt。并不是说内核开发者已经分配掉 的主设备号我们就不能用了，具体能不能用还得看我们的硬件平台运行过程中有没有使用这个 主设备号，使用“cat /proc/devices”命令即可查看当前系统中所有已经使用了的设备号。

##### 2、动态分配设备号

静态分配设备号需要我们检查当前系统中所有被使用了的设备号，然后挑选一个没有使用 的。而且静态分配设备号很容易带来冲突问题，Linux 社区推荐使用动态分配设备号，在注册字 符设备之前先申请一个设备号，系统会自动给你一个没有被使用的设备号，这样就避免了冲突。 卸载驱动的时候释放掉这个设备号即可，设备号的申请函数如下：

```
int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name)
```

函数 alloc_chrdev_region 用于申请设备号，此函数有 4 个参数：

1. dev：保存申请到的设备号。
2. baseminor：次设备号起始地址，alloc_chrdev_region 可以申请一段连续的多个设备号，这 些设备号的主设备号一样，但是次设备号不同，次设备号以 baseminor 为起始地址地址开始递 增。一般 baseminor 为 0，也就是说次设备号从 0 开始。
3. count：要申请的设备号数量。
4. name：设备名字。

注销字符设备之后要释放掉设备号，设备号释放函数如下：

```
void unregister_chrdev_region(dev_t from, unsigned count)
```

两个参数：

1. from：要释放的设备号。
2. count：表示从 from 开始，要释放的设备号数量。

### chrdevbase 字符设备驱动开发实验

以 chrdevbase 这个虚拟设备为例，完整的编写一个字符设备驱动模块。chrdevbase 不是实际存在的一个设备，是为了方便讲解字符设备的开发而引入的一个虚拟设备。chrdevbase 设备有两个缓冲区，一个为读缓冲区，一个为写缓冲区，这两个缓冲区的大小都为 100 字节。在应用程序中可以向 chrdevbase 设备的写缓冲区中写入数据，从读缓冲区中读取数据。chrdevbase 这个虚拟设备的功能很简单，但是它包含了字符设备的最基本功能。

#### 1、准备工作

创建VSCode工程并给工程命名

添加头文件路径，需要用到Linux 源码中的函数，所以需要添加 Linux 源码中的头文件路径。
已有c_cpp_properties.json文件。

#### 2、编写驱动程序

新建 chrdevbase.c，完成驱动的编写

主要是实现各个函数然后初始化给到设备操作函数结构体

详见开发指南P1070

#### 3、编写测试APP

编写测试 APP 就是编写 Linux 应用，需要用到 C 库里面和文件操作有关的一些函数，比如 open、read、write 和 close 这四个函数。

详见开发指南P1072

#### 4、编译驱动程序和测试App

```makefile
KERNELDIR := /home/rodney/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CURRENT_PATH := $(shell pwd)
obj-m := chrdevbase.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
```

编译测试 APP：

arm-linux-gnueabihf-gcc chrdevbaseApp.c -o chrdevbaseApp

#### 5、运行测试

1. 网络启动并将文件拷贝到rootfs/lib/modules/4.1.15中

2. 加载 chrdevbase.ko 驱动文件

   insmod chrdevbase.ko   或    modprobe chrdevbase.ko
   可能遇到modprobe 提示无法打开“modules.dep”这个文件，直接运行depmod即可

3. 创建设备节点文件

   mknod /dev/chrdevbase c 200 0

4. chrdevbase 设备操作测试

![image-20231005221454487](image\20.png)

### LED灯驱动实验见开发指南41、42章

### 设备树

描述设备树的文件叫做 DTS(Device  Tree Source)，这个 DTS 文件采用**树形结**构描述**板级设备**，也就是开发板上的设备信息，比如 CPU 数量、 内存基地址、IIC 接口上接了哪些设备、SPI 接口上接了哪些设备等等

![image-20231007134748658](image\21.png)

树的主干就是系统总线，IIC 控制器、GPIO 控制器、SPI 控制器等都是接 到系统主线上的分支。IIC 控制器有分为 IIC1 和 IIC2 两种，其中 IIC1 上接了 FT5206 和 AT24C02 这两个 IIC 设备，IIC2 上只接了 MPU6050 这个设备。DTS 文件的主要功能就是按照上图所示的结构来描述板子上的设备信息，DTS 文件描述设备信息是有相应的语法规则要求。

#### 为什么使用设备树

在 Linux 内核源码中 大量的 arch/arm/mach-xxx 和 arch/arm/plat-xxx 文件夹，这些文件夹里面的文件就是对应平台下的板级信息。

这样的方式会导致linux内核中出现大量重复、冗余的文件。所以引入设备树将这些描述板级硬件信息的内容都从 Linux 内核中分离开来，用一个专属的文件格式来描述。文件扩展名为.dts

一个 SOC 可以作出很多不同的板子，这些不同的板子肯 定是有共同的信息，将这些共同的信息提取出来作为一个通用的文件，其他的.dts 文件直接引 用这个通用文件即可，这个通用文件就是.dtsi 文件，类似于 C 语言中的头文件。一般.dts 描述 板级信息(也就是开发板上有哪些 IIC 设备、SPI 设备等)，.dtsi 描述 SOC 级信息(也就是 SOC 有 几个 CPU、主频是多少、各个外设控制器信息等)。

#### DTS、DTB 和 DTC

DTS 是设备树源码文件，DTB 是将 DTS 编译以后得到的二进制文件。

将DTS文件编译得到DTB文件的工具是DTC

DTC 工具源码在 Linux 内核的 scripts/dtc 目录下， scripts/dtc/Makefile 文件内容如下：

```makefile
hostprogs-y := dtc
always := $(hostprogs-y)

dtc-objs:= dtc.o flattree.o fstree.o data.o livetree.o treesource.o \
srcpos.o checks.o util.o
dtc-objs += dtc-lexer.lex.o dtc-parser.tab.o
......
```

DTC 工具依赖于 dtc.c、flattree.c、fstree.c 等文件，最终编译并链接出 DTC 这 个主机文件。如果要编译 DTS 文件的话只需要进入到 Linux 源码根目录下，然后执行如下命令：	make all    或者    make dtbs

使用 I.MX6ULL 新做 了一个板子，只需要新建一个此板子对应的.dts 文件，然后将对应的.dtb 文件名添加到 dtb- $(CONFIG_SOC_IMX6ULL)下，这样在编译设备树的时候就会将对应的.dts 编译为二进制的.dtb 文件。

#### DTS 语法

详细语法规参考《 Devicetree SpecificationV0.2.pdf 》 和 《Power_ePAPR_APPROVED_v1.12.pdf》。

##### .dtsi 头文件

和 C 语言一样，设备树也支持头文件，设备树的头文件扩展名为.dtsi。

dts文件可以像C语言哪有使用#include引入dtsi头文件、dts文件甚至是.h头文件

一般.dtsi 文件用于描述 SOC 的内部外设信息，比如 CPU 架构、主频、外设寄存器地址范 围，比如 UART、IIC 等等。比如 imx6ull.dtsi 就是描述 I.MX6ULL 这颗 SOC 内部外设情况信息的。

#####  设备节点

设备树是采用树形结构来描述板子上的设备信息的文件，每个设备都是一个节点，叫做设 备节点，每个节点都通过一些属性信息来描述节点信息，属性就是键—值对。如：

```dts
/ {
aliases {
can0 = &flexcan1;
};

cpus {
#address-cells = <1>;
#size-cells = <0>;

cpu0: cpu@0 {
compatible = "arm,cortex-a7";
device_type = "cpu";
reg = <0>;
};
};

intc: interrupt-controller@00a01000 {
compatible = "arm,cortex-a7-gic";
#interrupt-cells = <3>;
interrupt-controller;
reg = <0x00a01000 0x1000>,
<0x00a02000 0x100>;
};
}

```

- “/”是根节点，每个设备树文件只有一个根节点。

- aliases、cpus 和 intc 是三个子节点，在设备树中节点命名格式如下：

  **node-name@unit-address**
  “node-name”是节点名字，为 ASCII 字符串，节点名字应该能够清晰的描述出节点的 功能，比如“uart1”就表示这个节点是 UART1 外设。
  “unit-address”一般表示设备的地址或寄存器首地址，如果某个节点没有地址或者寄存器的话“unit-address”可以不要，比如“cpu@0”、 “interrupt-controller@00a01000”

  另外一种节点命名格式：

  **label: node-name@unit-address**
  label是节点标签，引入 label 的目的就是为了方便访问节点，可以直接通过&label 来访问这个节点，比如通过 &cpu0 就可以访问“cpu@0”这个节点，而不需要输入完整的节点名字

- cpu0 也是一个节点，只是 cpu0 是 cpus 的子节点。

- 每个节点都有不同属性，不同的属性又有不同的内容，属性都是键值对，值可以为空或任意的字节流。设备树源码中常用的几种**数据形式**如下所示：

  - 字符串

    compatible = "arm,cortex-a7";

  - 32 位无符号整数

    reg = <0>;

  - 字符串列表

    compatible = "fsl,imx6ull-gpmi-nand", "fsl, imx6ul-gpmi-nand";
    属性值也可以为字符串列表，字符串和字符串之间采用“,”隔开

##### 标准属性

节点是由一堆的属性组成，节点都是具体的设备，不同的设备需要的属性不同，用户可以 自定义属性。除了用户自定义属性，有很多属性是标准属性，Linux 下的很多外设驱动都会使用 这些标准属性

###### 1、compatible 属性

描述：		“兼容性属性”，非常重要
作用：		用于将设备和驱动绑定起来，字符串列表用于选择设备所要 使用的驱动程序
值类型：	字符串列表
值格式：	"manufacturer,model"
	manufacturer 表示厂商，model 一般是模块对应的驱动名字。
	当字符串列表中有多个值时，首先使用第一个兼容值在 Linux 内核里面查找，看看能不能找到与之匹配的驱动文件， 如果没有找到的话就使用第二个兼容值查。
	一般驱动程序文件都会有一个 OF 匹配表，此 OF 匹配表保存着一些 compatible 值，如果设 备节点的 compatible 属性值和 OF 匹配表中的任何一个值相等，那么就表示设备可以使用这个 驱动。

###### 2、model属性

描述：		描述设备模块信息，比如名字
作用：		-
值类型：	字符串
值格式：	eg：model = "wm8960-audio";

###### 3、status属性

描述：		和设备状态有关的属性
作用：		描述设备状态信息，如是否可操作
值类型：	字符串
值格式：	值是字符串可选状态见下表：
		

|     值     |                             描述                             |
| :--------: | :----------------------------------------------------------: |
|   “okay”   |                     表明设备是可操作的。                     |
| “disabled” | 表明设备当前是不可操作的，但是在未来可以变为可操作的，比如热插拔设备 插入以后。至于 disabled 的具体含义还要看设备的绑定文档。 |
|   “fail”   | 表明设备不可操作，设备检测到了一系列的错误，而且设备也不大可能变得可 操作。 |
| “fail-sss” |    含义和“fail”相同，后面的 sss 部分是检测到的错误内容。     |

###### 4、\#address-cells 和#size-cells 属性

描述：		可以用在任何拥有子节点的设备中
作用：		用于描述子节点的地址信息
					\#address-cells 属性值决定了子节点 reg 属 性中地址信息所占用的字长(32 位)
					\#size-cells 属性值决定了子节点 reg 属性中长度信息所占的字长(32为)
值类型：	两个属性的值都是无符号 32 位整形
\#address-cells 和#size-cells 表明了子节点应该如何编写 reg 属性值，一般 reg 属性 都是和地址有关的内容，和地址相关的信息有两种：起始地址和地址长度，reg 属性的格式一为：
**reg = <address1 length1 address2 length2 address3 length3……>**

每个“address length”组合表示一个地址范围，其中 address 是起始地址，length 是地址长度，#address-cells 表明 address 这个数据所占用的字长，#size-cells 表明 length 这个数据所占用 的字长

###### 5、reg属性

描述：		值一般是(address，length)对
作用：		用于描述设备地址空间资源信息，一般都是某个外设的寄存器地址范围信息。
值类型：	无符号整数
值格式见前文

###### 6、ranges 属性

描述：		可以为空或者按照一定格式编写数字矩阵
作用：		ranges 是一个地址映射/转换表
值类型：	无符号整数
值格式：	ranges = <child-bus-address	parent-bus-address	length>;
				child-bus-address：子总线地址空间的物理地址，由父节点的#address-cells 确定此物理地址 所占用的字长。	
				parent-bus-address：父总线地址空间的物理地址，同样由父节点的#address-cells 确定此物 理地址所占用的字长。
				length：子地址空间的长度，由父节点的#size-cells 确定此地址长度所占用的字长。

如果 ranges 属性值为空值，说明子地址空间和父地址空间完全相同，不需要进行地址转换

###### 7、name 属性

name 属性值为字符串，name 属性用于记录节点名字，name 属性已经被弃用，不推荐使用 name 属性，一些老的设备树文件可能会使用此属性。

###### 8、device_type 属性

device_type 属性值为字符串，IEEE 1275 会用到此属性，用于描述设备的 FCode，但是设 备树没有 FCode，所以此属性也被抛弃了。此属性只能用于 cpu 节点或者 memory 节点。

##### 根节点 compatible 属性

每个节点都有 compatible 属性，根节点“/”也不例外。imx6ull-alientek-emmc.dts 文件中根 节点的 compatible 属性内容如下所示：



```dtd
/ {
model = "Freescale i.MX6 ULL 14x14 EVK Board";
compatible = "fsl,imx6ull-14x14-evk", "fsl,imx6ull";
...
	}
```

根节点的 compatible 属性可以知道我们所使用的设备，一般第一个值描述了所使用的硬件设备名字，比如这里使用的是“imx6ull-14x14-evk”这个设备，第二 个值描述了设备所使用的 SOC，比如这里使用的是“imx6ull”这颗 SOC。Linux 内核会通过根节点的 compoatible 属性查看是否支持此设备，如果支持的话设备就会启动 Linux 内核

###### 使用设备树之前设备匹配方法

在没有使用设备树以前，uboot 会向 Linux 内核传递一个叫做 machine id 的值，machine id 也就是设备 ID，告诉 Linux 内核自己是个什么设备，看看 Linux 内核是否支持。Linux 内核是 支持很多设备的，针对每一个设备(板子)，Linux内核都用MACHINE_START和MACHINE_END 来定义一个 machine_desc 结构体来描述这个设备

详见开发指南P1124

###### 使用设备树以后的设备匹配方法

当 Linux 内 核 引 入 设 备 树 以 后 就 不 再 使 用 MACHINE_START 了 ， 而 是 换 为 了 DT_MACHINE_START。DT_MACHINE_START 也定义在文件 arch/arm/include/asm/mach/arch.h 里面.

DT_MACHINE_START 和 MACHINE_START 基本相同，只是.nr 的设置不同， 在 DT_MACHINE_START 里面直接将.nr 设置为~0。说明引入设备树以后不会再根据 machine id 来检查 Linux 内核是否支持某个设备了。

machine_desc 结构体中有个.dt_compat 成员变量，此成员变量保存着本设备兼容属性，源码中设置.dt_compat = imx6ul_dt_compat，imx6ul_dt_compat 表里面有"fsl,imx6ul" 和"fsl,imx6ull"这两个兼容值。只要某个设备(板子)根节点“/”的 compatible 属性值与 imx6ul_dt_compat 表中的任何一个值相等，那么就表示 Linux 内核支持此设备。前文中imx6ull-alientek-emmc.dts 文件中根 节点的 compatible 属性为：compatible = "fsl,imx6ull-14x14-evk", "fsl,imx6ull";
其中“fsl,imx6ull”与 imx6ul_dt_compat 中的“fsl,imx6ull”匹配，因此 I.MX6U-ALPHA 开 发板可以正常启动 Linux 内核。

**根据设备树根节点的 compatible 属性来匹配出对 应的 machine_desc**：

1. Linux 内核调用 start_kernel 函数来启动内核，
2. start_kernel 函数会调用 setup_arch 函数来匹配 machine_desc，setup_arch 函数定义在文件 arch/arm/kernel/setup.c 中
3. setup_arch调用setup_machine_fdt 函数来获取匹配的 machine_desc，参数就是 atags 的首地址，也就是 uboot 传递给 Linux 内核的 dtb 文件首地址，setup_machine_fdt 函数的返回值就是 找到的最匹配的 machine_desc。setup_machine_fdt 定义在文件 arch/arm/kernel/devtree.c 中
4. setup_machine_fdt 调用of_flat_dt_match_machine 来获取匹配的 machine_desc。
   参数 mdesc_best 是默认的 machine_desc ，
   参数 arch_get_next_mach 是个函数，此函数定义在定义在 arch/arm/kernel/devtree.c 文件中。
   找到匹配的 machine_desc 的过程就是用设备树根节点的 compatible 属性值和 Linux 内核中 machine_desc 下.dt_compat 的值比较，看看那个相等，如果相 等的话就表示找到匹配的 machine_desc，arch_get_next_mach 函数的工作就是获取 Linux 内核中 下一个 machine_desc 结构体。
   of_flat_dt_match_machine 函数定义在文件 drivers/of/fdt.c 中。通过函数 of_get_flat_dt_root 获取设备树根节点。of_flat_dt_match 函 数会将根节点 compatible 属性的值和每个 machine_desc 结构体中. dt_compat 的值进行比较，直 至找到匹配的那个 machine_desc。

Linux 内核通过根节点 compatible 属性找到对应的设备的函数调用过程，如图：

![image-20231007162323675](image\22.png)

##### 向节点追加或修改内容

当需要向设备树文件中添加硬件时，最简单的方法就是在需要添加的设备的父设备节点下创建一个子节点。但是这样的方式存在一个严重的问题，当父设备节点定义在dtsi文件中时，其代表所有使用这个SOC的板子，如果在dtsi文件中修改则表示在所有使用这个SOC的板子上都添加了这个设备。所以应该使用向节点追加数据的方式来添加子节点。

eg：

```
&i2c1 {
/* 要追加或修改的内容 */
};
```

&i2c1 表示要访问 i2c1 这个 label 所对应的节点

打开 imx6ull-alientek-emmc.dts，可以找到如下内容：

```
&i2c1 {
clock-frequency = <100000>;
pinctrl-names = "default";
pinctrl-0 = <&pinctrl_i2c1>;
status = "okay";

mag3110@0e {
compatible = "fsl,mag3110";
reg = <0x0e>;
position = <2>;
};

fxls8471@1e {
compatible = "fsl,fxls8471";
reg = <0x1e>;
position = <0>;
interrupt-parent = <&gpio5>;
interrupts = <0 8>;
};
};
```

- “clock-frequency” 就表示 i2c1 时钟为 100KHz。“clock-frequency”就是新添加的属性。
- 将 status 属性的值由原来的 disabled 改为 okay。
- i2c1 子节点 mag3110，因为 NXP 官方开发板在 I2C1 上接了一个磁力计芯 片 mag3110，I.MX6U-ALPHA 开发板并没有使用 mag3110。
- i2c1 子节点 fxls8471，同样是因为 NXP 官方开发板在 I2C1 上接了 fxls8471 这颗六轴芯片。

#### 创建小型模板设备树

详见开发指南第43章

#### 设备树常用 OF 操作函数

设备树描述了设备的详细信息，这些信息包括数字类型的、字符串类型的、数组类型的， 我们在编写驱动的时候需要获取到这些信息。比如设备树使用 reg 属性描述了某个外设的寄存 器地址为 0X02005482，长度为 0X400，我们在编写驱动的时候需要获取到 reg 属性的0X02005482 和 0X400 这两个值，然后初始化外设。Linux 内核给我们提供了一系列的函数来获 取设备树中的节点或者属性信息，这一系列的函数都有一个统一的前缀“of_”，所以在很多资 料里面也被叫做 OF 函数。这些 OF 函数原型都定义在 include/linux/of.h 文件中。

##### device_node 结构体

设备都是以节点的形式“挂”到设备树上的，因此要想获取这个设备的其他属性信息，必 须先获取到这个设备的节点。Linux 内核使用 device_node 结构体来描述一个节点，此结构体定 义在文件 include/linux/of.h 中，定义如下：

```c
struct device_node {
	const char *name; /* 节点名字 */
	const char *type; /* 设备类型 */
	phandle phandle;
	const char *full_name; /* 节点全名 */
	struct fwnode_handle fwnode;
	
	struct property *properties; /* 属性 */
	struct property *deadprops; /* removed 属性 */
	struct device_node *parent; /* 父节点 */
	struct device_node *child; /* 子节点 */
	struct device_node *sibling;
	struct kobject kobj;
	unsigned long _flags;
	void *data;
	#if defined(CONFIG_SPARC)
	const char *path_component_name;
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
	#endif
};
```

##### 查找节点的 OF 函数

与查找节点有关的 OF 函数有 5 个

- **of_find_node_by_name 函数**

  of_find_node_by_name 函数通过节点名字查找指定的节点，函数原型如下：

  ```c
  struct device_node *of_find_node_by_name(struct device_node *from, const char *name);
  ```

  **from：**开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  **name：**要查找的节点名字。
  **返回值：**找到的节点，如果为 NULL 表示查找失败。

- **of_find_node_by_type 函数**

  of_find_node_by_type 函数通过 device_type 属性查找指定的节点，函数原型如下：

  ```c
  struct device_node *of_find_node_by_type(struct device_node *from, const char *type)
  ```

  **from：**开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  **type：**要查找的节点对应的 type 字符串，也就是 device_type 属性值。 
  **返回值：**找到的节点，如果为 NULL 表示查找失败。

- **of_find_compatible_node 函数**

  of_find_compatible_node 函数根据 device_type 和 compatible 这两个属性查找指定的节点， 函数原型如下：

  ```c
  struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compatible)
  ```

  **from：**开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  **type：**要查找的节点对应的 type 字符串，也就是 device_type 属性值，可以为 NULL，表示 忽略掉 device_type 属性。 
  **compatible：**要查找的节点所对应的 compatible 属性列表。
  **返回值：**找到的节点，如果为 NULL 表示查找失败

- **of_find_matching_node_and_match 函数**

  of_find_matching_node_and_match 函数通过 of_device_id 匹配表来查找指定的节点，函数原 型如下：

  ```c
  struct device_node *of_find_matching_node_and_match(struct device_node *from, const struct of_device_id *matches, const struct of_device_id **match)
  ```

  **from：**开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。 
  **matches：**of_device_id 匹配表，也就是在此匹配表里面查找节点。 
  **match：**找到的匹配的 of_device_id。 
  **返回值：**找到的节点，如果为 NULL 表示查找失败

- **of_find_node_by_path 函数**

  of_find_node_by_path 函数通过路径来查找指定的节点，函数原型如下：

  ```c
  inline struct device_node *of_find_node_by_path(const char *path)
  ```

  **path：**带有全路径的节点名，可以使用节点的别名，比如“/backlight”就是 backlight 这个 节点的全路径。 
  **返回值：**找到的节点，如果为 NULL 表示查找失败

##### 查找父/子节点的 OF 函数

1. **of_get_parent 函数**

   of_get_parent 函数用于获取指定节点的父节点(如果有父节点的话)，函数原型如下：

   ```c
   struct device_node *of_get_parent(const struct device_node *node)
   ```

   **node：**要查找的父节点的节点。 
   **返回值：**找到的父节点。

2. **of_get_next_child 函数**

   of_get_next_child 函数用迭代的方式查找子节点，函数原型如下：

   ```c
   struct device_node *of_get_next_child(const struct device_node *node, struct device_node *prev)
   ```

   **node：**父节点。 
   **prev：**前一个子节点，也就是从哪一个子节点开始迭代的查找下一个子节点。可以设置为 NULL，表示从第一个子节点开始。 
   **返回值：**找到的下一个子节点。

##### 提取属性值的 OF 函数

节点的属性信息里面保存了驱动所需要的内容，因此对于属性值的提取非常重要，Linux 内核中使用结构体 property 表示属性，此结构体同样定义在文件 include/linux/of.h 中，内容如下：

```c
struct property {
	char *name; /* 属性名字 */
	int length; /* 属性长度 */
	void *value; /* 属性值 */
	struct property *next; /* 下一个属性 */
	unsigned long _flags;
	unsigned int unique_id;
	struct bin_attribute attr;
};
```

1. **of_find_property 函数**

   of_find_property 函数用于查找指定的属性，函数原型如下：

   ```c
   property *of_find_property(const struct device_node *np, const char *name, int *lenp)
   ```

   **np：**设备节点。 
   **name：** 属性名字。 
   **lenp：**属性值的字节数 
   **返回值：**找到的属性。

2. **of_property_count_elems_of_size 函数**

   of_property_count_elems_of_size 函数用于获取属性中元素的数量，比如 reg 属性值是一个 数组，那么使用此函数可以获取到这个数组的大小，此函数原型如下：

   ```c
   int of_property_count_elems_of_size(const struct device_node *np, const char *propname, int elem_size)
   ```

   **np：**设备节点。 
   **proname：** 需要统计元素数量的属性名字。 
   **elem_size：**元素长度。
   **返回值：**得到的属性元素数量。

3. **of_property_read_u32_index 函数**

   of_property_read_u32_index 函数用于从属性中获取指定标号的 u32 类型数据值(无符号 32 位)，比如某个属性有多个 u32 类型的值，那么就可以使用此函数来获取指定标号的数据值，此 函数原型如下：

   ```c
   int of_property_read_u32_index(const struct device_node *np, const char *propname, u32 index, u32 *out_value)
   ```

   **np：**设备节点。
   **proname：** 要读取的属性名字。
   **index：**要读取的值标号。 
   **out_value：**读取到的值 
   **返回值：**0 读取成功，负值，读取失败，-EINVAL 表示属性不存在，-ENODATA 表示没有 要读取的数据，-EOVERFLOW 表示属性值列表太小。

4. **of_property_read_u8_array 函数** 
   **of_property_read_u16_array 函数** 
   **of_property_read_u32_array 函数** 
   **of_property_read_u64_array 函数**

   这 4 个函数分别是读取属性中 u8、u16、u32 和 u64 类型的数组数据，比如大多数的 reg 属 性都是数组数据，可以使用这 4 个函数一次读取出 reg 属性中的所有数据。这四个函数的原型 如下：

   ```c
   int of_property_read_u8_array(const struct device_node *np, const char *propname, u8 *out_values, size_t sz)
       
   int of_property_read_u16_array(const struct device_node *np, const char *propname,  u16 *out_values, size_t sz)
       
   int of_property_read_u32_array(const struct device_node *np, const char *propname, u32 *out_values, size_t sz)
       
   int of_property_read_u64_array(const struct device_node *np, const char *propname, u64 *out_values, size_t sz)
   ```

   **np：**设备节点。
   **proname：** 要读取的属性名字。 
   **out_value：**读取到的数组值，分别为 u8、u16、u32 和 u64。 
   **sz：**要读取的数组元素数量。 
   **返回值：**0，读取成功，负值，读取失败，-EINVAL 表示属性不存在，-ENODATA 表示没 有要读取的数据，-EOVERFLOW 表示属性值列表太小。

5. **of_property_read_u8 函数**
   **of_property_read_u16 函数** 
   **of_property_read_u32 函数** 
   **of_property_read_u64 函数**

   有些属性只有一个整形值，这四个函数就是用于读取这种只有一个整形值的属性，分别用 于读取 u8、u16、u32 和 u64 类型属性值，函数原型如下：

   ```c
   nt of_property_read_u8(const struct device_node *np, const char *propname, u8 *out_value)
   int of_property_read_u16(const struct device_node *np, const char *propname, u16 *out_value)
   int of_property_read_u32(const struct device_node *np, const char *propname, u32 *out_value)
   int of_property_read_u64(const struct device_node *np, const char *propname, u64 *out_value)
   ```

   **np：**设备节点。 
   **proname：** 要读取的属性名字。 
   **out_value：**读取到的数组值。 
   **返回值：**0，读取成功，负值，读取失败，-EINVAL 表示属性不存在，-ENODATA 表示没 有要读取的数据，-EOVERFLOW 表示属性值列表太小。

6. **of_property_read_string 函数**

   of_property_read_string 函数用于读取属性中字符串值，函数原型如下：

   ```c
   int of_property_read_string(struct device_node *np,  const char *propname, const char **out_string)
   ```

   np：设备节点。 
   proname： 要读取的属性名字。 
   out_string：读取到的字符串值。 
   返回值：0，读取成功，负值，读取失败。

7. **of_n_addr_cells 函数**

   of_n_addr_cells 函数用于获取#address-cells 属性值，函数原型如下：

   ```
   int of_n_addr_cells(struct device_node *np)
   ```

   np：设备节点。 
   返回值：获取到的#address-cells 属性值。

8. **of_n_size_cells 函数**

   of_size_cells 函数用于获取#size-cells 属性值，函数原型如下：

   ```
   int of_n_size_cells(struct device_node *np)
   ```

   np：设备节点。 
   返回值：获取到的#size-cells 属性值。

##### 其他常用的 OF 函数

1. **of_device_is_compatible 函数**

   of_device_is_compatible 函数用于查看节点的 compatible 属性是否有包含 compat 指定的字符串，也就是检查设备节点的兼容性，函数原型如下：

   ```c
   int of_device_is_compatible(const struct device_node *device,
    							const char *compat)
   ```

   device：设备节点。 
   compat：要查看的字符串。 
   返回值：0，节点的 compatible 属性中不包含 compat 指定的字符串；正数，节点的 compatible 属性中包含 compat 指定的字符串。

2. **of_get_address 函数**

   of_get_address 函数用于获取地址相关属性，主要是“reg”或者“assigned-addresses”属性 值，函数原型如下：

   ```c
   const __be32 *of_get_address(struct device_node *dev, 
   							int index, 
   							u64 *size,
    							unsigned int *flags)
   ```

   dev：设备节点。 
   index：要读取的地址标号。 
   size：地址长度。 
   flags：参数，比如 IORESOURCE_IO、IORESOURCE_MEM 等
   返回值：读取到的地址数据首地址，为 NULL 的话表示读取失败。

3. **of_translate_address 函数**
   of_translate_address 函数负责将从设备树读取到的地址转换为物理地址，函数原型如下：

   ```c
   u64 of_translate_address(struct device_node *dev,
   					const __be32 *in_addr)	
   ```

   dev：设备节点。 
   in_addr：要转换的地址。 
   返回值：得到的物理地址，如果为 OF_BAD_ADDR 的话表示转换失败。

4. **of_address_to_resource 函数**

   IIC、SPI、GPIO 等这些外设都有对应的寄存器，这些寄存器其实就是一组内存空间，Linux 内核使用 resource 结构体来描述一段内存空间，“resource”翻译出来就是“资源”，因此用 resource 结构体描述的都是设备资源信息，resource 结构体定义在文件 include/linux/ioport.h 中，定义如 下：

   ```c
   struct resource {
   	resource_size_t start;
   	resource_size_t end;
   	const char *name;
   	unsigned long flags;
   	struct resource *parent, *sibling, *child;
   };
   ```

   对于 32 位的 SOC 来说，resource_size_t 是 u32 类型的。其中 start 表示开始地址，end 表示 结束地址，name 是这个资源的名字，flags 是资源标志位，一般表示资源类型，可选的资源标志 定义在文件 include/linux/ioport.h 中，如下所示：

   ```c
   #define IORESOURCE_BITS 			0x000000ff 
   #define IORESOURCE_TYPE_BITS 		0x00001f00 
   #define IORESOURCE_IO 				0x00000100 
   #define IORESOURCE_MEM 				0x00000200
   #define IORESOURCE_REG 				0x00000300 
   #define IORESOURCE_IRQ 				0x00000400
   #define IORESOURCE_DMA 				0x00000800
   #define IORESOURCE_BUS 				0x00001000
   #define IORESOURCE_PREFETCH 		0x00002000 
   #define IORESOURCE_READONLY 		0x00004000
   #define IORESOURCE_CACHEABLE 		0x00008000
   #define IORESOURCE_RANGELENGTH 		0x00010000
   #define IORESOURCE_SHADOWABLE 		0x00020000
   #define IORESOURCE_SIZEALIGN 		0x00040000 
   #define IORESOURCE_STARTALIGN 		0x00080000 
   #define IORESOURCE_MEM_64 			0x00100000
   #define IORESOURCE_WINDOW 			0x00200000 
   #define IORESOURCE_MUXED 			0x00400000 
   #define IORESOURCE_EXCLUSIVE 		0x08000000 
   #define IORESOURCE_DISABLED 		0x10000000
   #define IORESOURCE_UNSET 			0x20000000
   #define IORESOURCE_AUTO 			0x40000000
   #define IORESOURCE_BUSY 			0x80000000 
   ```

   一 般 最 常 见 的 资 源 标 志 就 是 IORESOURCE_MEM 、 IORESOURCE_REG 和 IORESOURCE_IRQ 等。

   of_address_to_resource 函数是从设 备树里面提取资源值，但是本质上就是将 reg 属性值，然后将其转换为 resource 结构体类型， 函数原型如下所示：

   ```c
   int of_address_to_resource(struct device_node *dev, 
    						int index,
    						struct resource *r)
   ```

   dev：设备节点。 
   index：地址资源标号。 
   r：得到的 resource 类型的资源值。 
   返回值：0，成功；负值，失败。

5. **of_iomap 函数**

   of_iomap 函数用于直接内存映射，以前我们会通过 ioremap 函数来完成物理地址到虚拟地址的映射，采用设备树以后就可以直接通过 of_iomap 函数来获取内存地址所对应的虚拟地址， 不需要使用 ioremap 函数了。当然了，你也可以使用 ioremap 函数来完成物理地址到虚拟地址的内存映射，只是在采用设备树以后，大部分的驱动都使用 of_iomap 函数了。of_iomap 函数本质上也是将 reg 属性中地址信息转换为虚拟地址，如果 reg 属性有多段的话，可以通过 index 参数指定要完成内存映射的是哪一段，of_iomap 函数原型如下：

   ```c
   void __iomem *of_iomap(struct device_node *np, 
   						int index)
   ```

   np：设备节点。 
   index：reg 属性中要完成内存映射的段，如果 reg 属性只有一段的话 index 就设置为 0。
   返回值：经过内存映射后的虚拟内存首地址，如果为 NULL 的话表示内存映射失败。 关于设备树常用的 OF 函数就先讲解到这里，Linux 内核中关于设备树的 OF 函数不仅仅只 有前面讲的这几个，还有很多 OF 函数我们并没有讲解，这些没有讲解的 OF 函数要结合具体 的驱动，比如获取中断号的 OF 函数、获取 GPIO 的 OF 函数等等，这些 OF 函数我们在后面的 驱动实验中再详细的讲解。

### 设备树下的 LED 驱动实验

具体步骤：

1. 修改设备树，添加相应的节点，节点里面重点是设置 reg 属性，reg 属性包括了 GPIO 相关寄存器。
2. 修改设备树，添加相应的节点，节点里面重点是设置 reg 属性，reg 属性包括了 GPIO 相关寄存器。
3. 在②里面将 GPIO1_IO03 这个 PIN 复用为了 GPIO 功能，因此需要设置 GPIO1_IO03 这个 GPIO 相关的寄存器，也就是 GPIO1_DR 和 GPIO1_GDIR 这两个寄存器。

### pinctrl 和 gpio 子系统实验

Linux 驱动讲究驱动分离与分层，pinctrl 和 gpio 子系统就是驱动分离与分层思想下的产物， 驱动分离与分层其实就是按照面向对象编程的设计思想而设计的设备驱动框架

对于大多数的 32 位 SOC 而言，引脚的设置基本都是**设置某个 PIN 的复用功 能、速度、上下拉等，然后再设置 PIN 所对应的 GPIO**这两方面，因此 Linux 内核针对 PIN 的配置推出了 pinctrl 子系统，对于 GPIO 的配置推出了 gpio 子系统。

#### pinctrl 子系统

大多数 SOC 的 pin 都是支持复用的，比如 I.MX6ULL 的 GPIO1_IO03 既可以作为普通的 GPIO 使用，也可以作为 I2C1 的 SDA 等等。此外我们还需要配置 pin 的电气特性，比如上/下 拉、速度、驱动能力等等。传统的配置 pin 的方式就是直接操作相应的寄存器，但是这种配置 方式比较繁琐、而且容易出问题(比如 pin 功能冲突)。pinctrl 子系统就是为了解决这个问题而引 入的，pinctrl 子系统主要工作内容如下：

1. 获取设备树中 pin 信息。
2. 根据获取到的 pin 信息来设置 pin 的复用功能
3. 根据获取到的 pin 信息来设置 pin 的电气特性，比如上/下拉、速度、驱动能力等。

### linux中的并发与竞争

原子操作、自旋锁、信号量、互斥量对linux中资源的保护实验，详见47、48章。

### Linux内核定时器

硬件定时器提供时钟源，时钟源的频率可以设置， 设置好以后就周期性的产生定时中断，系统使用定时中断来计时。中断周期性产生的频率就是系统频率，
也叫做节拍率(tick rate)(有的资料也叫系统频率)，比如 1000Hz，100Hz 等等说的就是系统节拍率。系统节拍率是可以设置的，单位是 Hz，我们在编译 Linux 内核的时候可以通过图形化界面设置系统节拍率。

Linux 内核使用全局变量 jiffies 来记录系统从启动以来的系统节拍数，系统启动的时候会 将 jiffies 初始化为 0，jiffies 定义在文件 include/linux/jiffies.h 中

```c
extern u64 __jiffy_data jiffies_64;
extern unsigned long volatile __jiffy_data jiffies;
```

当我们访问 jiffies 的时候其实访问的是 jiffies_64 的低 32 位，使用 get_jiffies_64 这个函数 可以获取 jiffies_64 的值。在 32 位的系统上读取 jiffies 的值，在 64 位的系统上 jiffes 和 jiffies_64 表示同一个变量，因此也可以直接读取 jiffies 的值。所以不管是 32 位的系统还是 64 位系统， 都可以使用 jiffies。

处理 32 位 jiffies 的绕回显得尤为重要， Linux 内核提供了几个 API 函数来处理绕回：

![image-20231016153646060](image\23.png)

为了方便开发，Linux 内核提供了几个 jiffies 和 ms、us、ns 之间的转换函数

![image-20231016153721766](image\24.png)

Linux 内核使用 timer_list 结构体表示内核定时器，timer_list 定义在文件 include/linux/timer.h 中，定义如下：

![image-20231016153759085](image\25.png)

#### API函数：

##### 1、init_timer 函数  

负责初始化 timer_list 类型变量，当我们定义了一个 timer_list 变量以后一定 要先用 init_timer 初始化一下。
![image-20231016153937696](image\26.png)

##### 2、add_timer 函数

用于向 Linux 内核注册定时器，使用 add_timer 函数向内核注册定时器以后， 定时器就会开始运行

##### ![image-20231016154008925](image\27.png)

##### 3、del_timer 函数

用于删除一个定时器，不管定时器有没有被激活，都可以使用此函数删除。 在多处理器系统上，定时器可能会在其他的处理器上运行，因此在调用 del_timer 函数删除定时 器之前要先等待其他处理器的定时处理器函数退出。
![image-20231016154058720](image\28.png)

##### 4、del_timer_sync 函数

del_timer_sync 函数是 del_timer 函数的同步版，会等待其他处理器使用完定时器再删除， del_timer_sync 不能使用在中断上下文中。
![image-20231016154146562](image\29.png)

##### 5、mod_timer 函数

用于修改定时值，如果定时器还没有激活的话，mod_timer 函数会激活定时 器！
![image-20231016154237262](image\30.png)

### Linux中断

#### 裸机实验里面中断的处理方法：

1. 使能中断，初始化相应的寄存器。
2. 注册中断服务函数，也就是向 irqTable 数组的指定标号处写入中断服务函数
3. 中断发生以后进入 IRQ 中断服务函数，在 IRQ 中断服务函数在数组 irqTable 里面查找 具体的中断处理函数，找到以后执行相应的中断处理函数。

#### Linux中断：

每个中断都有一个中断号，通过中断号即可区分不同的中断，有的资料也把中断号叫做中 断线。在 Linux 内核中使用一个 int 变量表示中断号

#### request_irq 函数

在 Linux 内核中要想使用某个中断是需要申请的，request_irq 函数用于申请中断，**request_irq 函数可能会导致睡眠，因此不能在中断上下文或者其他禁止睡眠的代码段中使用** request_irq 函 数。request_irq 函数会激活(使能)中断，所以不需要我们手动去使能中断，request_irq 函数原型 如下：
![image-20231016155049102](image\31.png)![image-20231016155107623](image\32.png)

表中的这些标志可以通过“|”来实现多种组合。

**name：**中断名字，设置以后可以在/proc/interrupts 文件中看到对应的中断名字。
 **dev：**如果将 flags 设置为 IRQF_SHARED 的话，dev 用来区分不同的中断，**一般情况下将 dev 设置为设备结构体**，dev 会传递给中断处理函数 irq_handler_t 的第二个参数。
 **返回值：**0 中断申请成功，其他负值 中断申请失败，如果返回-EBUSY 的话表示中断已经被申请了

#### free_irq 函数

使用中断的时候需要通过 request_irq 函数申请，使用完成以后就要通过 free_irq 函数释放 掉相应的中断。如果中断不是共享的，那么 free_irq 会删除中断处理函数并且禁止中断。
![image-20231016155337309](image\33.png)

#### 中断处理函数

使用 request_irq 函数申请中断的时候需要设置中断处理函数，中断处理函数格式如下所示：

```
irqreturn_t (*irq_handler_t) (int, void *)
```

第一个参数是要中断处理函数要相应的中断号。第二个参数是一个指向 void 的指针，也就 是个通用指针，需要与 request_irq 函数的 dev 参数保持一致。用于区分共享中断的不同设备， dev 也可以指向设备数据结构。中断处理函数的返回值为 irqreturn_t 类型，irqreturn_t 类型定义 如下所示：
![image-20231016155503118](image\34.png)

#### 中断使能与禁止函数

![image-20231016155608360](image\35.png)

enable_irq 和 disable_irq 用于使能和禁止指定的中断，irq 就是要禁止的中断号。disable_irq函数要等到当前正在执行的中断处理函数执行完才返回，因此使用者需要保证不会产生新的中 断，并且确保所有已经开始执行的中断处理程序已经全部退出。在这种情况下，可以使用另外 一个中断禁止函数：

void disable_irq_nosync(unsigned int irq)

disable_irq_nosync 函数调用以后立即返回，不会等待当前中断处理程序执行完毕。上面三 个函数都是使能或者禁止某一个中断，有时候我们需要关闭当前处理器的整个中断系统，也就 是在学习 STM32 的时候常说的关闭全局中断，这个时候可以使用如下两个函数：

![image-20231016155712597](image\36.png)

local_irq_enable 用于使能当前处理器中断系统，local_irq_disable 用于禁止当前处理器中断 系统。假如 A 任务调用 local_irq_disable 关闭全局中断 10S，当关闭了 2S 的时候 B 任务开始运 行，B 任务也调用 local_irq_disable 关闭全局中断 3S，3 秒以后 B 任务调用 local_irq_enable 函 数将全局中断打开了。此时才过去 2+3=5 秒的时间，然后全局中断就被打开了，此时 A 任务要 关闭 10S 全局中断的愿望就破灭了，然后 A 任务就“生气了”，结果很严重，可能系统都要被 A 任务整崩溃。为了解决这个问题，B 任务不能直接简单粗暴的通过 local_irq_enable 函数来打 开全局中断，而是将中断状态恢复到以前的状态，要考虑到别的任务的感受，此时就要用到下 面两个函数：

![image-20231016155803362](image\37.png)

#### 上半部与下半部

在有些资料中也将上半部和下半部称为顶半部和底半部，都是一个意思。我们在使用 request_irq 申请中断的时候注册的中断服务函数属于中断处理的上半部，只要中断触发，那么 中断处理函数就会执行。我们都知道中断处理函数一定要快点执行完毕，越短越好，但是现实 往往是残酷的，有些中断处理过程就是比较费时间，我们必须要对其进行处理，缩小中断处理 函数的执行时间。比如电容触摸屏通过中断通知 SOC 有触摸事件发生，SOC 响应中断，然后 通过 IIC 接口读取触摸坐标值并将其上报给系统。但是我们都知道 IIC 的速度最高也只有 400Kbit/S，所以在中断中通过 IIC 读取数据就会浪费时间。我们可以将通过 IIC 读取触摸数据 的操作暂后执行，中断处理函数仅仅相应中断，然后清除中断标志位即可。这个时候中断处理 过程就分为了两部分：

- **上半部：**上半部就是中断处理函数，那些处理过程比较快，不会占用很长时间的处理就可 以放在上半部完成。
- **下半部：**如果中断处理过程比较耗时，那么就将这些比较耗时的代码提出来，交给下半部去执行，这样中断处理函数就会快进快出。

因此，Linux 内核将中断分为上半部和下半部的主要目的就是实现中断处理函数的快进快 出，那些对时间敏感、执行速度快的操作可以放到中断处理函数中，也就是上半部。剩下的所 有工作都可以放到下半部去执行，比如在上半部将数据拷贝到内存中，关于数据的具体处理就 可以放到下半部去执行。至于哪些代码属于上半部，哪些代码属于下半部并没有明确的规定， 一切根据实际使用情况去判断，这个就很考验驱动编写人员的功底了。这里有一些可以借鉴的 参考点：

1. 如果要处理的内容不希望被其他中断打断，那么可以放到上半部。
2. 如果要处理的任务对时间敏感，可以放到上半部。
3. 如果要处理的任务与硬件有关，可以放到上半部
4. 除了上述三点以外的其他任务，优先考虑放到下半部。

**Linux 内 核提供了多种下半部机制，接下来我们来学习一下这些下半部机制。**

##### 软中断

一开始 Linux 内核提供了“bottom half”机制来实现下半部，简称“BH”。后面引入了软中 断和 tasklet 来替代“BH”机制，完全可以使用软中断和 tasklet 来替代 BH，从 2.5 版本的 Linux 内核开始 BH 已经被抛弃了。Linux 内核使用结构体 softirq_action 表示软中断， softirq_action 结构体定义在文件 include/linux/interrupt.h 中

```
struct softirq_action
{
	void (*action)(struct softirq_action *);
};
```

在 kernel/softirq.c 文件中一共定义了 10 个软中断：

![image-20231016160152677](image\38.png)

可以看出，一共有 10 个软中断，因此 NR_SOFTIRQS 为 10，因此数组 softirq_vec 有 10 个 元素。softirq_action 结构体中的 action 成员变量就是软中断的服务函数，数组 softirq_vec 是个 全局数组，因此所有的 CPU(对于 SMP 系统而言)都可以访问到，每个 CPU 都有自己的触发和 控制机制，并且只执行自己所触发的软中断。但是各个 CPU 所执行的软中断服务函数确是相同 的，都是数组 softirq_vec 中定义的 action 函数。**要使用软中断，必须先使用 open_softirq 函数注 册对应的软中断处理函数**，open_softirq 函数原型如下：
void open_softirq(int nr, void (*action)(struct softirq_action *))

**nr：**要开启的软中断，在示例代码 51.1.2.3 中选择一个。 
**action：**软中断对应的处理函数。 
**返回值：**没有返回值。

注册好软中断以后需要通过 raise_softirq 函数触发，raise_softirq 函数原型如下：
![image-20231016160356829](image\39.png)

软中断必须在编译的时候静态注册！Linux 内核使用 softirq_init 函数初始化软中断， softirq_init 函数定义在 kernel/softirq.c 文件里面：

![image-20231016160432218](image\40.png)

##### tasklet

tasklet 是利用软中断来实现的另外一种下半部机制，在软中断和 tasklet 之间，建议大家使 用 tasklet。Linux 内核使用 tasklet_struct 结构体来表示 tasklet：
![image-20231016160535965](image\41.png)

第 489 行的 func 函数就是 tasklet 要执行的处理函数，用户定义函数内容，相当于中断处理 函数。如果要使用 tasklet，必须先定义一个 tasklet，然后使用 **tasklet_init 函数初始化 tasklet**：

```c
void tasklet_init(struct tasklet_struct *t,
				void (*func)(unsigned long), 
					unsigned long data);
```

**t：**				要初始化的 tasklet
**func：**		 tasklet 的处理函数。 
**data：**		 要传递给 func 函数的参数 
**返回值：**	   没有返回值。

也可以使用**宏 DECLARE_TASKLET** 来一次性完成 tasklet 的定义和初始化， DECLARE_TASKLET 定义在 include/linux/interrupt.h 文件中，定义如下:

![image-20231016160801107](image\42.png)

在上半部，也就是中断处理函数中调用 tasklet_schedule 函数就能使 tasklet 在合适的时间运 行，tasklet_schedule 函数原型如下：
![image-20231016160841399](image\43.png)

##### 工作队列

工作队列是另外一种下半部执行方式，工作队列在进程上下文执行，工作队列将要推后的 工作交给一个内核线程去执行，因为工作队列工作在进程上下文，因此工作队列允许睡眠或重 新调度。因此如果你要推后的工作可以睡眠那么就可以选择工作队列，否则的话就只能选择软 中断或 tasklet。

Linux 内核使用 work_struct 结构体表示一个工作，内容如下(省略掉条件编译)：
![image-20231016161037812](image\44.png)

这些工作组织成工作队列，工作队列使用 workqueue_struct 结构体表示，内容如下(省略掉 条件编译)：
![image-20231016161103810](image\45.png)

Linux 内核使用工作者线程(worker thread)来处理工作队列中的各个工作，Linux 内核使用 worker 结构体表示工作者线程，worker 结构体内容如下：
![image-20231016161130296](image\46.png)

每个 worker 都有一个工作队列，工作者线程处理自己工 作队列中的所有工作。**在实际的驱动开发中，我们只需要定义工作(work_struct)即可**，关于工作 队列和工作者线程我们基本不用去管。
简单创建工作很简单，直接定义一个 work_struct 结构体 变量即可，然后使用 INIT_WORK 宏来初始化工作，INIT_WORK 宏定义如下：
**#define INIT_WORK(_work, _func)**
_work 表示要初始化的工作，_func 是工作对应的处理函数。

也可以使用 DECLARE_WORK 宏一次性完成工作的创建和初始化，宏定义如下：
**\**#define DECLARE_WORK(n, f)****
n 表示定义的工作(work_struct)，f 表示工作对应的处理函数。

和 tasklet 一样，工作也是需要调度才能运行的，工作的调度函数为 schedule_work，函数原 型如下所示：
![image-20231016162221061](image\47.png)



#### 设备树中断信息节点

如果使用设备树的话就需要在设备树中设置好中断属性信息，Linux 内核通过读取设备树 中 的 中断 属性 信息 来配 置中 断。 对于 中断 控制 器 而言 ，设 备树 绑定 信息 参考 文档 Documentation/devicetree/bindings/arm/gic.txt
imx6ull.dtsi 文件中的 intc 节点就是 I.MX6ULL 的中断控制器节点
![image-20231016165825403](image\48.png)

第 2 行，compatible 属性值为“arm,cortex-a7-gic”在 Linux 内核源码中搜索“arm,cortex-a7- gic”即可找到 GIC 中断控制器驱动文件。
第 3 行，#interrupt-cells 和#address-cells、#size-cells 一样。表示此中断控制器下设备的 cells 大小，对于设备而言，会使用 interrupts 属性描述中断信息，#interrupt-cells 描述了 interrupts 属性的 cells 大小，也就是一条信息有几个 cells。每个 cells 都是 32 位整形值，对于 ARM 处理的 GIC 来说，一共有 3 个 cells，这三个 cells 的含义如下：

1. 第一个 cells：中断类型，0 表示 SPI 中断，1 表示 PPI 中断。
2. 第二个 cells：中断号，对于 SPI 中断来说中断号的范围为 0~987，对于 PPI 中断来说中断 号的范围为 0~15。
3. 第三个 cells：标志，bit[3:0]表示中断触发类型，为 1 的时候表示上升沿触发，为 2 的时候 表示下降沿触发，为 4 的时候表示高电平触发，为 8 的时候表示低电平触发。bit[15:8]为 PPI 中 断的 CPU 掩码。

第 4 行，interrupt-controller 节点为空，表示当前节点是中断控制器。

关于GPIO5的中断控制设备树相关描述见P1281

简单总结一下与**中断有关的设备树属性信息**：

- \#interrupt-cells，指定中断源的信息 cells 个数。
- interrupt-controller，表示当前节点为中断控制器。
- interrupts，指定中断号，触发方式等。
- interrupt-parent，指定父中断，也就是中断控制器。

##### 获取中断号

编写驱动的时候需要用到中断号，我们用到中断号，中断信息已经写到了设备树里面，因 此可以通过 irq_of_parse_and_map 函数从 interupts 属性中提取到对应的设备号，函数原型如下：
![image-20231016170112880](image\49.png)

如果使用 GPIO 的话，可以使用 **gpio_to_irq** 函数来获取 gpio 对应的中断号，函数原型如 下：
![image-20231016171529434](image\50.png)

### 阻塞与非阻塞

这里的 IO 指的是 Input/Output，也就是输入/输出，是应用程序对驱动设备的输入/输出操作。
当 应用程序对设备驱动进行操作的时候，**如果不能获取到设备资源**，**那么阻塞式 IO 就会将应用程 序对应的线程挂起，直到设备资源可以获取为止**。
**对于非阻塞 IO，应用程序对应的线程不会挂 起，它要么一直轮询等待**，直到设备资源可以使用，要么就**直接放弃**。

#### 等待队列

阻塞访问最大的好处就是当设备文件不可操作的时候进程可以进入休眠态，这样可以将 CPU 资源让出来。但是，当设备文件可以操作的时候就必须唤醒进程，一般在中断函数里面完成唤醒工作。
Linux 内核提供了等待队列(wait queue)来实现阻塞进程的唤醒工作

##### 等待队列头

要 在驱动中使用等待队列，必须创建并初始化一个等待队列头，等待队列头使用结构体 wait_queue_head_t 表示，wait_queue_head_t 结构体定义在文件 include/linux/wait.h 中，结构体定义：

```c
struct __wait_queue_head {
	spinlock_t lock;
	struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;
```

定义好等待队列头以后需要初始化，使用 init_waitqueue_head 函数初始化等待队列头，函 数原型如下：

**void init_waitqueue_head(wait_queue_head_t *q)**
参数q即要初始化的等待队列头

也可以使用宏 DECLARE_WAIT_QUEUE_HEAD 来一次性完成等待队列头的定义的初始 化。

##### 等待队列项

等待队列头就是一个等待队列的头部，每个访问设备的进程都是一个队列项，当设备不可 用的时候就要将这些进程对应的等待队列项添加到等待队列里面。结构体 wait_queue_t 表示等 待队列项，结构体内容如下：

```c
struct __wait_queue {
 unsigned int flags;
 void *private;
 wait_queue_func_t func;
 struct list_head task_list;
};
typedef struct __wait_queue wait_queue_t;
```

使用宏 DECLARE_WAITQUEUE 定义并初始化一个等待队列项，宏的内容如下：

**DECLARE_WAITQUEUE(name, tsk)**

name 就是等待队列项的名字，tsk 表示这个等待队列项属于哪个任务(进程)，一般设置为 current ， 在 Linux 内 核 中 current 相 当 于 一 个 全 局 变 量 ， 表 示 当 前 进 程 。 因 此 宏DECLARE_WAITQUEUE 就是**给当前正在运行的进程创建并初始化了一个等待队列项。**

##### 将队列项添加/移除等待队列头

当设备不可访问的时候就需要将进程对应的等待队列项添加到前面创建的等待队列头中， 只有添加到等待队列头中以后进程才能进入休眠态。当设备可以访问以后再将进程对应的等待 队列项从等待队列头中移除即可。
**等待队列项添加** API 函数如下：

![image-20231017162812194](image\51.png)

**等待队列项移除** API 函数如下：

![image-20231017162921785](image\52.png)

##### 等待唤醒

当设备可以使用的时候就要唤醒进入休眠态的进程，唤醒可以使用如下两个函数：

```c
void wake_up(wait_queue_head_t *q)
void wake_up_interruptible(wait_queue_head_t *q)
```

参数 q 就是要唤醒的等待队列头，这两个函数会将这个等待队列头中的所有进程都唤醒。 wake_up 函数可以唤醒处于 TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 状态的进 程，而 wake_up_interruptible 函数只能唤醒处于 TASK_INTERRUPTIBLE 状态的进程。

##### 等待事件

除了主动唤醒以外，也可以设置等待队列等待某个事件，当这个事件满足以后就自动唤醒 等待队列中的进程。相关函数：

| 函数                                                      | 描述                                                         |
| --------------------------------------------------------- | ------------------------------------------------------------ |
| wait_event(wq, condition)                                 | 等待以 wq 为等待队列头的等待队列被唤醒，前提是 condition 条件必须满足(为真)，否则一直阻塞。此函数会将进程设置为 TASK_UNINTERRUPTIBLE 状态 |
| wait_event_timeout(wq, condition, timeout)                | 功能和 wait_event 类似，但是此函数可以添加超时时间，以 jiffies 为单位。此函数有返回值，如果返回 0 的话表示超时时间到，而且 condition 为假。为 1 的话表示 condition 为真，也就是条件满足了。 |
| wait_event_interruptible(wq, condition)                   | 与 wait_event 函数类似，但是此函数将进程设置为 TASK_INTERRUPTIBLE，就是可以被信号打断。 |
| wait_event_interruptible_timeout(wq,  condition, timeout) | 与 wait_event_timeout 函数类似，此函数也将进程设置为 TASK_INTERRUPTIBLE，可以被信号打断。 |

##### 轮询

如果用户应用程序以非阻塞的方式访问设备，设备驱动程序就要提供非阻塞的处理方式， 也就是轮询。**poll、epoll 和 select 可以用于处理轮询**，应用程序通过 **select、epoll 或 poll 函数**来查询设备是否可以操作，**如果可以操作的话就从设备读取或者向设备写入数**据。当应用程序调用 select、epoll 或 poll 函数的时候设备驱动程序中的poll函数就会执行，因此需要在设备驱动程序中编写 poll 函数。

###### select 函数

原型：

![image-20231017165711766](image\53.png)

比如我们现在要从一个设备文件中读取数据，那么就可以定义一个 fd_set 变量，这个变量 要传递给参数 readfds。当我们定义好一个 fd_set 变量以后可以使用如下所示几个宏进行操作：

![image-20231019175357762](image\54.png)

**FD_ZERO** 用于将 fd_set 变量的所有位都清零，
**FD_SET** 用于将 fd_set 变量的某个位置 1， 也就是向 fd_set 添加一个文件描述符，参数 fd 就是要加入的文件描述符。
**FD_CLR** 用于将 fd_set变量的某个位清零，也就是将一个文件描述符从 fd_set 中删除，参数 fd 就是要删除的文件描述 符。
**FD_ISSET** 用于测试一个文件是否属于某个集合，参数 fd 就是要判断的文件描述符。

![image-20231019180021121](image\55.png)

###### poll函数

在单个线程中，select 函数能够监视的文件描述符数量有最大的限制，一般为 1024，可以 修改内核将监视的文件描述符数量改大，但是这样会降低效率！这个时候就可以使用 poll 函数， poll 函数本质上和 select 没有太大的差别，但是 poll 函数没有最大文件描述符限制，Linux 应用 程序中 poll 函数原型如下所示：
![image-20231019182509815](image\56.png)

###### epoll函数

见P1302.

##### Linux驱动下的poll操作函数

**当应用程序调用 select 或 poll 函数来对驱动程序进行非阻塞访问的时候，驱动程序 file_operations 操作集中的 poll 函数就会执行。**

所以驱动程序的编写者需要提供对应的 poll 函 数，poll 函数原型如下所示：
![image-20231019183008693](image\57.png)

我们需要在驱动程序的 poll 函数中调用 poll_wait 函数，poll_wait 函数不会引起阻塞，只是 将应用程序添加到 poll_table 中，poll_wait 函数原型如下：
![image-20231019183042802](image\58.png)

### 异步通知

#### 简介

前文中的阻塞与非阻塞方式访问驱动设备，其中通过阻塞方式访问的话应用程序会处于休眠态，等待驱动设备可以使用；非阻塞方式的话会通过 poll 函数来不断的轮询，查看驱动设备文件是否可以使用。这两种方式都需要应用程序主动的去查询设备是否可以使用。而异步通知就是一种能够让驱动程序可以访问的时候主动告诉应用程序的机制。

**“信号”**为此应运而生，信号类似于我们硬件上使用的“中断”，只不过信号是软件层次上 的。算是在软件层次上对中断的一种模拟，驱动可以通过主动向应用程序发送信号的方式来报 告自己可以访问了，应用程序获取到信号以后就可以从驱动设备中读取或者写入数据了。整个 过程就相当于应用程序收到了驱动发送过来了的一个中断，然后应用程序去响应这个中断，在 整个处理过程中应用程序并没有去查询驱动设备是否可以访问，一切都是由驱动设备自己告诉 给应用程序的。

阻塞、非阻塞、异步通知，这三种是针对不同的场合提出来的不同的解决方法，**没有优劣之分**，在实际的工作和学习中，根据自己的实际需求选择合适的处理方法即可。

异步通知的**核心就是信号**，在 arch/xtensa/include/uapi/asm/signal.h 文件中定义了 Linux 所支 持的所有信号：
![image-20231020154859783](image\59.png)![image-20231020154915165](image\60.png)

上述信号中除了 SIGKILL(9)和 SIGSTOP(19)这两个信号不能被忽略外，**其他的信号都可以忽略**。这些信号就相当于中断号，不同的中断号代表了不同的中断， 不同的中断所做的处理不同，因此，驱动程序可以通过向应用程序发送不同的信号来实现不同 的功能。

使用中断的时候需要设置中断处理函数，同样的，如果要在应用程序中使用信号，那么就必须**设置信号所使用的信号处理函数**，在应用程序中**使用 signal 函数来设置**指定信号的处理函数，signal 函数原型如下所示：

![image-20231020155158479](image\61.png)

信号处理函数的原型：

typedef void (*sighandler_t)(int)

#### 驱动中的信息处理

##### 1、fasync_struct 结构体

在驱动中使用信号需要在驱动程序中定义一个 fasync_struct 结构体指针变量，fasync_struct 结构体内 容如下：
![image-20231020160811117](image\62.png)

一般将 fasync_struct 结构体指针变量**定义到设备结构体中**

##### 2、fasync 函数

如果要使用异步通知，需要在设备驱动中实现 file_operations 操作集中的 fasync 函数。函数原型：
**int (*fasync) (int fd, struct file *filp, int on)**

fasync 函数里面一般通过调用 **fasync_helper** 函数来初始化前面定义的 fasync_struct 结构体指针，原型：
**int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp)**
fasync_helper 函数的前三个参数就是 fasync 函数的那三个参数
第四个参数就是**要初始化 的 fasync_struct 结构体指针变量**。

当应用程序通过**“fcntl(fd, F_SETFL, flags | FASYNC)”**改变 fasync 标记的时候，驱动程序 file_operations 操作集中的 fasync 函数就会执行。

在关闭驱动文件的时候需要在 file_operations 操作集中的 release 函数中释放 fasync_struct， **fasync_struct 的释放函数同样为 fasync_helper**

##### 3、kill_fasync 函数

当设备可以访问的时候，驱动程序需要向应用程序发出信号，相当于产生“中断”。kill_fasync 函数负责发送指定的信号。其函数原型如下：
![image-20231020161514203](image\63.png)

#### 应用程序对异步通知的处理

应用程序对异步通知的处理包括以下三步：

1. ##### 注册信号处理函数

   应用程序根据驱动程序所使用的信号来设置信号的处理函数，应用程序使用 signal 函数来 设置信号的处理函数。

2. 将本应用程序的进程号告诉给内核

   使用 **fcntl(fd, F_SETOWN, getpid())**将本应用程序的进程号告诉给内核。

3. 开启异步通知

   使用如下两行程序开启异步通知：

   ```c
   flags = fcntl(fd, F_GETFL); /* 获取当前的进程状态 */
   fcntl(fd, F_SETFL, flags | FASYNC); /* 开启当前进程异步通知功能 */
   ```


### platform 设备驱动实验

#### Linux 驱动的分离与分层

对于 Linux 这样一个成熟、庞大、复杂的操作系统，代码的重用性非常重要，否则的话就会在 Linux 内核中存在大量无意义的重复代码。尤其是驱动程序，因为驱动程序占用了 Linux 内核代码量的大头，如果不对驱动程序加以管理，任由重复的代码肆意增加，那么用不了多久 Linux 内核的文件数量就庞大到无法接受的地步。

##### 驱动的分隔与分离


驱动的分隔，也就是将主机驱动和设备驱动分隔开来，比如 I2C、SPI 等等都会采用驱动分隔的方式来简化驱动的开发。

在实际的驱动开发中，一般 I2C 主机控制器驱动已经由半导体厂家编写好了，而设备驱动一般也由设备器件的厂家编写好了，我们只需要提供设备信息即可，比如 I2C 设备的话提供设备连接到了哪个 I2C 接口上，I2C 的速度是多少等等。相当于将设备信息从设备驱动中剥离开来，驱动使用标准方法去获取到设备信息(比如从设备树中获取到设备信息)，然后根据获取到的设备信息来初始化设备。 这样就相当于驱动只负责驱动，设备只负责设备，想办法将两者进行匹配即可。这个就是 Linux 中的**总线(bus)、驱动(driver)和 设备(device)模型**，也就是常说的驱动分离。

具体见开发指南54章

。。。

#### platform 平台驱动模型简介

前面我们讲了设备驱动的分离，并且引出了总线(bus)、驱动(driver)和设备(device)模型，比 如 I2C、SPI、USB 等总线。但是在 SOC 中有些外设是没有总线这个概念的，但是又要使用总 线、驱动和设备模型该怎么办呢？为了解决此问题，**Linux 提出了 platform 这个虚拟总线**，相应 的就有 platform_driver 和 platform_device。

##### platform 总线

Linux系统内核使用bus_type 结构体表示总线，此结构体定义在文件 include/linux/device.h
![image-20231023161147480](image\64.png)![image-20231023161212911](image\65.png)

第 10 行，match 函数，此函数很重要，单词 match 的意思就是“匹配、相配”，因此**此函数就是完成设备和驱动之间匹配的**，总线就是使用 match 函数来根据注册的设备来查找对应的驱动，或者根据注册的驱动来查找相应的设备，因此**每一条总线都必须实现此函数**。match 函数有 两个参数：dev 和 drv，这**两个参数分别为 device 和 device_driver 类型，也就是设备和驱动。**

platform 总线是 bus_type 的一个具体实例，定义在文件 drivers/base/platform.c，platform 总 线定义如下：

![image-20231023161348957](image\66.png)

platform_bus_type 就是 platform 平台总线，其中 platform_match 就是匹配函数。
platform_match 函数定义在文件 drivers/base/platform.c 中

```c
static int platform_match(struct device *dev, struct device_driver *drv)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct platform_driver *pdrv = to_platform_driver(drv);
	
	/*When driver_override is set,only bind to the matching driver*/
	if (pdev->driver_override)
	return !strcmp(pdev->driver_override, drv->name);
	
	/* Attempt an OF style match first */
	if (of_driver_match_device(dev, drv))
		return 1;
	
	/* Then try ACPI style match */
	if (acpi_driver_match_device(dev, drv))
		return 1;
	
	/* Then try to match against the id table */
	if (pdrv->id_table)
		return platform_match_id(pdrv->id_table, pdev) != NULL;
	
	/* fall-back to driver name match */
	return (strcmp(pdev->name, drv->name) == 0);
}
```

驱动和设备的匹配有四种方法:

1. **OF 类型的匹配**，也就是设备树采用的匹配方式， of_driver_match_device 函数定义在文件 include/linux/of_device.h 中。device_driver 结构体(表示设备驱动)中有个名为of_match_table的成员变量，此成员变量保存着驱动的compatible匹配表， 设备树中的每个设备节点的 compatible 属性会和 of_match_table 表中的所有成员比较，查看是否有相同的条目，如果有的话就表示设备和此驱动匹配，设备和驱动匹配成功以后 **probe** 函数就会执行。
2. **ACPI 匹配方式**。
3. **id_table 匹配**，每个 platform_driver 结构体有一个 id_table 成员变量，顾名思义，保存了很多 id 信息。这些 id 信息存放着这个 platformd 驱动所支持的驱动类型。
4. 如果第三种匹配方式的 id_table 不存在的话就直接比较驱动和 设备的 name 字段，看看是不是相等，如果相等的话就匹配成功。

对于支持设备树的 Linux 版本号，一般设备驱动为了兼容性都支持设备树和无设备树两种 匹配方式。也就是第一种匹配方式一般都会存在，第三种和第四种只要存在一种就可以，一般 用的最多的还是第四种，也就是直接比较驱动和设备的 name 字段，毕竟这种方式最简单了。

##### platform 驱动

platform_driver 结构体表示 platform 驱动，此结构体定义在文件 include/linux/platform_device.h中：

```c
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
 };
```

**probe 函数**，当驱动与设备**匹配成功以后** probe 函数就会执行，**非常重要的函数一般驱动的提供者会编写，如果自己要编写一个全新的驱动，那么 probe 就需要自行实现。**

**driver 成员**，为 device_driver 结构体变量，Linux 内核里面大量使用到了面向对象的思维，device_driver 相当于基类，提供了最基础的驱动框架。plaform_driver 继承了这个基类， 然后在此基础上又添加了一些特有的成员变量。

**id_table 表**，也就是前面提到 platform 总线匹配驱动和设备的时候采用的第三种方法，id_table 是个表( 也就是数组) ，每个元素的类型为 platform_device_id，其结构体定义为：
![image-20231023164135384](image\67.png)

device_driver 结构体定义在 include/linux/device.h：
![image-20231023164215808](image\68.png)

第 10 行，of_match_table 就是采用设备树的时候驱动使用的匹配表，同样是数组，每个匹配项都为 of_device_id结构体类型，此结构体定义在文件 include/linux/mod_devicetable.h 中：
![image-20231023164501818](image\69.png)

第 4 行的 compatible 非常重要，因为对于设备树而言，就是通过设备节点的 compatible 属 性值和 of_match_table 中每个项目的 compatible 成员变量进行比较，如果有相等的就表示设备 和此驱动匹配成功。

在编写 platform 驱动的时候，首先定义一个 platform_driver 结构体变量，然后实现结构体 中的各个成员变量，重点是实现匹配方法以及 probe 函数。当驱动和设备匹配成功以后 probe 函数就会执行，具体的驱动程序在 probe 函数里面编写，比如字符设备驱动等等。

当我们定义并初始化好 platform_driver 结构体变量以后，需要在驱动入口函数里面调用 platform_driver_register 函数向 Linux 内核注册一个 platform 驱动，platform_driver_register 函数 原型如下所示：
![image-20231023164638679](image\70.png)

还需要在驱动卸载函数中通过 platform_driver_unregister 函数卸载 platform 驱动， platform_driver_unregister 函数原型如下：
![image-20231023164752278](image\71.png)

##### platform 设备

platform 驱动已经准备好了，我们还需要 platform 设备。platform_device 这个结构体表示 platform 设备。**注意**：如果内核支持设备树 的话就不要再使用 platform_device 来描述设备了，因为改用设备树去描述了。当然一定要用platform_device 来描述设备信息的话也是可以的。

platform_device 结构体定义在文件 include/linux/platform_device.h 中：
![image-20231023165552512](image\72.png)

第 23 行，name 表示设备名字，要和所使用的 platform 驱动的 name 字段相同，否则的话设 备就无法匹配到对应的驱动。

第 27 行，num_resources 表示资源数量，一般为第 28 行 resource 资源的大小。

第 28 行，resource 表示资源，也就是设备信息，比如外设寄存器等。Linux 内核使用 resource 结构体表示资源，resource 结构体内容如下：
![image-20231023165743895](image\73.png)
start 和 end 分别表示资源的起始和终止信息，对于内存类的资源，就表示内存起始和终止 地址，name 表示资源名字，flags 表示资源类型，可选的资源类型都定义在了文件 include/linux/ioport.h 里面。

在以前不支持设备树的Linux版本中，用户需要编写platform_device变量来描述设备信息， 然后使用 platform_device_register 函数将设备信息注册到 Linux 内核中，此函数原型如下所示：

![image-20231023165848116](image\74.png)

如果不再使用 platform 的话可以通过 platform_device_unregister 函数注销掉相应的 platform 设备，platform_device_unregister 函数原型如下：
![image-20231023165917432](image\75.png)



### Linux MISC驱动

MISC 驱动其实就是最简单的字符设备驱 动，通常嵌套在 platform 总线驱动中，实现复杂的驱动

所有的 MISC 设备驱动的主设备号都为 10，不同的设备使用不同的从设备号。随着 Linux 字符设备驱动的不断增加，设备号变得越来越紧张，尤其是主设备号，MISC 设备驱动就用于解 决此问题。MISC 设备会自动创建 cdev，不需要像我们以前那样手动创建，因此采用 MISC 设 备驱动可以简化字符设备驱动的编写
需要向 Linux 注册一个 miscdevice 设备，miscdevice 是一个结构体，定义在文件 include/linux/miscdevice.h 中:

```c
struct miscdevice {
	int minor; /* 子设备号 */
	const char *name; /* 设备名字 */ 
	const struct file_operations *fops; /* 设备操作集 */
	struct list_head list;
	struct device *parent;
	struct device *this_device;
	const struct attribute_group **groups;
	const char *nodename;
	umode_t mode;
};
```

定义一个 MISC 设备(miscdevice 类型)以后需要设置 **minor、name 和 fops** 这三个成员变量。

minor 表示子设备号，MISC 设备的主设备号为 10，这个是固定的，需要用户指定子设备号Linux 系统已经预定义了一些 MISC 设备的子设备号，这些预定义的子设备号定义在 include/linux/miscdevice.h 文件中:
![image-20231029144104847](image\76.png)

使用的时候可以从这些预定义的子设备号中挑选一个，当然也可以自己定义，只要 这个子设备号没有被其他设备使用接口。
name 就是此 MISC 设备名字，当此设备注册成功以后就会在/dev 目录下生成一个名为 name 的设备文件。
fops 就是字符设备的操作集合，MISC 设备驱动最终是需要使用用户提供的 fops 操作集合。

当设置好 miscdevice 以后就需要使用 misc_register 函数向系统中注册一个 MISC 设备，此函数原型如下：
![image-20231029144148945](image\77.png)

以前我们需要自己调用一堆的函数去创建设备,现在我们可以直接使用 misc_register 一个函数来完成.
卸载设备驱动模块的时候需要调用 misc_deregister 函数来注销掉 MISC 设备，函数原型如下：
![image-20231029144241407](image\78.png)
同理，我们在注销设备驱动的时候也可以使用这函数来代替以前的一系列函数。

### Linux INPUT子系统

按键、鼠标、键盘、触摸屏等都属于输入(input)设备，Linux 内核为此专门做了一个叫做 input 子系统的框架来处理输入事件。
输入设备本质上还是字符设备，只是在此基础上套上了input框架，用户只需要负责上报输入事件，比如按键值、坐标等信息，input 核心层负责处理这些事件。

不同的输入设备所代表的含义不同，按键和键盘就是代表按键信息， 鼠标和触摸屏代表坐标信息，因此在应用层的处理就不同，对于驱动编写者而言不需要去关心 应用层的事情，我们只需要按照要求上报这些输入事件即可。为此 input 子系统分为 input 驱动 层、input 核心层、input 事件处理层，最终给用户空间提供可访问的设备节点。

![image-20231030103338115](image\79.png)

我 们编写驱动程序的时候只需要关注中间的驱动层、核心层和事件层，这三个层的分工如下：

**驱动层**：输入设备的具体驱动程序，比如按键驱动程序，向内核层报告输入内容。

**核心层**：承上启下，为驱动层提供输入设备注册和操作接口。通知事件层对输入事件进行 处理。

**事件层**：主要和用户空间进行交互。

#### input 驱动编写流程

input 核心层会向 Linux 内核注册一个字符设备，drivers/input/input.c 这个文件， input.c 就是 input 输入子系统的核心层，此文件里面有如下所示代码：

```c
1767 struct class input_class = {
1768 	.name = "input",
1769 	.devnode = input_devnode,
1770 };
......
2414 static int __init input_init(void)
2415 {
2416 	int err;
2417	
2418 	err = class_register(&input_class);
2419 	if (err) {
2420 		pr_err("unable to register input_dev class\n");
2421 		return err;
2422 	}
2423	
2424 	err = input_proc_init();
2425 	if (err)
2426 		goto fail1;
2427	
2428 	err = register_chrdev_region(MKDEV(INPUT_MAJOR, 0),
2429 					INPUT_MAX_CHAR_DEVICES, "input");
2430 	if (err) {
2431 		pr_err("unable to register char major %d", INPUT_MAJOR);
2432 		goto fail2;
2433 	}
2434	
2435 	return 0;
2436	
2437 fail2: input_proc_exit();
2438 fail1: class_unregister(&input_class);
2439 	return err;
2440 }
```

第 2418 行，注册一个 input 类，这样系统启动以后就会在/sys/class 目录下有一个 input 子 目录

第 2428~2429 行，注册一个字符设备，主设备号为 INPUT_MAJOR，INPUT_MAJOR 定义 在 include/uapi/linux/major.h 文件中:
	\#define INPUT_MAJOR 13
因此，input 子系统的所有设备主设备号都为 13，在使用 input 子系统处理输入设备的时候就不需要去注册字符设备了，我们只需要向系统注册一个 input_device 即可。

##### 1、注册 input_dev

在使用 input 子系统的时候只需要注册一个 input 设备即可，input_dev 结构体表示 input 设备，此结构体定义在 include/linux/input.h 文件中：

```c
121 struct input_dev {
122 	const char *name;
123 	const char *phys;
124 	const char *uniq;
125 	struct input_id id;
126	
127 	unsigned long propbit[BITS_TO_LONGS(INPUT_PROP_CNT)];
128	
129 	unsigned long evbit[BITS_TO_LONGS(EV_CNT)]; /* 事件类型的位图 */
130 	unsigned long keybit[BITS_TO_LONGS(KEY_CNT)]; /* 按键值的位图 */
131 	unsigned long relbit[BITS_TO_LONGS(REL_CNT)]; /* 相对坐标的位图 */ 
132 	unsigned long absbit[BITS_TO_LONGS(ABS_CNT)]; /* 绝对坐标的位图 */
133 	unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)]; /* 杂项事件的位图 */
134 	unsigned long ledbit[BITS_TO_LONGS(LED_CNT)]; /*LED 相关的位图 */
135 	unsigned long sndbit[BITS_TO_LONGS(SND_CNT)];/* sound 有关的位图*/
136 	unsigned long ffbit[BITS_TO_LONGS(FF_CNT)]; /* 压力反馈的位图 */
137 	unsigned long swbit[BITS_TO_LONGS(SW_CNT)]; /*开关状态的位图 */
......
189 	bool devres_managed;
190 };
```

第 129 行，evbit 表示输入事件类型，可选的事件类型定义在 include/uapi/linux/input.h 文件 中，事件类型如下：

```c
#define EV_SYN 0x00 		/* 同步事件 */
#define EV_KEY 0x01 		/* 按键事件 */
#define EV_REL 0x02 		/* 相对坐标事件 */
#define EV_ABS 0x03 		/* 绝对坐标事件 */
#define EV_MSC 0x04 		/* 杂项(其他)事件 */
#define EV_SW 0x05  		/* 开关事件 */
#define EV_LED 0x11 		/* LED */
#define EV_SND 0x12 		/* sound(声音) */
#define EV_REP 0x14 		/* 重复事件 */
#define EV_FF 0x15  		/* 压力事件 */
#define EV_PWR 0x16 		/* 电源事件 */
#define EV_FF_STATUS 0x17   /* 压力状态事件 */

```

例如下面马上使用到的按键，就需要注册EV_KEY事件，如果要使用连按功能还需要注册EV_REP 事件。

第 129 行~137 行的 evbit、keybit、relbit 等等都是存放不同事件对应的值。比如要使用按键事件，因此要用到 keybit，keybit 就是按键事件使用的位图，Linux 内核定义了很多按键值，这些按键值定义在 include/uapi/linux/input.h 文件中，按键值如下：

```c
#define KEY_RESERVED 0
#define KEY_ESC 1
#define KEY_1 2
#define KEY_2 3
#define KEY_3 4
#define KEY_4 5
#define KEY_5 6
#define KEY_6 7
#define KEY_7 8
#define KEY_8 9
#define KEY_9 10
#define KEY_0 11
......
#define BTN_TRIGGER_HAPPY39 0x2e6
#define BTN_TRIGGER_HAPPY40 0x2e7
```

可以将开发板上的按键值设置为上面的任意一个，比如等会将开发板上的KEY设置为KEY_0。

在编写 input 设备驱动的时候我们需要先申请一个 input_dev 结构体变量，使用 input_allocate_device 函数来**申请**一个 input_dev，此函数原型如下所示：
![image-20231030105201841](image\80.png)

要注销的 input 设备的话需要使用 input_free_device 函数来释放掉前面申请到的 input_dev，input_free_device 函数原型如下：
![image-20231030105321368](image\81.png)

申请好一个 input_dev 以后就需要**初始化**这个 input_dev，需要初始化的内容主要为事件类 型(evbit)和事件值(keybit)这两种。

input_dev 初始化完成以后就需要**向 Linux 内核注册 input_dev** 了，需要用到 input_register_device 函数，此函数原型如下：

![image-20231030105852731](image\82.png)

同样的，注销 input 驱动的时候也需要使用 input_unregister_device 函数来注销掉前面注册 的 input_dev，input_unregister_device 函数原型如下：
![image-20231030105920769](image\83.png)

##### 2、上报输入事件

向 Linux 内核注册好 input_dev 以后还不能高枕无忧的使用 input 设备，input 设备都 是具有输入功能的，但是具体是什么样的输入值 Linux 内核是不知道的，我们需要获取到具体 的输入值，或者说是输入事件，然后将输入事件上报给 Linux 内核。比如按键，我们需要在按 键中断处理函数，或者消抖定时器中断函数中将按键值上报给 Linux 内核，这样 Linux 内核才 能获取到正确的输入值。不同的事件，其上报事件的 API 函数不同，我们依次来看一下一些常 用的事件上报 API 函数。

首先是 input_event 函数，此函数用于上报指定的事件以及对应的值，函数原型如下：
![image-20231030110321473](image\84.png)

input_event 函数可以上报所有的事件类型和事件值，Linux 内核也提供了其他的针对具体 事件的上报函数，这些函数其实都用到了 input_event 函数。

比如上报按键所使用的 input_report_key 函数：
![image-20231030110430810](image\85.png)

同样的还有一些其他的事件上报函数，这些函数如下所示：

```c
void input_report_rel(struct input_dev *dev, unsigned int code, int value)
void input_report_abs(struct input_dev *dev, unsigned int code, int value)
void input_report_ff_status(struct input_dev *dev, unsigned int code, int value)
void input_report_switch(struct input_dev *dev, unsigned int code, int value)
void input_mt_sync(struct input_dev *dev)
```

当我们上报事件以后还需要**使用 input_sync 函数来告诉 Linux 内核 input 子系统上报结束**， input_sync 函数本质是上报一个同步事件，此函数原型如下所示：

![image-20231030110540253](image\86.png)

#### input_event 结构体

Linux 内核使用 input_event 这个结构体来表示所有的输入事件，input_envent 结构体定义在 include/uapi/linux/input.h 文件中

```c
struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};
```

input_event 结构体中的各个成员变量:

1. time：时间，也就是此事件发生的时间，为 timeval 结构体类型

   timeval 结构体定义如下：
   ![image-20231030110813861](image\87.png)
   tv_sec 和 tv_usec 这两个成员变量都为 long 类型，也就是 **32 位**，这个一定要记住

2. type：事件类型.比如 EV_KEY，表示此次事件为按键事件，此成员变量为 **16 位**。

3. code：事件码.比如在 EV_KEY 事件中 code 就表示具体的按键码，如：KEY_0、KEY_1 等等这些按键。此成员变量为 **16 位**。

4. value：值.比如 EV_KEY 事件中 value 就是按键值，表示按键有没有被按下，如果为 1 的 话说明按键按下，如果为 0 的话说明按键没有被按下或者按键松开了。

input_envent 这个结构体非常重要，因为所有的输入设备最终都是按照 input_event 结构体 呈现给用户的，用户应用程序可以通过 input_event 来获取到具体的输入事件或相关的值，比如 按键值等。

### Linux下LCD驱动

#### Framebuffer 设备

##### 裸机下LCD驱动的编写

1. 初始化 I.MX6U 的 eLCDIF 控制器，重点是 LCD 屏幕宽(width)、高(height)、hspw、
   hbp、hfp、vspw、vbp 和 vfp 等信息。
2. 初始化 LCD 像素时钟。
3. 设置 RGBLCD 显存。
4. 应用程序直接通过操作显存来操作 LCD，实现在 LCD 上显示字符、图片等信息。

在 Linux 中应用程序最终也是通过操作 RGB LCD 的显存来实现在 LCD 上显示字符、图片 等信息。在裸机中我们可以随意的分配显存，但是在 Linux 系统中内存的管理很严格，显存是 **需要申**请的，而且因为虚拟内存的存在，驱动程序设置的显存和应用程 序访问的显存要是同一片物理内存。

**Framebuffer**就是为了解决上面的问题，帧缓冲，简称fb。

fb 是一种机制，将系统中所有跟显示有关的硬件以及软件集合起来，虚拟出一 个 fb 设备，当我们编写好 LCD 驱动以后会生成一个名为/dev/fbX(X=0~n)的设备。

应用程序通 过访问/dev/fbX 这个设备就可以访问 LCD。NXP 官方的 Linux 内核默认已经开启了 LCD 驱动， 因此我们是可以看到/dev/fb0 这样一个设备。/dev/fb0 是个字符设备，因此肯定有 file_operations 操作集，fb 的 file_operations 操作集定义在 drivers/video/fbdev/core/fbmem.c 文件中：

```c
static const struct file_operations fb_fops = {
	.owner = THIS_MODULE,
	.read = fb_read,
	.write = fb_write,
	.unlocked_ioctl = fb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fb_compat_ioctl,
#endif
	.mmap = fb_mmap,
	.open = fb_open,
	.release = fb_release,
#ifdef HAVE_ARCH_FB_UNMAPPED_AREA
	.get_unmapped_area = get_fb_unmapped_area,
#endif
#ifdef CONFIG_FB_DEFERRED_IO
	.fsync = fb_deferred_io_fsync,
#endif
	.llseek = default_llseek,
};
```

#### LCD驱动简析

LCD 裸机例程主要分两部分：

1. 获取 LCD 的屏幕参数。
2. 根据屏幕参数信息来初始化 eLCDIF 接口控制器。

不同分辨率的 LCD 屏幕其 eLCDIF 控制器驱动代码都是一样的，只需要修改好对应的屏 幕参数即可。屏幕参数信息属于屏幕设备信息内容，这些肯定是要放到设备树中的，因此我们本实验的主要工作就是修改设备数。

NXP 官方的设备树已经添加了 LCD 设备节点，只是此 节点的 LCD 屏幕信息是针对 NXP 官方 EVK 开发板所使用的 4.3 寸 480*272 编写的，我们需 要将其改为我们所使用的屏幕参数。

看一下 NXP 官方编写的 Linux 下的 LCD 驱动，打开 imx6ull.dtsi，然后找到 lcdif 节点内容，如下所示：
![image-20231031105921524](image\88.png)

lcdif 节点信息是所有使用 I.MX6ULL 芯片的板子所共有的，并不是 完整的 lcdif 节点信息。像屏幕参数这些需要根据不同的硬件平台去添加。可以看出 lcdif 节点 的 compatible 属性值为“fsl,imx6ul-lcdif”和“fsl,imx28-lcdif”，因此在 Linux 源码中搜索这两个 字符串即可找到 I.MX6ULL 的 LCD 驱动文件，这个文件为 drivers/video/fbdev/mxsfb.c，mxsfb.c 就是 I.MX6ULL 的 LCD 驱动文件，在此文件中找到如下内容：

```c
static const struct of_device_id mxsfb_dt_ids[] = {
	{ .compatible = "fsl,imx23-lcdif", .data = &mxsfb_devtype[0], },
	{ .compatible = "fsl,imx28-lcdif", .data = &mxsfb_devtype[1], },
	{ /* sentinel */ }
};
.....
static struct platform_driver mxsfb_driver = {
	.probe = mxsfb_probe,
	.remove = mxsfb_remove,
	.shutdown = mxsfb_shutdown,
	.id_table = mxsfb_devtype,
	.driver = {
	.name = DRIVER_NAME,
	.of_match_table = mxsfb_dt_ids,
	.pm = &mxsfb_pm_ops,
	},
};

module_platform_driver(mxsfb_driver);
```

这是一个标准的 platform 驱动，当驱动和设备匹配以后 mxsfb_probe 函数就会执行

 Linux 下 Framebuffer 驱动的编写流程:Linux 内核将所有的 Framebuffer 抽象为一个叫做 fb_info 的结构 体，fb_info 结构体包含了 Framebuffer 设备的完整属性和操作集合，因此每一个 Framebuffer 设 备都必须有一个 fb_info。换言之就是，**LCD 的驱动就是构建 fb_info，并且向系统注册 fb_info 的过程**。fb_info 结构体定义在 include/linux/fb.h 文件里面:

```c
struct fb_info {
	atomic_t count;
	int node;
	int flags;
	struct mutex lock; /* 互斥锁 */
	struct mutex mm_lock; /* 互斥锁，用于 fb_mmap 和 smem_*域*/
	struct fb_var_screeninfo var; /* 当前可变参数 */
	struct fb_fix_screeninfo fix; /* 当前固定参数 */
	struct fb_monspecs monspecs; /* 当前显示器特性 */
	struct work_struct queue; /* 帧缓冲事件队列 */
	struct fb_pixmap pixmap; /* 图像硬件映射 */
	struct fb_pixmap sprite; /* 光标硬件映射 */
	struct fb_cmap cmap; /* 当前调色板 */
	struct list_head modelist; /* 当前模式列表 */
	struct fb_videomode *mode; /* 当前视频模式 */

#ifdef CONFIG_FB_BACKLIGHT /* 如果 LCD 支持背光的话 */
	/* assigned backlight device */
	/* set before framebuffer registration, 
	remove after unregister */
	struct backlight_device *bl_dev; /* 背光设备 */
	
	/* Backlight level curve */
	struct mutex bl_curve_mutex; 
	u8 bl_curve[FB_BACKLIGHT_LEVELS];
#endif
......
	struct fb_ops *fbops; /* 帧缓冲操作函数集 */ 
	struct device *device; /* 父设备 */
	struct device *dev; /* 当前 fb 设备 */
	int class_flag; /* 私有 sysfs 标志 */
	..
	char __iomem *screen_base; /* 虚拟内存基地址(屏幕显存) */
	unsigned long screen_size; /* 虚拟内存大小(屏幕显存大小) */
	void *pseudo_palette; /* 伪 16 位调色板 */
......
};
```

重点关注 var、fix、fbops、screen_base、screen_size 和 pseudo_palette。

mxsfb_probe 函数的主要工作内容为：

1. 申请 fb_info。
2. 初始化 fb_info 结构体中的各个成员变量。
3. 初始化 eLCDIF 控制器。
4. 使用 register_framebuffer 函数向 Linux 内核注册初始化好的 fb_info
   register_framebuffer 函数原型如下：
   ![image-20231031111728501](image\89.png)

mxsfb_probe 函数（见开发指南P1423）

### Linux RTC驱动实验

RTC 设备驱动是一个标准的字符设备驱动，应用程序通过 open、release、read、write 和 ioctl 等函数完成对 RTC 设备的操作。RTC 硬件原理：开发指南C25

Linux 内核将 RTC 设备抽象为 **rtc_device 结构体**，因此 RTC 设备驱动就是申请并初始化 rtc_device，最后将 rtc_device 注册到 Linux 内核里面，这样 Linux 内核就有一个 RTC 设备了。此结构体定义在 include/linux/rtc.h 文件中:

```c
struct rtc_device
{
	struct device dev; /* 设备 */
	struct module *owner;
	
	int id; /* ID */ 
	char name[RTC_DEVICE_NAME_SIZE]; /* 名字 */
	
	const struct rtc_class_ops *ops; /* RTC 设备底层操作函数 */
	struct mutex ops_lock;
	
	struct cdev char_dev; /* 字符设备 */
	unsigned long flags;
	
	unsigned long irq_data;
	spinlock_t irq_lock;
	wait_queue_head_t irq_queue;
	struct fasync_struct *async_queue;
	
	struct rtc_task *irq_task;
	spinlock_t irq_task_lock;
	int irq_freq;
	int max_user_freq;
	
	struct timerqueue_head timerqueue;
	struct rtc_timer aie_timer;
	struct rtc_timer uie_rtctimer;
	struct hrtimer pie_timer; /* sub second exp, so needs hrtimer */
	int pie_enabled;
	struct work_struct irqwork;
	/* Some hardware can't support UIE mode */
	int uie_unsupported;
......
};
```

显然RTC设备结构体中**struct rtc_class_ops *ops**是操作函数集合，这是一个 rtc_class_ops 类型的指针变量，rtc_class_ops 为 RTC 设备的最底层操作函数集合，包括从 RTC 设备中读取时间、向 RTC 设备写入新的时间值等。此结构体定义在 include/linux/rtc.h 文件中，内容如下：

```c
struct rtc_class_ops {
	int (*open)(struct device *);
	void (*release)(struct device *);
	int (*ioctl)(struct device *, unsigned int, unsigned long);
	int (*read_time)(struct device *, struct rtc_time *);
	int (*set_time)(struct device *, struct rtc_time *);
	int (*read_alarm)(struct device *, struct rtc_wkalrm *);
	int (*set_alarm)(struct device *, struct rtc_wkalrm *);
	int (*proc)(struct device *, struct seq_file *);
	int (*set_mmss64)(struct device *, time64_t secs);
	int (*set_mmss)(struct device *, unsigned long secs);
	int (*read_callback)(struct device *, int data);
	int (*alarm_irq_enable)(struct device *, unsigned int enabled);
};

```

注意， rtc_class_ops 中的这些函数只是最底层的 RTC 设备操作函数，并不是提供给应用层的 file_operations 函数操作集。RTC 是个字符设备，那么肯定有字符设备的 file_operations 函数操 作集，Linux 内核提供了一个 RTC 通用字符设备驱动文件，文件名为 drivers/rtc/rtc-dev.c，其中提供了所有RTC设备公用的file_operations 函数操作集：

```c
static const struct file_operations rtc_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = rtc_dev_read,
	.poll = rtc_dev_poll,
	.unlocked_ioctl = rtc_dev_ioctl,
	.open = rtc_dev_open,
	.release = rtc_dev_release,
	.fasync = rtc_dev_fasync,
};

```

应用程序可以通过 ioctl 函 数来设置/读取时间、设置/读取闹钟的操作，那么对应的 **rtc_dev_ioctl** 函数就会执行， rtc_dev_ioctl 最终会通过操作 rtc_class_ops 中的 read_time、set_time 等函数来对具体 RTC 设备 的读写操作。 
rtc_dev_ioctl 函数：见开发指南P1438。

Linux 内核中 RTC 驱动调用流程：
![image-20231101103142087](image\90.png)

当 rtc_class_ops 准备好以后需要将其注册到 Linux 内核中，这里我们可以使用 rtc_device_register函数完成注册工作。此函数会申请一个rtc_device并且初始化这个rtc_device， 最后向调用者返回这个 rtc_device，此函数原型如下：
![image-20231101105521873](image\92.png)

当卸载 RTC 驱动的时候需要调用 rtc_device_unregister 函数来注销注册的 rtc_device，函数 原型如下：
![image-20231101105537416](image\91.png)

还有另外一对 rtc_device 注册函数 devm_rtc_device_register 和 devm_rtc_device_unregister， 分别为注册和注销 rtc_device。

#### IMX6ULL内部RTC驱动分析

对 于大多数的 SOC 来讲，内部 RTC 驱动都不需要我们去编写，半导体厂商会编写好。虽然不用编写 RTC 驱动，但是我们得看一下这些原厂是怎么编写 RTC 驱动。

见开发指南P1440

linux中date查看事件，date -s设置时间，hwclock -w将当前系统时间写入到 RTC 里面。

### Linux I2C驱动

#### 裸机中的I2C驱动

在裸机中编写的关于AP3216C的驱动，一共有四个文件：bsp_i2c.c、 bsp_i2c.h、bsp_ap3216c.c 和 bsp_ap3216c.h。其中前两个是I.MX6U 的 IIC 接口驱动，后两个文 件是 AP3216C 这个 I2C 设备驱动文件。相当于有两部分驱动：

1. I2C 主机驱动。
2. I2C 设备驱动。

对于 I2C 主机驱动，一旦编写完成就不需要再做修改，其他的 I2C 设备直接调用主机驱动 提供的 API 函数完成读写操作即可。这个正好符合 Linux 的驱动分离与分层的思想，因此 Linux 内核也将 I2C 驱动分为两部分：

1. I2C 总线驱动，I2C 总线驱动就是 SOC 的 I2C 控制器驱动，也叫做 I2C 适配器驱动。
2. I2C 设备驱动，I2C 设备驱动就是针对具体的 I2C 设备而编写的驱动。

#### I2C 总线驱动

platform 是虚拟出来的一条总线， 目的是为了实现总线、设备、驱动框架。但是对于I2C 而言，不需要虚拟出一条总线，直接使用 I2C 总线即可。I2C 总线驱动重点是 I2C 适配器(也就是 SOC 的 I2C 接口控制器)驱动，这里要用到 两个重要的数据结构：**i2c_adapter 和 i2c_algorithm**，Linux 内核将 SOC 的 I2C 适配器(控制器) 抽象成 i2c_adapter，i2c_adapter 结构体定义在 include/linux/i2c.h 文件中，结构体内容如下：

```c
struct i2c_adapter {
	struct module *owner;
	unsigned int class; /* classes to allow probing for */
	const struct i2c_algorithm *algo; /* 总线访问算法 */
	void *algo_data;
	
	/* data fields that are valid for all devices */
	struct rt_mutex bus_lock;
	
	int timeout; /* in jiffies */
	int retries;
	struct device dev; /* the adapter device */
	
	int nr;
	char name[48];
	struct completion dev_released;
	
	struct mutex userspace_clients_lock;
	struct list_head userspace_clients;
	
	struct i2c_bus_recovery_info *bus_recovery_info;
	const struct i2c_adapter_quirks *quirks;
};

```

i2c_algorithm 类型的指针变量 algo，对于一个 I2C 适配器，肯定要对外提供读 写 API 函数，设备驱动程序可以使用这些 API 函数来完成读写操作。i2c_algorithm 就是 I2C 适配器与 IIC 设备进行通信的方法。

i2c_algorithm 结构体定义在 include/linux/i2c.h 文件中：

```c
struct i2c_algorithm {
.....
	int (*master_xfer)(struct i2c_adapter *adap,
							struct i2c_msg *msgs,int num);
	int (*smbus_xfer) (struct i2c_adapter *adap, u16 addr,
	unsigned short flags, char read_write,
	u8 command, int size, union i2c_smbus_data *data);
	
	/* To determine what the adapter supports */
	u32 (*functionality) (struct i2c_adapter *);
.....
};

```

master_xfer 就是 I2C 适配器的传输函数，可以通过此函数来完成与 IIC 设备之 间的通信。
smbus_xfer 就是 SMBUS 总线的传输函数。

综上所述，I2C 总线驱动，或者说 I2C 适配器驱动的主要工作就是初始化 i2c_adapter 结构 体变量，然后设置 i2c_algorithm 中的 master_xfer 函数。完成以后通过 i2c_add_numbered_adapter 或 i2c_add_adapter 这两个函数向系统注册设置好的 i2c_adapter，这两个函数的原型如下：
![image-20231101124803682](image\93.png)

#### I2C 设备驱动

I2C 设备驱动重点关注两个数据结构：i2c_client 和 i2c_driver，根据总线、设备和驱动模型,I2C 总线上一小节已经讲了。还剩下设备和驱动，i2c_client 就是描述设备信息的，i2c_driver 描述驱动内容，类似于 platform_driver。

##### 1、i2c_client 结构体

i2c_client 结构体定义在 include/linux/i2c.h 文件中，内容如下：

```c
struct i2c_client {
	unsigned short flags; /* 标志 */
	unsigned short addr; /* 芯片地址，7 位，存在低 7 位*/
...
	char name[I2C_NAME_SIZE]; /* 名字 */
	struct i2c_adapter *adapter; /* 对应的 I2C 适配器 */
	struct device dev; /* 设备结构体 */
	int irq; /* 中断 */
	struct list_head detected;
...
};
```

一个设备对应一个 i2c_client，每检测到一个 I2C 设备就会给这个 I2C 设备分配一个 i2c_client。

##### 2、i2c_driver 结构体

i2c_driver 类似 platform_driver，是我们编写 I2C 设备驱动重点要处理的内容，i2c_driver 结 构体定义在 include/linux/i2c.h 文件中，内容如下：

```c
struct i2c_driver {
	unsigned int class;
	
	/* Notifies the driver that a new bus has appeared. You should 
	* avoid using this, it will be removed in a near future.
	*/
	int (*attach_adapter)(struct i2c_adapter *) __deprecated;
	
	/* Standard driver model interfaces */
	int (*probe)(struct i2c_client *, const struct i2c_device_id *);
	int (*remove)(struct i2c_client *);
	
	/* driver model interfaces that don't relate to enumeration */
	void (*shutdown)(struct i2c_client *);
	
	/* Alert callback, for example for the SMBus alert protocol.
	* The format and meaning of the data value depends on the 
	* protocol.For the SMBus alert protocol, there is a single bit 
	* of data passed as the alert response's low bit ("event 
	flag"). */
	void (*alert)(struct i2c_client *, unsigned int data);
	
	/* a ioctl like command that can be used to perform specific 
	* functions with the device.
	*/
	int (*command)(struct i2c_client *client, unsigned int cmd,*arg);
	
	struct device_driver driver;
	const struct i2c_device_id *id_table;
	
	/* Device detection callback for automatic device creation */
	int (*detect)(struct i2c_client *, struct i2c_board_info *);
	const unsigned short *address_list;
	struct list_head clients;
};

```

当 I2C 设备和驱动匹配成功以后 probe 函数就会执行，和 platform 驱动一样。
device_driver 驱动结构体，如果使用设备树的话，需要设置 device_driver 的 of_match_table 成员变量，也就是驱动的兼容(compatible)属性。
id_table 是传统的、未使用设备树的设备匹配 ID 表。

对于我们 I2C 设备驱动编写人来说，重点工作就是构建 i2c_driver，构建完成以后需要向 Linux 内核注册这个 i2c_driver。i2c_driver 注册函数为 int i2c_register_driver，此函数原型如下：
![image-20231101125741199](image\94.png)

另外 i2c_add_driver 也常常用于注册 i2c_driver，i2c_add_driver 是一个宏，定义如下：
![image-20231101125802520](image\95.png)

注销 I2C 设备驱动的时候需要将前面注册的 i2c_driver 从 Linux 内核中注销掉，需要用到 i2c_del_driver 函数，此函数原型如下：
![image-20231101125836462](image\96.png)

#### I2C 设备和驱动匹配过程

I2C 设备和驱动的匹配过程是由 I2C 核心来完成的，drivers/i2c/i2c-core.c 就是 I2C 的核心 部分，I2C 核心提供了一些与具体硬件无关的 API 函数，比如前面讲过的：

1. ##### i2c_adapter 注册/注销函数

   ![image-20231101130518903](image\97.png)

2. ##### i2c_driver 注册/注销函数

   ![image-20231101130549241](image\98.png)

   设备和驱动的匹配过程也是由 I2C 总线完成的，I2C 总线的数据结构为 i2c_bus_type，定义 在 drivers/i2c/i2c-core.c 文件，i2c_bus_type 内容如下：

   ```c
   struct bus_type i2c_bus_type = {
   	.name = "i2c",
   	.match = i2c_device_match,
   	.probe = i2c_device_probe,
   	.remove = i2c_device_remove,
   	.shutdown = i2c_device_shutdown,
   };
   ```

   .match 就是 I2C 总线的设备和驱动匹配函数，在这里就是 i2c_device_match 这个函数，此函数内容如下：

   ```c
   static int i2c_device_match(struct device *dev, struct device_driver *drv)
   {
   	struct i2c_client *client = i2c_verify_client(dev);
   	struct i2c_driver *driver;
   	
   	if (!client)
   	return 0;
   	
   	/* Attempt an OF style match */
   	if (of_driver_match_device(dev, drv))
   	return 1;
   	
   	/* Then ACPI style match */
   	if (acpi_driver_match_device(dev, drv))
   	return 1;
   	
   	driver = to_i2c_driver(drv);
   	/* match on an id table if there is one */
   	if (driver->id_table)
   	return i2c_match_id(driver->id_table, client) != NULL;
   	
   	return 0;
   }
   ```

####  I.MX6U 的 I2C 适配器驱动分析

Linux 下的 I2C 驱动框架，重点分为 I2C 适配器驱动和 I2C 设备驱动， 其中 I2C 适配器驱动就是 SOC 的 I2C 控制器驱动。I2C 设备驱动是需要用户根据不同的 I2C 设 备去编写，而 I2C 适配器驱动一般都是 SOC 厂商去编写的，比如 NXP 就编写好了 I.MX6U 的 I2C 适配器驱动。

见开发指南P1455

#### I2C设备驱动编写流程

##### I2C设备信息描述

1. 未使用设备树时：

   在未使用设备树的时候需要在 BSP 里面使用 i2c_board_info 结构体来描 述一个具体的 I2C 设备。i2c_board_info 结构体如下：

   ```c
   struct i2c_board_info {
   	char type[I2C_NAME_SIZE]; /* I2C 设备名字 */
   	unsigned short flags; /* 标志 */
   	unsigned short addr; /* I2C 器件地址 */
   	void *platform_data; 
   	struct dev_archdata *archdata;
   	struct device_node *of_node;
   	struct fwnode_handle *fwnode;
   	int irq;
   };
   
   ```

   type 和 addr 这两个成员变量是必须要设置的，一个是 I2C 设备的名字，一个是 I2C 设备的 器件地址。

   **I2C_BOARD_INFO** 是一个初始化i2c_board_info结构体宏，定义如下：

   ```c
   #define I2C_BOARD_INFO(dev_type, dev_addr) \
   						.type = dev_type, .addr = (dev_addr)
   ```

2. 使用设备树

   使用设备树的时候 I2C 设备信息通过创建相应的节点就行了，比如 NXP 官方的 EVK 开发 板在 I2C1 上接了 mag3110 这个磁力计芯片，因此必须在 i2c1 节点下创建 mag3110 子节点，然 后在这个子节点内描述 mag3110 这个芯片的相关信息。打开 imx6ull-14x14-evk.dts 这个设备树 文件，然后找到如下内容：

   ```
   &i2c1 {
   	clock-frequency = <100000>;
   	pinctrl-names = "default";
   	pinctrl-0 = <&pinctrl_i2c1>;
   	status = "okay";
   	
   	mag3110@0e {
   		compatible = "fsl,mag3110";
   		reg = <0x0e>;
   		position = <2>;
   	 };
   	....
    };
   ```

##### I2C 设备数据收发处理流程

I2C 设备驱动首先要做的就是初始化 i2c_driver 并向 Linux 内核 注册。当设备和驱动匹配以后 i2c_driver 里面的 probe 函数就会执行，probe 函数里面所做的就是字符设备驱动那一套了。

一般需要在 probe 函数里面初始化 I2C 设备，要初始化 I2C 设备就 必须能够对 I2C 设备寄存器进行读写操作，这里就要用到 i2c_transfer 函数了。i2c_transfer 函数 最终会调用 I2C 适配器中 i2c_algorithm 里面的 master_xfer 函数，

对于 I.MX6U 而言就是 i2c_imx_xfer 这个函数。i2c_transfer 函数原型如下：

![image-20231102112430430](image\99.png)

重点来看一下 msgs 这个参数，这是一个 i2c_msg 类型的指针参数，I2C 进行数据收发 说白了就是消息的传递，Linux 内核使用 i2c_msg 结构体来描述一个消息。i2c_msg 结构体定义 在 include/uapi/linux/i2c.h 文件中，结构体内容如下：

```c
struct i2c_msg {
	__u16 addr; /* 从机地址 */
	__u16 flags; /* 标志 */
	#define I2C_M_TEN 0x0010
	#define I2C_M_RD 0x0001
	#define I2C_M_STOP 0x8000
	#define I2C_M_NOSTART 0x4000
	#define I2C_M_REV_DIR_ADDR 0x2000 
	#define I2C_M_IGNORE_NAK 0x1000 
	#define I2C_M_NO_RD_ACK 0x0800
	#define I2C_M_RECV_LEN 0x0400
	__u16 len; /* 消息(本 msg)长度 */
	__u8 *buf; /* 消息数据 */
};
```

使用 i2c_transfer 函数发送数据之前要先构建好 i2c_msg

另外还有两个API函数分别用于I2C数据的收发操作，这两个函数最终都会调用i2c_transfer。 首先来看一下 I2C 数据发送函数 i2c_master_send，函数原型如下：
![image-20231102112754611](image\100.png)

I2C 数据接收函数为 i2c_master_recv，函数原型如下：
![image-20231102112815107](image\101.png)

### Linux SPI驱动

SPI 驱动框架和 I2C 很类似，都分为主机控制器驱动和设备驱动，主机控制器也就是 SOC 的 SPI 控制器接口。不管是什么 SPI 设备，SPI 控制器部分的驱动都是一样，我们的重点就落在了 种类繁多的 SPI 设备驱动。

#### SPI主机驱动

SPI 主机驱动就是 SOC 的 SPI 控制器驱动，类似 I2C 驱动里面的适配器驱动。Linux 内核 使用 spi_master 表示 SPI 主机驱动，spi_master 是个结构体，定义在 include/linux/spi/spi.h 文件中：

```c
struct spi_master {
	struct device dev;
	struct list_head list;
	.....
	s16 bus_num;

	/* chipselects will be integral to many controllers; some others
	* might use board-specific GPIOs.
	*/
	u16 num_chipselect;
	
	/* some SPI controllers pose alignment requirements on DMAable
	* buffers; let protocol drivers know about these requirements.
	*/
	u16 dma_alignment;
	
	/* spi_device.mode flags understood by this controller driver */
	u16 mode_bits;
	
	/* bitmask of supported bits_per_word for transfers */
	u32 bits_per_word_mask;
	.....
	/* limits on transfer speed */
	u32 min_speed_hz;
	u32 max_speed_hz;
	
	/* other constraints relevant to this driver */
	u16 flags;
	.....
	/* lock and mutex for SPI bus locking */
	spinlock_t bus_lock_spinlock;
	struct mutex bus_lock_mutex;
	
	/* flag indicating that the SPI bus is locked for exclusive use */
	bool bus_lock_flag;
	..
	int (*setup)(struct spi_device *spi);
	
	.....
	int (*transfer)(struct spi_device *spi,
	struct spi_message *mesg);
	.....
	int (*transfer_one_message)(struct spi_master *master,
	struct spi_message *mesg);
	.....
};
```

transfer 函数，和 i2c_algorithm 中的 master_xfer 函数一样，控制器数据传输函 数。
transfer_one_message 函数，也用于 SPI 数据发送，用于发送一个 spi_message， SPI 的数据会打包成 spi_message，然后以队列方式发送出去。
也就是 SPI 主机端最终会通过 transfer 函数与 SPI 设备进行通信，因此对于 SPI 主机控制器的驱 动编写者而言 transfer 函数是需要实现的，因为不同的 SOC 其 SPI 控制器不同，寄存器都不一 样。和 I2C 适配器驱动一样，SPI 主机驱动一般都是 SOC 厂商去编写的，所以我们作为 SOC 的 使用者，这一部分的驱动就不用操心了，除非你是在 SOC 原厂工作，内容就是写 SPI 主机驱 动。
SPI 主机驱动的核心就是申请 spi_master，然后初始化 spi_master，最后向 Linux 内核注册 spi_master。

1. spi_master申请与释放

   spi_alloc_master 函数用于申请 spi_master，函数原型如下：
   ![image-20231103112654288](image\102.png)
   spi_master 的释放通过 spi_master_put 函数来完成，当我们删除一个 SPI 主机驱动的时候就 需要释放掉前面申请的 spi_master，spi_master_put 函数原型如下：

   ![image-20231103112740832](image\103.png)

2. spi_master 的注册与注销

   当 spi_master 初始化完成以后就需要将其注册到 Linux 内核，spi_master 注册函数为 spi_register_master，函数原型如下：
   ![image-20231103112823623](image\104.png)

   I.MX6U 的 SPI 主机驱动会采用 spi_bitbang_start 这个 API 函数来完成 spi_master 的注册， spi_bitbang_start 函数内部其实也是通过调用 spi_register_master 函数来完成 spi_master 的注册。

   如果要注销 spi_master 的话可以使用 spi_unregister_master 函数，此函数原型为：
   ![image-20231103112859675](image\105.png)
   如果使用 spi_bitbang_start 注册 spi_master 的话就要使用 spi_bitbang_stop 来注销掉 spi_master。

#### SPI设备驱动

spi 设备驱动和 i2c 设备驱动也很类似，Linux 内核使用 spi_driver 结构体来表示 spi 设备驱 动，我们在编写 SPI 设备驱动的时候需要实现 spi_driver 。spi_driver 结构体定义在 include/linux/spi/spi.h 文件中，结构体内容如下：

```c
struct spi_driver {
	const struct spi_device_id *id_table;
	int (*probe)(struct spi_device *spi);
	int (*remove)(struct spi_device *spi);
	void (*shutdown)(struct spi_device *spi);
	struct device_driver driver;
};
```

spi_driver 和 i2c_driver、platform_driver 基本一样，当 SPI 设备和驱动匹配成功 以后 probe 函数就会执行。

同样的，spi_driver 初始化完成以后需要向 Linux 内核注册，spi_driver 注册函数为 spi_register_driver，函数原型如下：
![image-20231103113117665](image\106.png)

注销 SPI 设备驱动以后也需要注销掉前面注册的 spi_driver，使用 spi_unregister_driver 函 数完成 spi_driver 的注销，函数原型如下：
![image-20231103113135942](image\107.png)

返回值：无

#### SPI 设备和驱动匹配过程

SPI 设备和驱动的匹配过程是由 SPI 总线来完成的，这点和 platform、I2C 等驱动一样，SPI 总线为 spi_bus_type，定义在 drivers/spi/spi.c 文件中：

```c
struct bus_type spi_bus_type = {
	.name = "spi",
	.dev_groups = spi_dev_groups,
	.match = spi_match_device,
	.uevent = spi_uevent,
};
```

SPI 设备和驱动的匹配函数为 spi_match_device，函数内容如下：

```c
static int spi_match_device(struct device *dev,
								struct device_driver *drv)
{
	const struct spi_device *spi = to_spi_device(dev);
	const struct spi_driver *sdrv = to_spi_driver(drv);
	
	/* Attempt an OF style match */
	if (of_driver_match_device(dev, drv))
	return 1;
	
	/* Then try ACPI */
	if (acpi_driver_match_device(dev, drv))
	return 1;
	
	if (sdrv->id_table)
	return !!spi_match_id(sdrv->id_table, spi);
	
	return strcmp(spi->modalias, drv->name) == 0;
}
```

of_driver_match_device 函数用于完成设备树设备和驱动匹配。比较 SPI 设备节 点的 compatible 属性和 of_device_id 中的 compatible 属性是否相等，如果相当的话就表示 SPI 设 备和驱动匹配。
acpi_driver_match_device 函数用于 ACPI 形式的匹配。
spi_match_id 函数用于传统的、无设备树的 SPI 设备和驱动匹配过程。比较 SPI 设备名字和 spi_device_id 的 name 字段是否相等，相等的话就说明 SPI 设备和驱动匹配。

#### imx6u spi主机驱动分析

开发指南P1488

#### SPI设备驱动编写流程

##### SPI 设备信息描述

1. IO 的 pinctrl 子节点创建与修改

   一定要检测相应的IO有没有被其他设备使用

2. SPI 设备节点的创建与修改

   采用设备树的情况下，SPI 设备信息描述就通过创建相应的设备子节点来完成，我们可以 打开 imx6qdl-sabresd.dtsi 这个设备树头文件，在此文件里面找到如下所示内容：

   ```
   &ecspi1 {
   	fsl,spi-num-chipselects = <1>;
   	cs-gpios = <&gpio4 9 0>;
   	pinctrl-names = "default";
   	pinctrl-0 = <&pinctrl_ecspi1>;
   	status = "okay";
   	
   	flash: m25p80@0 {
   		#address-cells = <1>;
   		#size-cells = <1>;
   		compatible = "st,m25p32";
   		spi-max-frequency = <20000000>;
   		reg = <0>;
   	};
   };
   ```

##### SPI 设备数据收发处理流程

SPI 设备驱动的核心是 spi_drive。r当我们向 Linux 内 核注册成功 spi_driver 以后就可以使用 SPI 核心层提供的 API 函数来对设备进行读写操作了。

首先是 spi_transfer 结构体，此结构体用于描述 SPI 传输信息，结构体内容如下：

```c
struct spi_transfer {
	/* it's ok if tx_buf == rx_buf (right?)
	* for MicroWire, one buffer must be null
	* buffers must work with dma_*map_single() calls, unless
	* spi_message.is_dma_mapped reports a pre-existing mapping
	*/
	const void *tx_buf;
	void *rx_buf;
	unsigned len;
	
	dma_addr_t tx_dma;
	dma_addr_t rx_dma;
	struct sg_table tx_sg;
	struct sg_table rx_sg;
	
	unsigned cs_change:1;
	unsigned tx_nbits:3;
	unsigned rx_nbits:3;
	#define SPI_NBITS_SINGLE 0x01 /* 1bit transfer */
	#define SPI_NBITS_DUAL 0x02 /* 2bits transfer */
	#define SPI_NBITS_QUAD 0x04 /* 4bits transfer */
	u8 bits_per_word;
	u16 delay_usecs;
	u32 speed_hz;
	
	struct list_head transfer_list;
};

```

tx_buf 保存着要发送的数据。
rx_buf 用于保存接收到的数据。
len 是要进行传输的数据长度，SPI 是全双工通信，因此在一次通信中发送和 接收的字节数都是一样的，所以 spi_transfer 中也就没有发送长度和接收长度之分。

spi_transfer 需要组织成 spi_message，spi_message 也是一个结构体，内容如下：

```c
struct spi_message {
	struct list_head transfers;
	
	struct spi_device *spi;
	
	unsigned is_dma_mapped:1;
	.....
	/* completion is reported through a callback */
	void (*complete)(void *context);
	void *context;
	unsigned frame_length;
	unsigned actual_length;
	int status;
	
	/* for optional use by whatever driver currently owns the
	* spi_message ... between calls to spi_async and then later
	* complete(), that's the spi_master controller driver.
	*/
	struct list_head queue;
	void *state;
};
```

在使用spi_message之前需要对其进行初始化，spi_message初始化函数为spi_message_init， 函数原型如下：
![image-20231103125921517](image\108.png)

spi_message 初始化完成以后需要将 spi_transfer 添加到 spi_message 队列中，这里我们要用 到 spi_message_add_tail 函数，此函数原型如下：
![image-20231103125951019](image\109.png)

spi_message 准备好以后就可以进行数据传输了，数据传输分为同步传输和异步传输，**同步传输会阻塞的等待 SPI 数据传输完成**，同步传输函数为 spi_sync，函数原型如下：
![image-20231103130029074](image\110.png)

异步传输不会阻塞的等到 SPI 数据传输完成，异步传输需要设置 spi_message 中的 complete 成员变量，complete 是一个回调函数，当 SPI 异步传输完成以后此函数就会被调用。SPI 异步传 输函数为 spi_async，函数原型如下：
![image-20231103130052385](image\111.png)

SPI 数据传输步骤如下：

1. 申请并初始化 spi_transfer，设置 spi_transfer 的 tx_buf 成员变量，tx_buf 为要发送的数 据。然后设置 rx_buf 成员变量，rx_buf 保存着接收到的数据。最后设置 len 成员变量，也就是 要进行数据通信的长度。
2. 使用 spi_message_init 函数初始化 spi_message。
3. 使用spi_message_add_tail函数将前面设置好的spi_transfer添加到spi_message 队列中。
4. 使用 spi_sync 函数完成 SPI 数据同步传输。

### Linux RS232/485/GPS驱动

串口是很常用的一个外设，在 Linux 下通常通过串口和其他设备或传感器进行通信，根据电平的不同，串口分为 TTL 和 RS232。不管是什么样的接口电平，其驱动程序都是一样的，通过外接 RS485 这样的芯片就可以将串口转换为 RS485 信号。

正点原子的ALPHA开发板，RS232、RS485以及GPS模块接口都连接在了UART3接口上，所以这些外设最终都归结为UART3的串口驱动。

#### Linux下UART驱动框架

1. uart_driver 注册与注销

​	同 I2C、SPI 一样，Linux 也提供了串口驱动框架，我们只需要按照相应的串口框架编写驱动程序即可。串口驱动没有主机端和设备端区别，只有一个串口驱动，而且这个串口驱动已经有NXP官方编写好了。所以只需要在设备树中添加要使用的串口节点信息。当系统启动以后串口驱动个设备匹配成功，相应的串口就会被驱动起来，并生成/dev/ttymxcX(X是数字)文件。

​	**串口驱动框架**：
​	uart_driver 结构体表示 UART 驱动，uart_driver 定义在 include/linux/serial_core.h 文件中：

```c
struct uart_driver {
	struct module *owner; /* 模块所属者 */
	const char *driver_name; /* 驱动名字 */
	const char *dev_name; /* 设备名字 */
	int major; /* 主设备号 */
	int minor; /* 次设备号 */
	int nr; /* 设备数 */
	
	struct console *cons; /* 控制台 */
	/* 305 * these are private; the low level driver should not
	* touch these; they should be initialised to NULL
	*/
	struct uart_state *state;
	struct tty_driver *tty_driver;
};
```

每个串口驱动都需要定义一个 uart_driver，加载驱动的时候通过 uart_register_driver 函数向
系统注册这个 uart_driver，此函数原型如下：
![image-20231106151252802](image/112.png)

注销驱动的时候也需要注销掉前面注册的 uart_driver，需要用到 uart_unregister_driver 函数，函数原型如下：

![image-20231106151653004](/media/pc/Program/Program Files(x86)/Linux/Linux_driver/image/113.png)

2、uart_port 的添加与移除

uart_port 表示一个具体的 port，uart_port 定义在 include/linux/serial_core.h 文件：

```c
struct uart_port {
	spinlock_t lock; /* port lock */
	unsigned long iobase; /* in/out[bwl] */
	unsigned char __iomem *membase; /* read/write[bwl] */
	const struct uart_ops *ops; 236 unsigned int custom_divisor; 237 unsigned int line; /* port index */
	unsigned int minor; 239 resource_size_t mapbase; /* for ioremap */
	resource_size_t mapsize; 241 struct device *dev; /* parent device */
	....
	....
};
```

uart_port中最主要的就算ops，其中包含了串口的具体驱动函数每个UART都有一个uart_port，将uart_port和uart_driver结合起来会用到uart_add_one_port 函数，原型如下：
![image-20231106152826513](/home/pc/.config/Typora/typora-user-images/image-20231106152826513.png)

卸载 UART 驱动的时候也需要将 uart_port 从相应的 uart_driver 中移除，需要用到uart_remove_one_port 函数，函数原型如下：
![image-20231106164947025](/media/pc/Program/Program Files(x86)/Linux/Linux_driver/image/114.png)

3、uart_ops实现

uart_port 中的 ops 成员变量很重要，因为 ops 包含了针对 UART 具体的驱动函数，Linux 系统收发数据最终调用的都是 ops 中的函数。ops 是 uart_ops类型的结构体指针变量，uart_ops 定义在 include/linux/serial_core.h 文件中，内容如下：

```c
struct uart_ops {
	unsigned int (*tx_empty)(struct uart_port *);
	void (*set_mctrl)(struct uart_port *, unsigned int mctrl);
	unsigned int (*get_mctrl)(struct uart_port *);
	void (*stop_tx)(struct uart_port *);
	void (*start_tx)(struct uart_port *);
	void (*throttle)(struct uart_port *);
	void (*unthrottle)(struct uart_port *);
	void (*send_xchar)(struct uart_port *, char ch);
	void (*stop_rx)(struct uart_port *);
	void (*enable_ms)(struct uart_port *);
	void (*break_ctl)(struct uart_port *, int ctl);
	int (*startup)(struct uart_port *);
	void (*shutdown)(struct uart_port *);
	void (*flush_buffer)(struct uart_port *);
	void (*set_termios)(struct uart_port *, struct ktermios *new,
	struct ktermios *old);
	void (*set_ldisc)(struct uart_port *, struct ktermios *);
	void (*pm)(struct uart_port *, unsigned int state,
	unsigned int oldstate);
	
	/*
	* Return a string describing the type of the port
	*/
	const char *(*type)(struct uart_port *);
	
	/*
	* Release IO and memory resources used by the port.
	* This includes iounmap if necessary.
	*/
	void (*release_port)(struct uart_port *);
	
	/*
	* Request IO and memory resources used by the port.
	* This includes iomapping the port if necessary.
	*/
	int (*request_port)(struct uart_port *);
	void (*config_port)(struct uart_port *, int);
	int (*verify_port)(struct uart_port *, struct serial_struct *);
	int (*ioctl)(struct uart_port *, unsigned int, unsigned long);
	#ifdef CONFIG_CONSOLE_POLL
	int (*poll_init)(struct uart_port *);
	void (*poll_put_char)(struct uart_port *, unsigned char);
	int (*poll_get_char)(struct uart_port *);
	#endif
};

```

UART 驱动编写人员需要实现 uart_ops，因为 uart_ops 是最底层的 UART 驱动接口，是实 实在在的和 UART 寄存器打交道的。关于 uart_ops 结构体中的这些函数的具体含义请参考 Documentation/serial/driver 这个文档。
看一下 NXP 官方的 UART 驱动

#### IMX6ULL UART驱动分析

见开发指南P1516