module stdarg;

public import core.stdc.stdarg : __va_list_tag;
alias va_list = __builtin_va_list;
alias va_arg = __builtin_va_arg;
alias va_end = __builtin_va_end;
alias va_start = __builtin_va_start;
