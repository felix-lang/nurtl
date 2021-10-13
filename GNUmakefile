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

tut1:
	clang++ -g -std=c++17 -ferror-limit=1 -O2 -Isrc/lib -Isrc/chips src/prog/tut01.cxx -o tut01
	./tut01

tut2:
	clang++ -g -std=c++17 -ferror-limit=1 -O1 -Isrc/lib -Isrc/chips src/prog/tut02.cxx -o tut02
	./tut02


docs:
	sh texify.sh csp_theory

