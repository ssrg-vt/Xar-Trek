# Xar-Trek

Xar-Trek is a compiler and run-time framework which enables execution migration of application functions from heterogeneous-ISA server host CPUs to FPGAs at run-time.  

The framework includes:  
1)Xar-Trek's compiler to generate multi-ISA binaries and FPGA implementations for a select set of application functions with very little programmer involvement;  
2)Xar-Trek's run-time to provide a scheduler infrastructure that monitors server workloads and migrates functions across heterogeneous-ISA CPUs and the FPGA.   

The installation of Xar-Trek assumes that two servers will be used to run the applications. The first one, an x86 server, must have installed the Popcorn Kernel, the Popcorn Compiler, Xilinx Vitis, Xilinx Run Time (XRT), and Xilinx Alveo U50 card deployment platform. The second one, an ARM server, must have installed the Popcorn Kernel.  

The following steps will guide you through thhe installation process. After installing all the components of Xar-Trek, the folder "examples" contains detailed instructions to use Xar-Trek with a set of applications.  

=
x86 Server Instalation 
-- 

These are the components of Xar-Trek that must be installed on the x86 server:  
-- Xilinx Vitis 2020.2  
-- Xilinx Runtime (XRT) version 2.6.655  
-- Xilinx U50 deployment platform "xilinx_u50_gen3x16_xdma_201920_3"  
-- Popcorn Kernel 4.4.137 (shared libraries 4.4 branch)  
-- Popcorn Compiler (Xar-Trek branch) 


The folowing steps will guide you through the installation processs.  

1) Install Xilinx Vitis 2020.2  

https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2020-2.html




2) Install Packages for Alveo U50 board

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


3) Install Popcorn Kernel on x86 (MIR4 machine at AISB building):   
(Internal repo: https://popcorn.rasec.tech/popcorn-kernel.git "rasec/shared-libraries-4.4")  
Public repo:  
~$ git clone https://github.com/ssrg-vt/Xar-Trek.git (main branch)  
~$ mkdir rasec  
~$ mv Xar-Trek/ ~/rasec/popcorn-kernel  
~$ cd ~/rasec/popcorn-kernel/  
Use our machine's configuration.  
mir4：～$ cp ~/rasec/popcorn-kernel/kernel/popcorn/configs/config-x86_64-mir .config **(Only do this on mir4)**  
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
  
4) Install Popcorn compiler **(Only on x86)**  

Install needed apps:  
~$ sudo apt-get install build-essential flex bison subversion cmake zip linux-compiler-gcc-6-x86 gawk

Public repo:  
~$ git clone https://github.com/ssrg-vt/popcorn-compiler.git  
branch: xar-trek

~$ cd popcorn-compiler  
~$ git checkout origin/xar-trek -b xar-trek  

Install compiler at POPCORN PATH:  
~$ ./install_compiler.py --install-path \<POPCORN PATH\> --install-all --threads 8  



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
Xar-Trek Scheduler
--

3) Generate the libmigrate.a (with the scheduler) and copy it to the popcorn compiler. **(Only on x86)**

cd ~/popcorn-compiler/lib/migration  

vi /src/migrate_sched.c  

#define POPCORN_X86 "10.1.10.14" /* TODO - change it according to your setup */  
#define POPCORN_ARM "10.1.1.51" /* TODO - change it according to your setup */  

sudo make POPCORN=~/pop_tool

cp build/aarch64/libmigrate.a ~/pop_tool/aarch64/lib/  
cp build/x86_64/libmigrate.a ~/pop_tool/x86_64/lib/  
 



6) Compile the applications:  
Copy the file "libARMOpenCL.a"  to the same folder as Makefile.  
make 
I'm also attaching one Makefile as a reference.  

7) Compile the scheduler server  
cd ~/Pop_Scheduler/popcorn-scheduler/  
vi popcorn_sched_server.c  
#define POPCORN_X86 "10.1.10.14" /* TODO - change it according to your setup */  
#define POPCORN_ARM "10.1.1.51" /* TODO - change it according to your setup */  
gcc -O3 -Wall -I./ ./popcorn_sched_server.c ./mytimer.c -lpthread -o pop_server  

8) Copy the binaries to the same folders on x86 and ARM  


====
Popcorn Linux for Distributed Thread Execution
----------------------------------------------

* Allow applications to run on multiple machines in a distributed and transparent way.

* If you want to try popcorn-kernel, please go to the [wiki page](https://github.com/ssrg-vt/popcorn-kernel/wiki) first. That wiki page should contain all the useful precedures for setting up popcorn with VMs.

* Popcorn Linux kernel is currently under development, you might get a very large git repo if you try to clone it. Instead, we provide a [release version](https://github.com/ssrg-vt/popcorn-kernel/releases/tag/linux-4.4.137) and strongly suggest you download the source code from the [release](https://github.com/ssrg-vt/popcorn-kernel/releases/tag/linux-4.4.137).

* Please visit http://popcornlinux.org for more information or e-mail Xiaoguang Wang (xiaoguang@vt.edu) or Sang-Hoon Kim (sanghoon@vt.edu).

* Copyright Systems Software Research Group at Virginia Tech, 2017-2019.
