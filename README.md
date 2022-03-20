
# MOST的用户态TCP协议栈(DPDK)解法

用户态协议栈相比内核协议栈大约能快几十微秒。

## 内核协议栈版本

```
mkdir build
cd build
cmake ..
make
./main
```

## 用户态协议栈(DPDK)版本

需要一块支持DPDK的网卡，我使用的是Intel E1G42ET双网口千兆网卡，使用82576芯片，淘宝价格135元。

首先需要编译[mTCP](https://github.com/mtcp-stack/mtcp)，按照其中的指令完成：

1. 编译并安装dpdk
2. 安装驱动并且将网卡绑定到驱动上
3. 设置hugepages
4. 安装mTCP的内核模块，安装之后使用ifconfig能够看到一个名称为dpdk0的网卡
5. 使用sudo ifconfig dpdk0 192.168.1.108 netmask 255.255.255.0 up设置网卡的IP
6. 设置RTE_SDK和RTE_TARGET环境变量
7. 编译mTCP

坑：
1. mTCP可以将DPDK的网卡伪装成一块正常的网卡，这样就能够使用ifconfig进行IP配置。但是Ubuntu的图形化程序有时候会自动配置网卡，导致DPDK网卡的IP变成不是我们想要的。最好关闭网卡自动配置。

将mTCP目录和本目录放置在同一级父目录下，比如：

```
.
├── MOST
└── mtcp
```

然后在`MOST`目录下编译并运行程序：

```
mkdir build
make
sudo ./build/main_mtcp
```
