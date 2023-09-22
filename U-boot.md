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