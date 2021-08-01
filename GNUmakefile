help:
	# Makefile help

test: test1 test2

test1:
	# CSP TEST
	clang++ -g -std=c++17 -ferror-limit=1 -O2 -Isrc/lib -Isrc/chips src/prog/ex01.cxx -o ex01
	./ex01

test2:
	clang++ -g -std=c++17 -ferror-limit=1 -O2 -Isrc/lib -Isrc/chips src/prog/ex02.cxx -o ex02
	./ex02

docs:
	sh texify.sh csp_theory
	sh texify.sh csp_howto
	sh texify.sh csp_tutorial
	sh texify.sh csp_reference

