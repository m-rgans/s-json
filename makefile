obj.cpp: s_json.hpp
	echo "#define SJSON_OBJECT\n#define SJSON_TEST\n#include \"s_json.hpp\"" > $@

sjson_test: obj.cpp
	g++ -g -Wall -Wextra -o $@ $< -lgtest

test: sjson_test
	./$<

debug: sjson_test
	gdb ./$<

clean:
	rm -f sjson_test obj.cpp