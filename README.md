# Xar-Trek

Xar-Trek is a compiler and run-time framework which enables execution migration of application functions from heterogeneous-ISA server host CPUs to FPGAs at run-time.  

The framework includes:  
1)Xar-Trek's compiler to generate multi-ISA binaries and FPGA implementations for a select set of application functions with very little programmer involvement;  
2)Xar-Trek's run-time to provide a scheduler infrastructure that monitors server workloads and migrates functions across heterogeneous-ISA CPUs and the FPGA.   

The installation of Xar-Trek assumes that two servers will be used to run the applications. The first one, an x86 server, must have installed the Popcorn Kernel, the Popcorn Compiler, Xilinx Vitis, Xilinx Run Time (XRT), and Xilinx Alveo U50 card deployment platform. The second one, an ARM server, must have installed the Popcorn Kernel.  

The following steps will guide you through the installation process. After installing all the components of Xar-Trek, the folder "examples" contains detailed instructions to use Xar-Trek with a set of applications.  



=
x86 Server Instalation 
-- 

These are the components of Xar-Trek that must be installed on the x86 server:  
-- Popcorn Kernel 4.4.137 (shared libraries 4.4 branch)  
-- Popcorn Compiler (Xar-Trek branch)  
-- Xilinx Vitis 2020.2  
-- Xilinx Runtime (XRT) version 2.6.655  
-- Xilinx U50 deployment platform "xilinx_u50_gen3x16_xdma_201920_3"  
 

The folowing steps will guide you through the installation processs.  

1) Install Popcorn Kernel on x86 (mir4 machine at AISB building):   
(Internal repo: https://popcorn.rasec.tech/popcorn-kernel.git "rasec/shared-libraries-4.4")  
Public repo:  
~$ git clone https://github.com/ssrg-vt/Xar-Trek.git (main branch)  
~$ mkdir rasec  
~$ mv Xar-Trek/ ~/rasec/popcorn-kernel  
~$ cd ~/rasec/popcorn-kernel/  
Use our machine's configuration.  
mir4：～$ cp ~/rasec/popcorn-kernel/kernel/popcorn/configs/config-x86_64-mir-ubuntu18 .config **(Only do this on mir4)**  
~$ make menuconfig (change kernel name to xar-trek)  
General setup  --->  
(-xar-trek) Local version - append to kernel release  
save and exit  
~$ make -j96  
~$ make modules -j96  
~$ sudo make modules_install  
~$ sudo make install  
...  
Found linux image: /boot/vmlinuz-4.15.0-159-generic >> #0  
Found initrd image: /boot/initrd.img-4.15.0-159-generic  
Found linux image: /boot/vmlinuz-4.4.137-xar-trek+ >> #1  
Found initrd image: /boot/initrd.img-4.4.137-xar-trek+  
Found linux image: /boot/vmlinuz-4.4.137-xar-trek+.old >> #2  
Found initrd image: /boot/initrd.img-4.4.137-xar-trek+  
...  
  
Set grub default number to the number of kernel-xar-trek  
~$ sudo grub-set-default ($number)  
In our X86 case, the ($number) should be 1.  
Reboot.  
$ uname -a (check if you are on -xar-trek kernel)  
Linux mir4 4.4.137-xar-trek+ #1...  
  
2) Install Popcorn compiler **(Only on x86)**  

Install needed apps:  
~$ sudo apt install build-essential flex bison subversion cmake zip gawk  
~$ sudo apt install texinfo  
~$ sudo apt install gcc-aarch64-linux-gnu  
~$ sudo apt install linux-compiler-gcc-6-x86  #(debian9)  
~$ sudo apt install g++-6-x86-64-linux-gnux32  #(ubuntu18)  

Public repo:  
~$ git clone https://github.com/ssrg-vt/popcorn-compiler.git  
branch: xar-trek

~$ cd popcorn-compiler  
~$ git checkout origin/xar-trek -b xar-trek  

Install compiler at POPCORN PATH:  
~$ ./install_compiler.py --install-path \<POPCORN PATH\> --install-all --threads 32  



3) Install Xilinx Vitis 2020.2  

https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2020-2.html




4) Install Packages for Alveo U50 board

The packages are available at:
https://www.xilinx.com/products/boards-and-kits/alveo/u50.html#gettingStarted

Choose 2020.1 --> XDMA --> x86_64 --> Ubuntu --> 18.04

Download and install Xilinx Runtime (XRT) version 2.6.655  
xrt_202010.2.6.655_18.04-amd64-xrt.deb  
sudo apt install ./xrt_202010.2.6.655_18.04-amd64-xrt.deb

Download Deployment Target Platform to an empty folder:  
Xilinx_u50-gen3x16-xdma-201920.3-2784799_18.04_deb.tar.gz  
Uncompress and install each package:  
tar xvzf Xilinx_u50-gen3x16-xdma-201920.3-2784799_18.04_deb.tar.gz  
sudo apt install ./xilinx-cmc-u50-1.0.17-2784148_18.04.deb  
sudo apt install ./xilinx-sc-fw-u50-5.0.27-2.e289be9_18.04.deb  
sudo apt install ./xilinx-u50-gen3x16-xdma-201920.3-2784799_18.04.deb

Download and install Development Target Platform  
xilinx-u50-gen3x16-xdma-dev-201920.3-2784799_18.04.deb  
sudo apt install ./xilinx-u50-gen3x16-xdma-dev-201920.3-2784799_18.04.deb  


+-For more detailed instruction, see User Guide 1370:
https://www.xilinx.com/support/documentation/boards_and_kits/accelerator-cards/1_7/ug1370-u50-installation.pdf


