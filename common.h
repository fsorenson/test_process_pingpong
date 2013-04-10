#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#define __PACKED __attribute__ ((packed))

typedef enum { no = 0, false = 0, yes = 1, true = 1 } __PACKED bool;



#endif
