all:
	#make --directory=bfs
	#make --directory=fft
	make --directory=gemm
	#make --directory=md
	make --directory=scan
	make --directory=sort
	make --directory=stencil
	make --directory=triad

run: all
	make --directory=bfs     run
	make --directory=fft     run
	make --directory=gemm    run
	make --directory=md      run
	make --directory=scan    run
	make --directory=sort    run
	make --directory=stencil run
	make --directory=triad   run

clean:
	make --directory=bfs     clean
	make --directory=fft     clean
	make --directory=gemm    clean
	make --directory=md      clean
	make --directory=scan    clean
	make --directory=sort    clean
	make --directory=stencil clean
	make --directory=triad   clean
