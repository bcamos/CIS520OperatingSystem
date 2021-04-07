#include "vm/frame.h"
#include "threads/synch.h"

static struct list frames;
static struct lock frame_table_lock;
#define lock_frames() ( lock_acquire(&frame_table_lock) )
#define unlock_frames() ( lock_release(&frame_table_lock) )

void 
init_frames()
{
	list_init(&frames);
	lock_init(&frame_table_lock);
}