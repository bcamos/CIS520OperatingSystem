#ifndef _FRAME_H
#define _FRAME_H

#include <stdio.h>
#include "threads/thread.h"
#include <list.h>

struct frame_elememt
{
	void *frame;
	tid_t owner_tid;
	struct list_elem elem;
};

void init_frames();

#endif