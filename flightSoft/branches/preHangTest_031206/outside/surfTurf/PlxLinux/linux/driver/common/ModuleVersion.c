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
 *      ModuleVersion.c
 *
 * Description:
 *
 *      The source file to contain module version information
 *
 * Revision History:
 *
 *      05-01-03 : PCI SDK v4.10
 *
 ******************************************************************************/


/*********************************************************
 * __NO_VERSION__ should be defined for all source files
 * that include <module.h>.  In order to get kernel
 * version information into the driver, __NO_VERSION__
 * must be undefined before <module.h> is included in
 * one and only one file.  Otherwise, the linker will
 * complain about multiple definitions of module version
 * information.
 ********************************************************/
#if defined(__NO_VERSION__)
    #undef __NO_VERSION__
#endif

#include <linux/module.h>




/***********************************************
 *            Module Information
 **********************************************/
EXPORT_NO_SYMBOLS;

MODULE_DESCRIPTION("PLX Linux Device Driver");

/*********************************************************
 * In later releases of Linux kernel 2.4, the concept of
 * module licensing was introduced.  This is used when
 * the module is loaded to determine if the kernel is
 * tainted due to loading of the module.  Each module
 * declares its license MODULE_LICENSE().  Refer to
 * http://www.tux.org/lkml/#export-tainted for more info.
 *
 * From "module.h", the possible values for license are:
 *
 *   "GPL"                        [GNU Public License v2 or later]
 *   "GPL and additional rights"  [GNU Public License v2 rights and more]
 *   "Dual BSD/GPL"               [GNU Public License v2 or BSD license choice]
 *   "Dual MPL/GPL"               [GNU Public License v2 or Mozilla license choice]
 *   "Proprietary"                [Non free products]
 *
 * Since PLX drivers are provided only to customers who
 * have purchased the PLX SDK, PLX modules are marked
 * as "Proprietary".
 ********************************************************/

#if defined(MODULE_LICENSE)
    MODULE_LICENSE("Proprietary");
#endif
