#!/bin/sh -
#-----------------------------------------------------------------------------
#
#      File         :  mountAnitaDisks.sh
#      Abstract     :  Mounts the disks needed for ANITA to take data
#      Last Revision:  29/06/14
#-----------------------------------------------------------------------------


# For now this justs mounts the external hard drive.
mount LABEL=HELIUM1 /mnt/helium1
mount --bind /mnt/helium1/data /mnt/data

# For now this justs mounts the other external hard drive.
mount LABEL=HELIUM2 /mnt/helium2

