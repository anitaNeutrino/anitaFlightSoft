#!/bin/sh -
#-----------------------------------------------------------------------------
#
#      File         :  mountAnitaDisks.sh
#      Abstract     :  Mounts the disks needed for ANITA to take data
#      Last Revision:  29/06/14
#-----------------------------------------------------------------------------


# For now this justs mounts the external hard drive.
mount LABEL=rjndata /mnt/data
mount --bind /mnt/data/helium1 /mnt/helium1


