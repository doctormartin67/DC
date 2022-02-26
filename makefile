.PHONY: check
check: VBA

VBA:
	cd test/$@; make; ./test; make clean
