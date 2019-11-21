RM := rm -rf

OBJS += gemm_app.o gemm_model.o

CFLAGS = -Wall -O3 -I../../ -I../

DEPS = ../../accelerators.hpp ../gemm_model.hpp

all: gemm_app

gemm_app: $(OBJS)
	@echo 'Building target: $@'
	g++ -o $@ $(OBJS)
	@echo 'Finished building target: $@'

%.o: ../%.cpp $(DEPS)
	@echo 'Building object file: $<'
	g++ $(CFLAGS) -c $<
	@echo 'Finished building: $<'

clean:
	$(RM) $(OBJS) gemm_app

.PHONY: all clean
