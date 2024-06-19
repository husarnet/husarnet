// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// Couple of macrodefinitions and type declarations
// Extracted from MacOS SDK
#pragma once

#define UTUN_CONTROL_NAME "com.apple.net.utun_control"
#define SYSPROTO_CONTROL 2
#define AF_SYS_CONTROL 2
#define CTLIOCGINFO _IOWR('N', 3, struct ctl_info) /* get id from name */
#define MAX_KCTL_NAME 96

struct ctl_info {
  u_int32_t ctl_id;             /* Kernel Controller ID  */
  char ctl_name[MAX_KCTL_NAME]; /* Kernel Controller Name (a C string) */
};

struct sockaddr_ctl {
  u_char sc_len;        /* depends on size of bundle ID string */
  u_char sc_family;     /* AF_SYSTEM */
  u_int16_t ss_sysaddr; /* AF_SYS_KERNCONTROL */
  u_int32_t sc_id;      /* Controller unique identifier  */
  u_int32_t sc_unit;    /* Developer private unit number */
  u_int32_t sc_reserved[5];
};
