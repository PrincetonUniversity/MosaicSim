# MosaicSim
MosaicSim is a lightweight, modular simulator for heterogeneous and hardware-software co-design systems. It relies on the [DecadesCompiler](https://github.com/PrincetonUniversity/DecadesCompiler).
MosaicSim: 
* is tightly integrated with the LLVM framework, providing agile programming models, enabling full-stack approaches; 
*  provides abstract tile models capturing pragmatic microarchitectural details and specialized tile-to-tile interactions; and 
*  provides support for accelerator model integration to create complex heterogeneous systems.

To learn more about the simulator, please read our [paper](https://dl.acm.org/doi/10.1145/3400302.3415751)

To get started on using MosaicSim, we prepared a step-by-step tutorial on how to install, compile and execute it [here](Inhttps://github.com/PrincetonUniversity/DecadesCompilerstallationCompilationExecution.md)


MosaicSim provides a platform that can be adapted to simulate a range of different architectures and compiler tools. These are the required
inputs in order to use the MosaicSim simulator:
1. A graph input that represents a static graph of LLVM instructions for a given algorithm. 
MosaicSim's compiler pass that can generate this graph can be incorporated any LLVM based compiler.
We provide one incorporated into the DECADES compiler which is a LLVM based compiler tool that provides 
do-all and decoupled parallelization (DeSC or GraphAttack).
2. A sim config, which contains the architecture specific information for a specific chip design and memory system. You can see an example here in config/sim_desc.txt.
3. Core configs, which are cofigurations of specific cores included in the sim config.
4. Parallelization mode such as do-all, decoupled and number of threads.


In addition, we prepared a tutorial on how to write your code in order for the compiler tools to be utilized as well as 
how to incorporate new acceletor tiles [here](HowToUseMosaicSim.md)
 
