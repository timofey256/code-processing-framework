// Source : https://github.com/alanxz/rabbitmq-c/blob/master/examples/utils.h

// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#pragma once

#include <cstdint>
#include <chrono>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <stdint.h>


void die(const char *fmt, ...);
void die_on_error(int x, char const *context);
void die_on_amqp_error(amqp_rpc_reply_t x, char const *context);

void amqp_dump(void const *buffer, size_t len);

void microsleep(int usec);
