/*******************************************************************************
 * Copyright (c) 2003 PLX Technology, Inc.
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
 *      PlxInit.c
 *
 * Description:
 *
 *      This file provides some common functionality, which is used by the
 *      various sample applications.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "ConsFunc.h"
#include "PlxApi.h"
#include "PlxInit.h"




/**********************************************
*               Globals
**********************************************/
API_ERRORS ApiErrors[] =
{
    { ApiSuccess,                   "ApiSuccess"                   },
    { ApiFailed,                    "ApiFailed"                    },
    { ApiAccessDenied,              "ApiAccessDenied"              },
    { ApiDmaChannelUnavailable,     "ApiDmaChannelUnavailable"     },
    { ApiDmaChannelInvalid,         "ApiDmaChannelInvalid"         },
    { ApiDmaChannelTypeError,       "ApiDmaChannelTypeError"       },
    { ApiDmaInProgress,             "ApiDmaInProgress"             },
    { ApiDmaDone,                   "ApiDmaDone"                   },
    { ApiDmaPaused,                 "ApiDmaPaused"                 },
    { ApiDmaNotPaused,              "ApiDmaNotPaused"              },
    { ApiDmaCommandInvalid,         "ApiDmaCommandInvalid"         },
    { ApiDmaManReady,               "ApiDmaManReady"               },
    { ApiDmaManNotReady,            "ApiDmaManNotReady"            },
    { ApiDmaInvalidChannelPriority, "ApiDmaInvalidChannelPriority" },
    { ApiDmaManCorrupted,           "ApiDmaManCorrupted"           },
    { ApiDmaInvalidElementIndex,    "ApiDmaInvalidElementIndex"    },
    { ApiDmaNoMoreElements,         "ApiDmaNoMoreElements"         },
    { ApiDmaSglInvalid,             "ApiDmaSglInvalid"             },
    { ApiDmaSglQueueFull,           "ApiDmaSglQueueFull"           },
    { ApiNullParam,                 "ApiNullParam"                 },
    { ApiInvalidBusIndex,           "ApiInvalidBusIndex"           },
    { ApiUnsupportedFunction,       "ApiUnsupportedFunction"       },
    { ApiInvalidPciSpace,           "ApiInvalidPciSpace"           },
    { ApiInvalidIopSpace,           "ApiInvalidIopSpace"           },
    { ApiInvalidSize,               "ApiInvalidSize"               },
    { ApiInvalidAddress,            "ApiInvalidAddress"            },
    { ApiInvalidAccessType,         "ApiInvalidAccessType"         },
    { ApiInvalidIndex,              "ApiInvalidIndex"              },
    { ApiMuNotReady,                "ApiMuNotReady"                },
    { ApiMuFifoEmpty,               "ApiMuFifoEmpty"               },
    { ApiMuFifoFull,                "ApiMuFifoFull"                },
    { ApiInvalidRegister,           "ApiInvalidRegister"           },
    { ApiDoorbellClearFailed,       "ApiDoorbellClearFailed"       },
    { ApiInvalidUserPin,            "ApiInvalidUserPin"            },
    { ApiInvalidUserState,          "ApiInvalidUserState"          },
    { ApiEepromNotPresent,          "ApiEepromNotPresent"          },
    { ApiEepromTypeNotSupported,    "ApiEepromTypeNotSupported"    },
    { ApiEepromBlank,               "ApiEepromBlank"               },
    { ApiConfigAccessFailed,        "ApiConfigAccessFailed"        },
    { ApiInvalidDeviceInfo,         "ApiInvalidDeviceInfo"         },
    { ApiNoActiveDriver,            "ApiNoActiveDriver"            },
    { ApiInsufficientResources,     "ApiInsufficientResources"     },
    { ApiObjectAlreadyAllocated,    "ApiObjectAlreadyAllocated"    },
    { ApiAlreadyInitialized,        "ApiAlreadyInitialized"        },
    { ApiNotInitialized,            "ApiNotInitialized"            },
    { ApiBadConfigRegEndianMode,    "ApiBadConfigRegEndianMode"    },
    { ApiInvalidPowerState,         "ApiInvalidPowerState"         },
    { ApiPowerDown,                 "ApiPowerDown"                 },
    { ApiFlybyNotSupported,         "ApiFlybyNotSupported"         },
    { ApiNotSupportThisChannel,     "ApiNotSupportThisChannel"     },
    { ApiNoAction,                  "ApiNoAction"                  },
    { ApiHSNotSupported,            "ApiHSNotSupported"            },
    { ApiVPDNotSupported,           "ApiVPDNotSupported"           },
    { ApiVpdNotEnabled,             "ApiVpdNotEnabled"             },
    { ApiNoMoreCap,                 "ApiNoMoreCap"                 },
    { ApiInvalidOffset,             "ApiInvalidOffset"             },
    { ApiBadPinDirection,           "ApiBadPinDirection"           },
    { ApiPciTimeout,                "ApiPciTimeout"                },
    { ApiDmaChannelClosed,          "ApiDmaChannelClosed"          },
    { ApiDmaChannelError,           "ApiDmaChannelError"           },
    { ApiInvalidHandle,             "ApiInvalidHandle"             },
    { ApiBufferNotReady,            "ApiBufferNotReady"            },
    { ApiInvalidData,               "ApiInvalidData"               },
    { ApiDoNothing,                 "ApiDoNothing"                 },
    { ApiDmaSglBuildFailed,         "ApiDmaSglBuildFailed"         },
    { ApiPMNotSupported,            "ApiPMNotSupported"            },
    { ApiInvalidDriverVersion,      "ApiInvalidDriverVersion"      },
    { ApiWaitTimeout,               "ApiWaitTimeout"               },
    { ApiWaitCanceled,              "ApiWaitCanceled"              },
    { ApiLastError,                 "Unknown"                      }
};




