#ifndef _DSM_H
#define _DSM_H

extern int dsm_node_id;
extern int dsm_node_num;


void read_size(int fd, size_t *size, int *closed);
void read_all(int fd, char *buffer, size_t size, int *closed);
void write_all(int fd, const char *buf, size_t count);

char *dsm_init(int argc, char **argv);
void  dsm_finalize(void);
int   dsm_page_size(void);


#endif

