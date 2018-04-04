
SYSTEM?=/data/dev/optimsoc/objdir/examples/sim/system_allct/build/optimsoc_examples_system_allct_sim_0/bld-verilator/Vtb_system_allct

measure: hello.vmem
	rm -rf ctf*
	$(SYSTEM) --meminit=hello.vmem --trace

vcd: hello.vmem
	rm -rf ctf*
	$(SYSTEM) --meminit=hello.vmem --trace --vcd
