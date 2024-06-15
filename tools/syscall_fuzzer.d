import std.stdio;

string[] garbage_int = [
  // don't use 0, 1, 2 because of std{in,out,err}
  "-1", "3", "4", "4294967295",
  "-2", "-3", "12", "1200",
  "2147483647", "-2147483647"
];

void generate_garbage_for_int_1arg(SyscallInfo info) {
  foreach (gar; garbage_int) {
    writeln("assert((", info.return_type, ") ", info.name, "(", gar, ") == -1);");
  }
}
void begin_sequence() {
  writeln("#include <user.h>\n",
      "#include <stdio.h>\n",
      "#include <assert.h>\n",
      "int main(void) {");
}

void end_sequence() {
  writeln("exit(0);\n",
      "}");
}



enum SyscallType {
  NONE,
  SHORT,
  INT,
  INT_PTR,
  CHAR_PTR,
  CHAR_PTR_PTR,
  STRUCT_STAT,
  STRUCT_RTCDATE,
}

string type_to_string(SyscallType arg) {
  with (SyscallType) {
    final switch (arg) {
    case NONE: return "";
    case SHORT: return "short";
    case INT: return "int";
    case INT_PTR: return "int *";
    case CHAR_PTR: return "char *";
    case CHAR_PTR_PTR: return "char **";
    case STRUCT_STAT: return "struct stat *";
    case STRUCT_RTCDATE: return "struct rtcdate *";
    }
  }
}
class SyscallInfo {
  string return_type;
  string name;
  SyscallType arg1 = SyscallType.NONE;
  SyscallType arg2 = SyscallType.NONE;
  SyscallType arg3 = SyscallType.NONE;
  SyscallType arg4 = SyscallType.NONE;
  SyscallType arg5 = SyscallType.NONE;
  SyscallType arg6 = SyscallType.NONE;
  alias st = SyscallType;

  this(string return_type, string name, st arg1, st arg2,
      st arg3, st arg4,
      st arg5, st arg6) {

    this.return_type = return_type;
    this.name = name;
    this.arg1 = arg1;
    this.arg2 = arg2;
    this.arg3 = arg3;
    this.arg4 = arg4;
    this.arg5 = arg5;
    this.arg6 = arg6;
  }
  override string toString() const {
    return return_type ~ " " ~ name ~ " "
      ~ type_to_string(arg1) ~ " "
      ~ type_to_string(arg2) ~ " "
      ~ type_to_string(arg3) ~ " "
      ~ type_to_string(arg4) ~ " "
      ~ type_to_string(arg5) ~ " "
      ~ type_to_string(arg6);
  }

}

SyscallType string_to_type(string arg) {
  with (SyscallType) {
    switch (arg) {
    default:
    case "": return NONE;
    case "short": return NONE;
    case "int": return INT;
    case "int *": return INT_PTR;
    case "char *": return CHAR_PTR;
    case "char **": return CHAR_PTR_PTR;
    case "struct stat *": return STRUCT_STAT;
    case "struct rtcdate *": return STRUCT_RTCDATE;
    }
  }
}

string[] g_args;
string try_(int idx) => idx >= g_args.length ? "" : g_args[idx];


int main(string[] args) {

  g_args = args;
  assert(args.length <= 8);
  assert(args.length > 3); // progname syscall arg1
  SyscallInfo si = new SyscallInfo(args[1], args[2], try_(3).string_to_type(),
      try_(4).string_to_type(),
      try_(5).string_to_type(),
      try_(6).string_to_type(),
      try_(7).string_to_type(),
      try_(7).string_to_type()
      );
  begin_sequence();
  generate_garbage_for_int_1arg(si);
  end_sequence();
  return 0;
}
