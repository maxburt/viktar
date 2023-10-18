#pragma once

#ifndef _VIKTAR_H
# define _VIKTAR_H

#ifndef FALSE
# define FALSE 0
#endif // FALSE
#ifndef TRUE
# define TRUE 1
#endif // TRUE

# define OPTIONS "xctTf:hv"

#define VIKTAR_FILE "!<viktar>\n"

#define VIKTAR_MAX_FILE_NAME_LEN 19

typedef struct viktar_header_s {
    char      viktar_name[VIKTAR_MAX_FILE_NAME_LEN]; /* Member file name, usually NULL terminated. */

    mode_t    st_mode;        /* File type and mode */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    off_t     st_size;        /* Total size, in bytes */

    struct timespec st_atim;  /* Time of last access */
    struct timespec st_mtim;  /* Time of last modification */
    struct timespec st_ctim;  /* Time of last status change */
} viktar_header_t;

#endif // _VIKTAR_H
