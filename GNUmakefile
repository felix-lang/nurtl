help:
	# Makefile help

test:
	# CSP TEST
	clang++ -std=c++17 -ferror-limit=1 -O2 -Isrc/lib src/prog/ex01.cxx -o ex01
	./ex01

docs:
	sh texify.sh csp_theory
	sh texify.sh csp_howto
	sh texify.sh csp_tutorial
	sh texify.sh csp_reference

