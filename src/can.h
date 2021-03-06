/** @file can.h
 *
 *  A versatile software tool to analyse and configure CANopen devices.
 *
 *  Copyright (c) 2022, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CAN_H
#define CAN_H

#include "SDL.h"
#include "lua.h"
#include "core.h"

typedef struct can_message
{
    Uint16 id;
    Uint8  length;
    Uint8  data[8];

} can_message_t;

void     can_init(core_t* core_t);
void     can_deinit(core_t* core);
void     can_quit(core_t* core);
Uint32   can_write(can_message_t* message);
Uint32   can_read(can_message_t* message);
void     can_set_baud_rate(Uint8 command, core_t* core);
int      lua_can_write(lua_State* L);
void     lua_register_can_commands(core_t* core);
void     can_print_error_message(const char* context, Uint32 can_status);
void     can_print_baud_rate_help(core_t* core);
SDL_bool is_can_initialised(core_t* core);

#endif /* CAN_H */
