#ifndef __DISPATCH_H
#define __DISPATCH_H

/*******************************************************************************
 * Copyright (c) 2002 PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this software under specific terms and
 * conditions.  Use of any of the software or derviatives thereof in any
 * product without a PLX Technology chip is strictly prohibited.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL PLX'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO PLX FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      Dispatch.h
 *
 * Description:
 *
 *      The Driver Dispatch functions
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include <linux/fs.h>




/**********************************************
*               Functions
**********************************************/
int
Dispatch_open(
    struct inode *inode,
    struct file  *filp
    );

int
Dispatch_release(
    struct inode *inode,
    struct file  *filp
    );

ssize_t
Dispatch_read(
    struct file *filp,
    char        *buff,
    size_t       count,
    loff_t      *offp
    );

ssize_t
Dispatch_write(
    struct file *filp,
    const char  *buff,
    size_t       count,
    loff_t      *offp
    );

int
Dispatch_mmap(
    struct file           *filp,
    struct vm_area_struct *vma
    );

int 
Dispatch_IoControl(
    struct inode  *inode,
    struct file   *filp,
    unsigned int   cmd,
    unsigned long  args
    );




#endif
