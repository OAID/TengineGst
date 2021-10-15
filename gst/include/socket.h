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
 
#ifndef		__SOCKET_H__
#define		__SOCKET_H__

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/socket.h>

#ifdef		__cplusplus
extern	"C"		{
#endif

int set_sock_attr(int fd, int reuseaddr_flag, int send_timeout, int recv_timeout, int send_size, int recv_size);
int set_sock_recvtimeout(int fd, int timeout_ms);
void close_socket(int *socket);
int get_sock_ip(int sock);
int get_sock_port(int sock);
int tcp_create_and_listen(int port);
int tcp_create_and_listen_ext(int port);
int create_sock(int type);
int bind_sock(int sockfd, int ip, int nPort);
int sock_set_linger(int sockfd);
int set_sock_nodelay(int fd);
int set_sock_keepalive(int fd);
int tcp_listen(const char *host, const char *serv, int *addrlenp);
int my_select(int *fd_array, int fd_num, int fd_type, int time_out);
int set_sock_boardcast(int fd);
int send_broadcast(int sockfd, int port, char *data, int len);
/* noblock operation */
int create_noblock_tcp_socket(void);
int create_noblock_udp_socket(void);
int set_sock_noblock(int sockfd);
int set_sock_block(int sockfd);
int tcp_noblock_connect(const char *server, int port, int timeout_ms);
int tcp_noblock_send(int fd, char *buf, int size);
int tcp_noblock_recv(int fd, char *buf, int size);
int udp_noblock_send(int sock, void *buf, int size, struct sockaddr *distAddr);
int udp_noblock_recv(int sockfd, char *buf, int size, struct sockaddr *from, int *fromlen);
/* block operation */
int create_block_tcp_socket(void);
int create_block_udp_socket(void);
int tcp_block_connect(const char *dstHost, int port);
int tcp_block_connect_ext(const char *dstHost, int port);
int tcp_block_send(int fd, const void *data, int n);
int tcp_block_send_ext(int fd, const void *data, int n);
int tcp_block_recv(int sockfd, void *rcvBuf, int rcvSize);
int tcp_block_recv_ext(int sockfd, void *rcvBuf, int rcvSize);
int tcp_block_accept(int fd, struct sockaddr *sa, int *salenptr);
int tcp_block_accept_ext(int fd, struct sockaddr *sa, socklen_t *salenptr);
int tcp_block_recv_once(int sockfd, char *rcvBuf, int rcvSize);
int tcp_force_recv(int sockfd, void *data, int size);

#ifdef		__cplusplus
}
#endif

#endif
