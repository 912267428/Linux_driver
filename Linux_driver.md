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

   of_iomap 函数用于直接内存映射，以前我们会通过 ioremap 函数来完成物理地址到虚拟地 址的映射，采用设备树以后就可以直接通过 of_iomap 函数来获取内存地址所对应的虚拟地址， 不需要使用 ioremap 函数了。当然了，你也可以使用 ioremap 函数来完成物理地址到虚拟地址 的内存映射，只是在采用设备树以后，大部分的驱动都使用 of_iomap 函数了。of_iomap 函数本 质上也是将 reg 属性中地址信息转换为虚拟地址，如果 reg 属性有多段的话，可以通过 index 参 数指定要完成内存映射的是哪一段，of_iomap 函数原型如下：

   ```c
   void __iomem *of_iomap(struct device_node *np, 
   						int index)
   ```

   np：设备节点。 
   index：reg 属性中要完成内存映射的段，如果 reg 属性只有一段的话 index 就设置为 0。
   返回值：经过内存映射后的虚拟内存首地址，如果为 NULL 的话表示内存映射失败。 关于设备树常用的 OF 函数就先讲解到这里，Linux 内核中关于设备树的 OF 函数不仅仅只 有前面讲的这几个，还有很多 OF 函数我们并没有讲解，这些没有讲解的 OF 函数要结合具体 的驱动，比如获取中断号的 OF 函数、获取 GPIO 的 OF 函数等等，这些 OF 函数我们在后面的 驱动实验中再详细的讲解。

### 设备树下的 LED 驱动实验

