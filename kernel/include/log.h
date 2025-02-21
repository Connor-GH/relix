#pragma once
#include <buf.h>
void
initlog(dev_t dev);
void
log_write(struct buf *);
void
begin_op(void);
void
end_op(void);
