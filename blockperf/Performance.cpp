#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

using namespace llvm;

// Anonymous namespace (local to this file).
namespace {
  struct PerformancePass : public FunctionPass {
    // Link in the ID defined below.
    static char ID;

    // Very simple constructor that just invokes the superclass' constructor.
    PerformancePass() : FunctionPass(ID) { }

    unsigned countCycles(unsigned opcode) {
      // Do a case analysis on all of the possible instruction types.
      switch (opcode) { 
        // Standard integer binary operators.
        case Instruction::Add:
        case Instruction::Sub:   return 1;

        // More time-consuming integer binary operators.
        case Instruction::Mul:   return 2;
        case Instruction::UDiv: 
        case Instruction::SDiv: 
        case Instruction::URem: 
        case Instruction::SRem:  return 3;

        // Standard floating-point binary operators.
        case Instruction::FAdd: 
        case Instruction::FSub:  return 2;

        // More time-consuming floating-point binary operators.
        case Instruction::FMul:  return 4;
        case Instruction::FDiv: 
        case Instruction::FRem:  return 5;
         
        // Logical operators.
        case Instruction::And:   
        case Instruction::Or :   
        case Instruction::Xor:   return 1;
        
        // Static memory instructions.
        case Instruction::Load:  return 9;
        case Instruction::Store: return 7;

        // Dynamic memory instructions.
        case Instruction::Alloca:       
        case Instruction::AtomicCmpXchg:
        case Instruction::AtomicRMW:    
        case Instruction::Fence:        
        case Instruction::GetElementPtr:

        // Terminating instructions.
        case Instruction::Ret:        
        case Instruction::Br:         
        case Instruction::Switch:     
        case Instruction::IndirectBr: 
        case Instruction::Invoke:     
        case Instruction::Resume:     
        case Instruction::Unreachable:
        case Instruction::CleanupRet: 
        case Instruction::CatchRet:   
        case Instruction::CatchPad:   
        case Instruction::CatchSwitch:
         
        // Convert instructions.
        case Instruction::Trunc:        
        case Instruction::ZExt:         
        case Instruction::SExt:         
        case Instruction::FPTrunc:      
        case Instruction::FPExt:        
        case Instruction::FPToUI:       
        case Instruction::FPToSI:       
        case Instruction::UIToFP:       
        case Instruction::SIToFP:       
        case Instruction::IntToPtr:     
        case Instruction::PtrToInt:     
        case Instruction::BitCast:      
        case Instruction::AddrSpaceCast:
         
        // Other instructions.
        case Instruction::ICmp:          
        case Instruction::FCmp:          
        case Instruction::PHI:           
        case Instruction::Select:        
        case Instruction::Call:          
        case Instruction::Shl:           
        case Instruction::LShr:          
        case Instruction::AShr:          
        case Instruction::VAArg:         
        case Instruction::ExtractElement:
        case Instruction::InsertElement: 
        case Instruction::ShuffleVector: 
        case Instruction::ExtractValue:  
        case Instruction::InsertValue:   
        case Instruction::LandingPad:    
        case Instruction::CleanupPad:    
        
        default: return 0; // TODO: Change the above to handle them.
      }
    }

    // The transformation that is run on all functions of the input program.
    virtual bool runOnFunction(Function &function) {

      // A mapping from LHSes to their approximate calculation times (in cycles).
      std::map<std::string,int> cycleMap;

      // Traverse over all of the basic blocks in the program.
      for (auto &basicBlock : function) {
        for (auto &inst : basicBlock) {
          cycleMap[inst.getOpcodeName()] = countCycles(inst.getOpcode());
        }
      }

      // Iterate through the map and print all of the cycle times.
      for (auto &entry : cycleMap) {
        outs() << entry.first << ": " << std::to_string(entry.second) << " cycles\n";
      }

      // The actual function is never modified.
      return false;
    }
  };
}

// This initial pass will have a simple identifier.
char PerformancePass::ID = 0;

// Register the pass with LLVM.
static RegisterPass<PerformancePass>
  X("blockperf", "Analyze cycle constraints on basic blocks.");