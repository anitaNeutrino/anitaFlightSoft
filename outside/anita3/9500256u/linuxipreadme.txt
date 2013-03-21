
Acromag, Inc.
30765 S. Wixom Rd., P.O. Box 437
Wixom, Michigan  48393-7037  U.S.A.
E-mail: support@acromag.com
Telephone (248) 295-0310
FAX (248) 624-9234

Red Hat Fedora 18
Kernel 3.6.10-4

Acromag: 9500256, REV U

A note about out-of-tree loadable kernel modules

Acromag developed a set of strategies, code, and testing methods to help us
reuse code, maintain quality, and maximize our testing resources in order
to get the best quality product to our customers in the shortest amount of time.
To accomplish this, out-of-tree loadable kernel modules have been
developed over the past ten years. Recently, out-of-tree loadable kernel
modules have become an unpopular topic with core Linux kernel developers.
Our Open Source out-of-tree drivers generally work with all kernel releases
back to 2.4 and many users still want a driver that will support the newest
hardware on older kernels making out-of-tree drivers a necessary business
solution for hardware vendors.
With kernel releases 3.4 and newer, loading any out-of-tree driver module
will taint the kernel with a debug message similar to this:
"Disabling lock debugging due to kernel taint".  It is not a kernel bug,
but expected behavior. The taint flags (multiple flags may be pending)
may be examined using the following:

cat/proc/sys/kernel/tainted
4096 = An out-of-tree module has been loaded.

Possibly, if the out-of-tree driver was merged into the kernel (i.e. add
the sources and update the kernel Makefiles) and build it as part of the
kernel build, the taint and taint message would likely go away.

This diskette contains library routines for Acromag I/O Boards.
Following is a table that indicates which Acromag I/O Boards are
supported and in what subdirectory the support files may be
found:

Subdirectory | Boards Supported
-------------+---------------------------------------------
IP220        |  IP220-8, IP220-16 .
IP230        |  IP230-4, IP230-8, IP235-4, IP235-8.
IP231        |  IP231-8, IP231-16.
IP236        |  IP236-4,IP236-8.
IP320        |  IP320.
IP330        |  IP330.
IP340        |  IP340,  IP341.
IP400        |  IP400.
IP405        |  IP405.
IP408        |  IP408.
IP409        |  IP409.
IP445        |  IP445.
IP470        |  IP440-1, IP440-2, IP440-3, IP470.
IP480        |  IP480.
IP482        |  IP482, IP483, IP484.
IP560        |  IP560, IP560-I.
IP570        |  IP571, IP572 IP-IOS570-EDK 9500-430 Required.
IP1K100      |  IP1K100.
IP1K110      |  IP1K110, IP1K125, IPEP20x.

Also included in each subdirectory is an "information" file
which contains a list and detailed explanation of all of
the program files which make up each library.  For example,
the information file for the IP440 and IP470 families
of boards is named "info470.txt".
