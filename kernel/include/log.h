#pragma once
#if __KERNEL__
#include <buf.h>
#include "lib/compiler_attributes.h"
void
initlog(dev_t dev);
void
log_write(struct block_buffer *);
void
begin_op(void) __acquires(op);
void
end_op(void) __releases(op);
#endif
