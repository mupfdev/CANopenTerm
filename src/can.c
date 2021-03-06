/** @file can.c
 *
 *  A versatile software tool to analyse and configure CANopen devices.
 *
 *  Copyright (c) 2022, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL.h"
#include "lua.h"
#include "can.h"
#include "core.h"
#include "printf.h"
#include "table.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include "PCANBasic.h"

static int can_monitor(void *core);

void can_init(core_t* core)
{
    if (NULL == core)
    {
        return;
    }

    core->can_monitor_th = SDL_CreateThread(can_monitor, "CAN monitor thread", (void *)core);
}

void can_deinit(core_t* core)
{
    if (NULL == core)
    {
        return;
    }

    core->can_status         = 0;
    core->is_can_initialised = SDL_FALSE;

    CAN_Uninitialize(PCAN_USBBUS1);
}

void can_quit(core_t* core)
{
    if (NULL == core)
    {
        return;
    }

    if (SDL_TRUE == is_can_initialised(core))
    {
        can_deinit(core);
    }

    SDL_DetachThread(core->can_monitor_th);
}

Uint32 can_write(can_message_t* message)
{
    int      index;
    TPCANMsg pcan_message = { 0 };

    pcan_message.ID      = message->id;
    pcan_message.MSGTYPE = PCAN_MESSAGE_STANDARD;
    pcan_message.LEN     = message->length;

    for (index = 0; index < 8; index += 1)
    {
        pcan_message.DATA[index] = message->data[index];
    }

    return (Uint32)CAN_Write(PCAN_USBBUS1, &pcan_message);
}

Uint32 can_read(can_message_t* message)
{
    int      index;
    Uint32   can_status;
    TPCANMsg pcan_message = { 0 };

    can_status = CAN_Read(PCAN_USBBUS1, &pcan_message, NULL);

    message->id     = pcan_message.ID;
    message->length = pcan_message.LEN;

    for (index = 0; index < 8; index += 1)
    {
        message->data[index] = pcan_message.DATA[index];
    }

    return can_status;
}

void can_set_baud_rate(Uint8 command, core_t* core)
{
    if (NULL == core)
    {
        return;
    }

    core->baud_rate = command;

    if (SDL_TRUE == is_can_initialised(core))
    {
        can_deinit(core);
    }
}

int lua_can_write(lua_State* L)
{
    int    can_id         = luaL_checkinteger(L, 1);
    int    length         = luaL_checkinteger(L, 2);
    Uint32 data_d0_d3     = luaL_checkinteger(L, 3);
    Uint32 data_d4_d7     = luaL_checkinteger(L, 4);
    can_message_t message = { 0 };

    message.id      = can_id;
    message.length  = length;

    message.data[3] = ( data_d0_d3        & 0xff);
    message.data[2] = ((data_d0_d3 >> 8)  & 0xff);
    message.data[1] = ((data_d0_d3 >> 16) & 0xff);
    message.data[0] = ((data_d0_d3 >> 24) & 0xff);
    message.data[7] = ( data_d4_d7        & 0xff);
    message.data[6] = ((data_d4_d7 >> 8)  & 0xff);
    message.data[5] = ((data_d4_d7 >> 16) & 0xff);
    message.data[4] = ((data_d4_d7 >> 24) & 0xff);

    if (0 != can_write(&message))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void lua_register_can_commands(core_t* core)
{
    lua_pushcfunction(core->L, lua_can_write);
    lua_setglobal(core->L, "can_write");
}

void can_print_error_message(const char* context, Uint32 can_status)
{
    if (PCAN_ERROR_OK != can_status)
    {
        char err_message[100] = { 0 };

        CAN_GetErrorText(can_status, 0x09, err_message);
        if (NULL == context)
        {
            c_log(LOG_WARNING, "%s", err_message);
        }
        else
        {
            c_log(LOG_WARNING, "%s: %s", context, err_message);
        }
    }
}

void can_print_baud_rate_help(core_t* core)
{
    table_t      table         = { DARK_CYAN, DARK_WHITE, 3, 13, 6 };
    char         status[14][7] = { 0 };
    unsigned int status_index  = core->baud_rate;
    unsigned int index;

    if (status_index > 13)
    {
        status_index = 13;
    }

    for (index = 0; index < 14; index += 1)
    {
        if (status_index == index)
        {
            SDL_snprintf(status[index], 7, "Active");
        }
        else
        {
            SDL_snprintf(status[index], 2, " ");
        }
    }

    table_print_header(&table);
    table_print_row("CMD", "Description", "Status", &table);
    table_print_divider(&table);
    table_print_row("  0", "1 MBit/s",      status[0],  &table);
    table_print_row("  1", "800 kBit/s",    status[1],  &table);
    table_print_row("  2", "500 kBit/s",    status[2],  &table);
    table_print_row("  3", "250 kBit/s",    status[3],  &table);
    table_print_row("  4", "125 kBit/s",    status[4],  &table);
    table_print_row("  5", "100 kBit/s",    status[5],  &table);
    table_print_row("  6", "95,238 kBit/s", status[6],  &table);
    table_print_row("  7", "83,333 kBit/s", status[7],  &table);
    table_print_row("  8", "50 kBit/s",     status[8],  &table);
    table_print_row("  9", "47,619 kBit/s", status[9],  &table);
    table_print_row(" 10", "33,333 kBit/s", status[10], &table);
    table_print_row(" 11", "20 kBit/s",     status[11], &table);
    table_print_row(" 12", "10 kBit/s",     status[12], &table);
    table_print_row(" 13", "5 kBit/s",      status[13], &table);
    table_print_footer(&table);
}

SDL_bool is_can_initialised(core_t* core)
{
    if (NULL == core)
    {
        return SDL_FALSE;
    }

    return core->is_can_initialised;
}

static int can_monitor(void *core_pt)
{
    char    err_message[100] = { 0 };
    core_t* core             = core_pt;

    if (NULL == core)
    {
        return 1;
    }

    core->baud_rate = 3;

    while (SDL_TRUE == core->is_running)
    {
        while (SDL_FALSE == is_can_initialised(core))
        {
            TPCANBaudrate baud_rate;

            switch (core->baud_rate)
            {
                case 0:
                    baud_rate = PCAN_BAUD_1M;
                    break;
                case 1:
                    baud_rate = PCAN_BAUD_800K;
                    break;
                case 2:
                    baud_rate = PCAN_BAUD_500K;
                    break;
                case 3:
                default:
                    baud_rate = PCAN_BAUD_250K;
                    break;
                case 4:
                    baud_rate = PCAN_BAUD_125K;
                    break;
                case 5:
                    baud_rate = PCAN_BAUD_100K;
                    break;
                case 6:
                    baud_rate = PCAN_BAUD_95K;
                    break;
                case 7:
                    baud_rate = PCAN_BAUD_83K;
                    break;
                case 8:
                    baud_rate = PCAN_BAUD_50K;
                    break;
                case 9:
                    baud_rate = PCAN_BAUD_47K;
                    break;
                case 10:
                    baud_rate = PCAN_BAUD_33K;
                    break;
                case 11:
                    baud_rate = PCAN_BAUD_20K;
                    break;
                case 12:
                    baud_rate = PCAN_BAUD_10K;
                    break;
                case 13:
                    baud_rate = PCAN_BAUD_5K;
                    break;
            }

            core->can_status = CAN_Initialize(
                PCAN_USBBUS1,
                baud_rate,
                PCAN_USB,
                0, 0);

            CAN_GetErrorText(core->can_status, 0x09, err_message);

            if ((core->can_status & PCAN_ERROR_OK) == core->can_status)
            {
                c_log(LOG_SUCCESS, "CAN successfully initialised");
                core->is_can_initialised = SDL_TRUE;
                c_print_prompt();
            }

            SDL_Delay(10);
            continue;
        }

        core->can_status = CAN_GetStatus(PCAN_USBBUS1);

        if (PCAN_ERROR_ILLHW == core->can_status)
        {
            core->can_status         = 0;
            core->is_can_initialised = SDL_FALSE;

            CAN_Uninitialize(PCAN_USBBUS1);
            c_log(LOG_WARNING, "CAN de-initialised: USB-dongle removed?");
            c_print_prompt();
        }
        SDL_Delay(10);
    }

    return 0;
}
