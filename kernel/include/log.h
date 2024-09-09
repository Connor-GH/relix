#pragma once
#include <buf.h>
void
initlog(int dev);
void
log_write(struct buf *);
void
begin_op(void);
void
end_op(void);