/*********************************************************************
 *
 * Function   : SelectDevice
 *
 * Description: Asks the user which PLX PCI device to select
 *
 * Returns    : Total Plx devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S8
SelectDevice(
    DEVICE_LOCATION *pDevice
    )
{
    U32  i;
    U32  DeviceNum;


    pDevice->BusNumber       = (U8)-1;
    pDevice->SlotNumber      = (U8)-1;
    pDevice->VendorId        = (U16)-1;
    pDevice->DeviceId        = (U16)-1;
    pDevice->SerialNumber[0] = '\0';

    DeviceNum = FIND_AMOUNT_MATCHED;
    if (PlxPciDeviceFind(
            pDevice,
            &DeviceNum
            ) != ApiSuccess)
    {
        return 0;
    }

    if (DeviceNum == 0)
        return 0;

    PlxPrintf("\n");
    for (i=0; i<DeviceNum; i++)
    {
        pDevice->BusNumber       = (U8)-1;
        pDevice->SlotNumber      = (U8)-1;
        pDevice->VendorId        = (U16)-1;
        pDevice->DeviceId        = (U16)-1;
        pDevice->SerialNumber[0] = '\0';

        PlxPciDeviceFind(
            pDevice,
            &i
            );

        PlxPrintf(
            "\t\t    %u. %.4x %.4x  [%04x - bus %.2x  slot %.2x]\n",
            i+1, pDevice->DeviceId, pDevice->VendorId,
            DetermineChipType(pDevice), pDevice->BusNumber, pDevice->SlotNumber
            );
    }

    PlxPrintf(
        "\t\t    0. Cancel\n\n"
        "\t  Device Selection --> "
        );

    while ((i = Plx_getch()-'0') > DeviceNum);

    PlxPrintf("%u\n\n", i);

    if (i == 0)
        return -1;

    pDevice->BusNumber       = (U8)-1;
    pDevice->SlotNumber      = (U8)-1;
    pDevice->VendorId        = (U16)-1;
    pDevice->DeviceId        = (U16)-1;
    pDevice->SerialNumber[0] = '\0';

    i--;
    PlxPciDeviceFind(
        pDevice,
        &i
        );

    return (S8)DeviceNum;
}




/*********************************************************************
 *
 * Function   : DetermineChipType
 *
 * Description: Determine PLX chip type from serial number
 *
 * Returns    : Hex equivalent of PLX chip
 *              -1,  if unknown
 *
 ********************************************************************/
U32
DetermineChipType(
    DEVICE_LOCATION *pDev
    )
{
    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9050", strlen("Pci9050")) == 0)
        return 0x9050;

    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9030", strlen("Pci9030")) == 0)
        return 0x9030;

    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9080", strlen("Pci9080")) == 0)
        return 0x9080;

    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9054", strlen("Pci9054")) == 0)
        return 0x9054;

    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9056", strlen("Pci9056")) == 0)
        return 0x9056;

    if (Plx_strncasecmp((char*)pDev->SerialNumber, "Pci9656", strlen("Pci9656")) == 0)
        return 0x9656;

    return (U32)-1;
}




/*********************************************************************
 *
 * Function   :  PlxSdkErrorText
 *
 * Description:  Returns the text string associated with a RETURN_CODE
 *
 ********************************************************************/
char*
PlxSdkErrorText(
    RETURN_CODE code
    )
{
    U16 i;


    i = 0;

    while (ApiErrors[i].code != ApiLastError)
    {
        if (ApiErrors[i].code == code)
        {
            return ApiErrors[i].text;
        }

        i++;
    }

    return ApiErrors[i].text;
}
