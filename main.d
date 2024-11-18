//import result;
import core.stdc.stdio : printf;
import std.sumtype : SumType, match;

/*auto my_func(int x) {
	alias R = Result!(int, string);
	if (x == 6)
		return R.ok(0);
	else
		return R.err("uh oh!");
}
struct Empty {}*/


auto my_func2(int x) {
	alias R2 = SumType!(int, string);
	if (x == 6)
		return R2(0);
	else
		return R2("uh oh!");
}

extern(C) int main() {
	/*Result!(int, string) x = my_func(6);

	printf(result.match(x,
		(int i) => "yay!",
		(string s) => "bad."
	).ptr);*/

	SumType!(int, string) x2 = my_func2(6);

	printf(x2.match!(
		(int i) => "yay!",
		(string s) => "bad."
		).ptr);

	return 0;
}
