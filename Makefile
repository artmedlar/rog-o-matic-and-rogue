# Top-level Makefile for Rog-O-Matic XIV + Rogue-Clone
# Builds both projects with a single 'make' command

all: rogue rogomatic

rogue:
	@echo "=== Building Rogue-Clone ==="
	$(MAKE) -C rogue

rogomatic:
	@echo "=== Building Rog-O-Matic XIV ==="
	$(MAKE) -C rgm14

clean:
	@echo "=== Cleaning Rogue-Clone ==="
	$(MAKE) -C rogue clean
	@echo "=== Cleaning Rog-O-Matic XIV ==="
	$(MAKE) -C rgm14 clean

.PHONY: all rogue rogomatic clean

