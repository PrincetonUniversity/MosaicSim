# Base name of the test bench to compile.
NAME     = bfs

# Initial compiler settings to create the human-readable bitcode file.
CXX      = clang++
CXXFLAGS = -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -S
CXXSRC   = $(NAME).c
CXXBC    = int/$(NAME).ll

# Linker settings to hook in the tracer.
LL       = llvm-link
LLSRC    = ../../tools/tracer.llvm
LLDST    = int/$(NAME)-integrated.ll

# Optimizer settings to load in profiling and graph-generating tools.
OPT      = opt
OPTFLAGS = -instnamer -always-inline
OPTLIB1  = ../../lib/libRecordDynamicInfo.so
OPTPASS1 = -recorddynamicinfo
OPTSRC1  = $(LLDST)
OPTED    = int/$(NAME)-pass.ll
OPTLIB2  = ../../lib/libGraphGen.so
OPTPASS2 = -graphgen
OPTSRC2  = $(LLDST)

# DOT/Graphviz settings to generate the graph image from the profiled DOT file.
DOT      = dot
DOTFMT   = png
DOTIN    = int/graphDiagram.dot
DOTOUT   = output/$(NAME).png

# LLVM end-to-end compiler settings for creating object/assembly files.
LLC      = llc
LLFTYPE1 = obj
LLFTYPE2 = asm
LLSRC1   = $(OPTED)
LLSRC2   = $(OPTED)
LLDST1   = int/$(NAME).o
LLDST2   = int/$(NAME).s

# Final output generation settings.
OUTSRC   = $(LLDST1)
OUTDST   = $(NAME)-profile.out

# Fully compile the test bench into the requisite parts, step by step.
all: $(SRC)
	mkdir -p int
	mkdir -p output
	$(CXX) $(CXXFLAGS) $(CXXSRC) -o $(CXXBC)
	$(LL)  -S $(LLSRC) $(CXXBC) -o $(LLDST)
	$(OPT) -S $(OPTFLAGS) -load $(OPTLIB1) $(OPTPASS1) $(OPTSRC1) -o $(OPTED)
	$(OPT) -S $(OPTFLAGS) -load $(OPTLIB2) $(OPTPASS2) $(OPTSRC2) > /dev/null
	$(DOT) -T $(DOTFMT) $(DOTIN) -o $(DOTOUT)
	$(LLC) -filetype=$(LLFTYPE1) $(LLSRC1) -o $(LLDST1)
	$(LLC) -filetype=$(LLFTYPE2) $(LLSRC2) -o $(LLDST2)
	$(CXX) $(OUTSRC) -o $(OUTDST)

# Actually run the generated output file.
run: all
	./$(OUTDST)

# Clean up the directories by removing compiled/generated files.
clean:
	rm -f $(NAME)
	rm -f *.out
	rm -f output/*.txt
	rm -f output/*.png
	rm -f int/*.dot
	rm -f int/*.ll
	rm -f int/*.o
