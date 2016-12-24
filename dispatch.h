#ifndef DISPATCH_H
#define DISPATCH_H

extern int module_count;
extern char* module_names[];
void dispatch(int conn, request req);

#endif