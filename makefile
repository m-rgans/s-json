obj.cpp: s_json.hpp
	echo "#define SJSON_OBJECT\n#define SJSON_TEST\n#include \"s_json.hpp\"" > $@

sjson_test: obj.cpp
	g++ -g -o $@ $< -lgtest

test: sjson_test
	./$<

clean:
	rm -f sjson_test obj.cpp