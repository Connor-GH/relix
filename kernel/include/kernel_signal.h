#pragma once
#include <signal.h>
sighandler_t kernel_attach_signal(int signum, sighandler_t handler);
