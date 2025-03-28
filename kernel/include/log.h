#pragma once
#if __KERNEL__
#include <buf.h>
#include "compiler_attributes.h"
void
initlog(dev_t dev);
void
log_write(struct buf *);
void
begin_op(void) __acquires(op);
void
end_op(void) __releases(op);
#endif
