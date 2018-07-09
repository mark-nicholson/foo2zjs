#
# This is more of a shim
#

all:
	$(MAKE) -C foo2zjs

install:
	$(MAKE) -C foo2zjs

update-models:
	$(MAKE) -C foo2zjs getweb
	cd foo2zjs; ./getweb all
