/** @file sdo_client.c
 *
 *  A versatile software tool to analyse and configure CANopen devices.
 *
 *  Copyright (c) 2022, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifdef _WIN32
#include <windows.h>
#endif
#include "PCANBasic.h"

#include "SDL.h"
#include "core.h"
#include "sdo_client.h"
#include "nuklear.h"

Uint32 write_sdo(Uint8 index, Uint16 sub_index, data_type_t data_type, Uint32 data, Uint8 node_id)
{
    Uint32   can_status;
    TPCANMsg can_message;

    if (node_id > 0x7f)
    {
        node_id = 0x00 + (node_id % 0x7f);
    }

    can_message.ID      = 0x600 + node_id;
    can_message.MSGTYPE = PCAN_MESSAGE_STANDARD;
    can_message.LEN     = data_type;

    /*
    can_message.DATA[0] = command;
    can_message.DATA[1] = node_id;
    */

    can_status = CAN_Write(PCAN_USBBUS1, &can_message);
    if (PCAN_ERROR_OK != can_status)
    {
        char err_message[100] = { 0 };
        CAN_GetErrorText(can_status, 0x09, err_message);
        SDL_LogWarn(0, "Could not write SDO: %s", err_message);
    }

    return can_status;
}