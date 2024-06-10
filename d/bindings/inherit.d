module inherit;
enum inherit(string base) = base ~ " __baseStruct" ~base.mangleof
~"; alias __baseStruct"~base.mangleof~ " this;";
