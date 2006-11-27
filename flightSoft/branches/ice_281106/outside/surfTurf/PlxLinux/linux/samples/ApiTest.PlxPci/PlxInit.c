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
#include "PciDrvApi.h"
#include "PciRegs.h"
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
 * Returns    : Total devices found
 *              -1,  if user cancelled the selection
 *
 ********************************************************************/
S8
SelectDevice(
    PLX_DEVICE_KEY *pKey
    )
{
    U8             i;
    U8             NumDevices;
    RETURN_CODE    rc;
    PLX_DEVICE_KEY DevKey[20];


    PlxPrintf("\n");

    i = 0;
    do
    {
        DevKey[i].bus         = PCI_FIELD_IGNORE;
        DevKey[i].slot        = PCI_FIELD_IGNORE;
        DevKey[i].function    = PCI_FIELD_IGNORE;
        DevKey[i].DeviceId    = PCI_FIELD_IGNORE;
        DevKey[i].VendorId    = PCI_FIELD_IGNORE;
        DevKey[i].SubDeviceId = PCI_FIELD_IGNORE;
        DevKey[i].SubVendorId = PCI_FIELD_IGNORE;
        DevKey[i].Revision    = PCI_FIELD_IGNORE;

        rc =
            PlxPci_DeviceFind(
                &DevKey[i],
                i
                );

        if (rc == ApiSuccess)
        {
            PlxPrintf(
                "\t\t    %2d. %.4x %.4x  [bus %02x  slot %02x  fn %02x]\n",
                i+1, DevKey[i].DeviceId, DevKey[i].VendorId,
                DevKey[i].bus, DevKey[i].slot, DevKey[i].function
                );

            i++;
        }
    }
    while (rc == ApiSuccess);

    if (i == 0)
        return 0;

    NumDevices = i;

    PlxPrintf(
        "\t\t     0. Cancel\n\n"
        );

    do
    {
        PlxPrintf(
            "\t  Device Selection --> "
            );

        Plx_scanf(
            "%d",
            &i
            );
    }
    while (i > NumDevices);

    if (i == 0)
        return -1;

    *pKey = DevKey[i - 1];

    return (S8)NumDevices;
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
