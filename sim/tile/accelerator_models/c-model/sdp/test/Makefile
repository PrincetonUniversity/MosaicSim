RM := rm -rf

OBJS += sdp_app.o sdp_model.o

CFLAGS = -Wall -O3 -I../../ -I../

DEPS = ../../accelerators.hpp ../sdp_model.hpp

all: sdp_app

sdp_app: $(OBJS)
	@echo 'Building target: $@'
	g++ -o $@ $(OBJS)
	@echo 'Finished building target: $@'

%.o: ../%.cpp $(DEPS)
	@echo 'Building object file: $<'
	g++ $(CFLAGS) -c $<
	@echo 'Finished building: $<'

clean:
	$(RM) $(OBJS) sdp_app

.PHONY: all clean