=
ARM Server Instalation
--

The ARM server must have only the Popcorn Kernel 4.4.137 (shared libraries 4.4 branch) installed in order to use Xar-Trek.

The steps are similar to the ones used on the x86 server:

Install Popcorn Kernel (LEG machine at AISB building):   
(Internal repo: https://popcorn.rasec.tech/popcorn-kernel.git "rasec/shared-libraries-4.4")  
Public repo:  
~$ git clone https://github.com/ssrg-vt/Xar-Trek.git (main branch)  
~$ mkdir rasec  
~$ mv Xar-Trek/ ~/rasec/popcorn-kernel  
~$ cd ~/rasec/popcorn-kernel/  
Use our machine's configuration.  
leg：～$ cp ~/rasec/popcorn-kernel/kernel/popcorn/configs/config-arm64-cavium .config **(Only do this on leg)**  
~$ make menuconfig (change kernel name to xar-trek)  
General setup  --->  
(-xar-trek) Local version - append to kernel release  
save and exit  
~$ make -j96  
~$ make modules -j96  
~$ sudo make modules_install  
~$ sudo make install  
...  
Found linux image: /boot/vmlinuz-4.19.60-popcorn+ >> #0  
Found initrd image: /boot/initrd.img-4.19.60-popcorn+  
Found linux image: /boot/vmlinuz-4.9.0-11-amd64 >> #1  
Found initrd image: /boot/initrd.img-4.9.0-11-amd64  
Found linux image: /boot/vmlinuz-4.4.137-xar-trek+ >> #2  
Found initrd image: /boot/initrd.img-4.4.137-xar-trek+  
...  
  
Set grub default number to the number of kernel-xar-trek  
~$ sudo grub-set-default ($number)  
In our X86 case, the ($number) should be 2.  
Reboot.  
$ uname -a (check if you are on -xar-trek kernel)  
Linux mir4 4.4.137-xar-trek+ #1...  
  



=
Popcorn Messaging Layer
--

Popcorn uses a messaging module to communicate between the servers. 
Both message layer modules contain the servers IPs hardcoded in the source code.
It is necessary to compile the message layer modules for x86 and ARM and load them on each server.

1) Compile the message layer module **(repeat for each machine, x86 and ARM)**.

cd ~/rasec/popcorn-kernel/  
vi msg_layer/config.h  

/* Node 0 */ "10.1.10.14",  
/* Node 1 */ "10.1.1.51",  

make -C  msg_layer  

2) Load the modules **(repeat for each machine, x86 and ARM)**.  
sudo insmod msg_layer/msg_socket.ko  

Verify if the machines are connected:  
x86  
dmesg|grep popcorn  
 popcorn: Loading node configuration...  
 popcorn: * 0: 10.1.10.14  
 popcorn:   1: 10.1.1.51  
 popcorn:    1 joined, aarch64  
 popcorn: Ready on TCP/IP  

ARM  
dmesg|grep popcorn  
 popcorn: Loading node configuration...  
 popcorn:   0: 10.1.10.14  
 popcorn: * 1: 10.1.1.51  
 popcorn: Ready on TCP/IP  
 popcorn:    0 joined, x86_64  


=
Xar-Trek Scheduler Client
--

The scheduler client is integrated to the application, at compilation time.  
The IP addresses of each machine must be updated in the Popcorn migration library before compiling the applications. 

This steps must be applied to te x86 server only.  

1) Generate the libmigrate.a (including the scheduler)

cd ~/popcorn-compiler/lib/migration  

vi src/migrate_sched.c  

#define POPCORN_X86 "10.1.10.14" /* change it according to your setup */  
#define POPCORN_ARM "10.1.1.51" /*  change it according to your setup */  

sudo make POPCORN=~/pop_tool

2) Copy the updated library to the popcorn compiler.  

cp build/aarch64/libmigrate.a ~/pop_tool/aarch64/lib/  
cp build/x86_64/libmigrate.a ~/pop_tool/x86_64/lib/  
 
=
Xar-Trek Scheduler Server
--

The scheduler server must ne running prior to lunch any application.  

It must also contain the IP addresses of each machine.

1) Update the IP addresses  

cd ~/Pop_Scheduler/popcorn-scheduler/  
vi popcorn_sched_server.c  
#define POPCORN_X86 "10.1.10.14" /* TODO - change it according to your setup */  
#define POPCORN_ARM "10.1.1.51" /* TODO - change it according to your setup */  


2) Compile the scheduler server  
gcc -O3 -Wall -I./ ./xar-trek_sch_server.c ./mytimer.c -lpthread -o xar-trek-sch_server

3) Start the scheduler server  

It shows at the beginning the available hardware kernels on the XCLBIN file:  

******************** Xar-Trek Scheduler Server ****************  
Available Hardware Kernels:  
KNL_HW_DigitRec_sw  
KNL_HW_Digit_500_Rec_sw  
KNL_HW_face_640_detect_sw  
KNL_HW_face_detect_sw  
KNL_Mod_CG_A  

Xar-Trek Scheduler Server: waiting for connections...  


=
Remote hardware set up (upon request)
--

A complete hardware set up can be accessed remotely, and consists of:  
a) Dell 7920 server (Xeon Bronze 3104 CPU, 1.7GHz, 6 cores, 64GB)  
b) Cavium ThunderX server (ARM CPU, 2GHz, 96 cores, 128GB)  
c) Xilinx Alveo U50 card  

The remote access is available upon request.  
Please, send an e-mail to edsonh@vt.edu explaining why you need to use our machines and we'll do our best to grant you the access.


***********************************************************

* Copyright Systems Software Research Group at Virginia Tech, 2021.
