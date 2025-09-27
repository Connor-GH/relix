#pragma once
// Nonstandard macros used to make devices.
#define makedev(maj, min) ((dev_t)(((dev_t)(maj) << 32) | (dev_t)(min)))
#define major(dev) ((unsigned int)(((dev_t)(dev)) >> 32))
#define minor(dev) ((unsigned int)((dev_t)dev))
