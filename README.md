# Xar-Trek
Xar-Trek Middleware'21

1) Install Popcorn Kernel on x86 (mir4) and ARM (leg):   
(Internal repo: https://popcorn.rasec.tech/popcorn-kernel.git "rasec/shared-libraries-4.4")  
Public repo:  
~$ git clone https://github.com/ssrg-vt/Xar-Trek.git (main branch)  
~$ mkdir rasec  
~$ mv Xar-Trek/ ~/rasec/popcorn-kernel  
~$ cd ~/rasec/popcorn-kernel/  
Use our machine's configuration.  
mir4：～$ cp ~/rasec/popcorn-kernel/kernel/popcorn/configs/config-x86_64-mir .config **(Only do this on mir4) **  
leg：～$ cp /rasec/popcorn-kernel/kernel/popcorn/configs/config-arm64-cavium .config **(Only do this on leg)**  
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
  
2) Install Popcorn compiler. 

Install needed apps:

~$ sudo apt-get install build-essential flex bison subversion cmake zip linux-compiler-gcc-6-x86 gawk

Public repo:  
~$ git clone https://github.com/ssrg-vt/popcorn-compiler.git  
branch: xar-trek

~$ cd popcorn-compiler

~$ git checkout origin/xar-trek -b xar-trek


Install compiler at POPCORN PATH:

~$ ./install_compiler.py --install-path \<POPCORN PATH\> --install-all --threads 8


3) Generate the libmigrate.a (with the scheduler) and copy it to the popcorn compiler.

cd ${HOME}/popcorn-compiler/lib/migration  

vi /src/migrate_sched.c  

#define POPCORN_X86 "192.168.0.72" /* TODO - change it according to your setup */  
#define POPCORN_ARM "192.168.0.185" /* TODO - change it according to your setup */  

sudo make POPCORN=${HOME}/pop_tool


TO DO:
cp /home/edshor/rasec/popcorn/popcorn-compiler/lib/migration/build/aarch64/libmigrate.a /home/edshor/pop_tool/aarch64/lib  
cp /home/edshor/rasec/popcorn/popcorn-compiler/lib/migration/build/x86_64/libmigrate.a /home/edshor/pop_tool/x86_64/lib  

I have separated the scheduler from the file "migrate.c". I'm attaching the last version of it to this e-mail.  

4) Compile the Kernel Object and load it, for x86 and ARM:  
cd ~/rasec/linux-{x86,arm}  
vi msg_layer/config.h  

/* Node 0 */ "192.168.0.72",  
/* Node 1 */ "192.168.0.185",  

make -C  msg_layer  
sudo insmod msg_layer/msg_socket.ko  

5) Compile the applications:  
Copy the file "libARMOpenCL.a"  to the same folder as Makefile.  
make 
I'm also attaching one Makefile as a reference.  

6) Compile the scheduler server  
cd ~/Pop_Scheduler/popcorn-scheduler/  
vi popcorn_sched_server.c  
#define POPCORN_X86 "192.168.0.72" /* TODO - change it according to your setup */  
#define POPCORN_ARM "192.168.0.185" /* TODO - change it according to your setup */  
gcc -O3 -Wall -I./ ./popcorn_sched_server.c ./mytimer.c -lpthread -o pop_server  

7) Copy the binaries to the same folders on x86 and ARM  


====
Popcorn Linux for Distributed Thread Execution
----------------------------------------------

* Allow applications to run on multiple machines in a distributed and transparent way.

* If you want to try popcorn-kernel, please go to the [wiki page](https://github.com/ssrg-vt/popcorn-kernel/wiki) first. That wiki page should contain all the useful precedures for setting up popcorn with VMs.

* Popcorn Linux kernel is currently under development, you might get a very large git repo if you try to clone it. Instead, we provide a [release version](https://github.com/ssrg-vt/popcorn-kernel/releases/tag/linux-4.4.137) and strongly suggest you download the source code from the [release](https://github.com/ssrg-vt/popcorn-kernel/releases/tag/linux-4.4.137).

* Please visit http://popcornlinux.org for more information or e-mail Xiaoguang Wang (xiaoguang@vt.edu) or Sang-Hoon Kim (sanghoon@vt.edu).

* Copyright Systems Software Research Group at Virginia Tech, 2017-2019.
