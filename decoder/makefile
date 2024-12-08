.PHONY : clean

CXXFLAGS := $(CXXFLAGS) -g -DBX_STANDALONE_DECODER -DBX_INSTR_STORE_OPCODE_BYTES
objects = disasm.o fetchdecode32.o fetchdecode64.o main.o
decoder: $(objects)
	cc -o decoder $(objects)
clean :
	rm decoder $(objects)