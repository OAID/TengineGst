/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2021, OPEN AI LAB
 * Author: wlwu@openailab.com
 */
 
#include "socket.h"
#include "debug.h"
 
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

ssize_t fd_puts(int fd, const void *buf, size_t count);

/*ssmtp.c fd_puts for send_to64frombits*/
static const char base64digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define BAD	-1
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)

static const char base64val[] = 
{
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 62, BAD, 
	BAD, BAD, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, BAD, BAD, BAD, BAD, BAD, BAD, 
	BAD, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
	25, BAD, BAD, BAD, BAD, BAD, BAD, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, BAD, BAD, BAD, BAD, BAD
};

/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
void to64frombits(unsigned char *out, const unsigned char *in, int inlen)
{
	for (; inlen >= 3; inlen -= 3)
	{
		*out++ = base64digits[in[0] >> 2];
		*out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = base64digits[in[2] & 0x3f];
		in += 3;
	}
	if (inlen > 0)
	{
		unsigned char fragment;

		*out++ = base64digits[in[0] >> 2];
		fragment = (in[0] << 4) & 0x30;
		if (inlen > 1)
		{
			fragment |= in[1] >> 4;
		}
		*out++ = base64digits[fragment];
		*out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}
	*out = '\0';
}

/* base 64 to raw bytes in quasi-big-endian order, returning count of bytes */
int from64tobits(char *out, const char *in)
{
	int len = 0;
	register unsigned char digit1 = 0, digit2 = 0, digit3 = 0, digit4 = 0;

	if (in[0] == '+' && in[1] == ' ')
	{
		in += 2;
	}
	if (*in == '\r')
	{
		return (0);
	}

	do
	{
		digit1 = in[0];
		if (DECODE64(digit1) == BAD)
			return (-1);
		digit2 = in[1];
		if (DECODE64(digit2) == BAD)
			return (-1);
		digit3 = in[2];
		if (digit3 != '=' && DECODE64(digit3) == BAD)
			return (-1);
		digit4 = in[3];
		if (digit4 != '=' && DECODE64(digit4) == BAD)
			return (-1);
		in += 4;
		*out++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
		++len;
		if (digit3 != '=')
		{
			*out++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
			++len;
			if (digit4 != '=')
			{
				*out++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
				++len;
			}
		}
	}
	while (*in &&  *in != '\r' && digit4 != '=');

	return (len);
}

/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
int send_to64frombits(int fd, const unsigned char *in, size_t inlen, int timeout)
{
	char out_buf[80] = {0}, *out = NULL;
	size_t out_len = 0, total = 0;

	total = inlen;
	out_len = 0;
	out = out_buf;
	
	for (; inlen >= 3; inlen -= 3)
	{
		*out++ = base64digits[in[0] >> 2];
		*out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = base64digits[in[2] & 0x3f];
		in += 3;
		out_len += 4;
		if (out_len >= 72) // divided by 4
		{
			out_buf[out_len++] = '\r';
			out_buf[out_len++] = '\n';
			out_buf[out_len] = 0x00;

			if (tcp_noblock_send(fd, out_buf, out_len) < 0)
			{
				LOG_INFO("tcp_noblock_send failed");
				return (-1);
			}

			out_len = 0;
			out = out_buf;
		}

	}
	if (inlen > 0 || out_len > 0)
	{
		unsigned char fragment;

        if (inlen > 0)
        {
            *out++ = base64digits[in[0] >> 2];
            fragment = (in[0] << 4) & 0x30;
            if (inlen > 1)
                 fragment |= in[1] >> 4;
            *out++ = base64digits[fragment];
            *out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
            *out++ = '=';

            out_len += 4;
         }
		out_buf[out_len++] = '\r';
		out_buf[out_len++] = '\n';
		out_buf[out_len] = 0x00;

		if (tcp_noblock_send(fd, out_buf, out_len) < 0)
		{
			LOG_INFO("tcp_noblock_send failed");
			return (-1);
		}
	}

	return 0;
}

