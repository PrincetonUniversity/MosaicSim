#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/CommandLine.h"
#include "cxxabi.h"
#include <string>

using namespace llvm;
bool FROM_NUMBA = false;
std::string KERNEL_STR = "_kernel_";
std::string KERNEL_TYPE = "_no_type_";
std::string RUN_DIR = "decades_base";
#define MAIN_STR "main"

namespace {
  struct RecordDynamicInfo : public ModulePass {
    static char ID;
    Function *printuBR;
    Function *printBR;
    Function *printSw;
    Function *printMem;
    Function *print_matmul;
    Function *print_sdp;
    Function *print_conv2d_layer;
    Function *print_dense_layer;
    Function *tracer_cleanup;
    Module *mod;
    std::map<BasicBlock*, int> bb_id_table;
    std::map<Instruction*, int> id_table;
    RecordDynamicInfo() : ModulePass(ID) {}

    void instrument_main(Function &f) {
  
      for (inst_iterator iI = inst_begin(f), iE = inst_end(f); iI != iE; ++iI) {
	if (ReturnInst::classof(&(*iI))) {
	  Instruction *InsertPoint = (&(*iI));
	  IRBuilder<> Builder(InsertPoint);
	  Builder.CreateCall(tracer_cleanup);
	}
      }
    }


    bool isMain(Function &func) {
      return (
	      (func.getName().str() == MAIN_STR) ||
	      (
	       (func.getName().str().find("cpython") != std::string::npos) &&
	       (func.getName().str().find("tile_launcher") != std::string::npos)
	       )
	      ); // Needs to be equality here!
    }
    
    bool runOnModule(Module &M) override {
      mod = &(M);

      //std::string k1 = "_Z12printuBranchPcS_";
      std::string k1 = "printuBranch";
      //std::string k2 = "_Z11printBranchPciS_S_";
      std::string k2 = "printBranch";
      //std::string k3 = "_Z7printSwPciS_iz";
      std::string k3 = "printSw";
      //std::string k4 = "_Z8printMemPcbxi";
      std::string k4 = "printMem";

      auto decades_kernel_type = getenv("DECADES_KERNEL_TYPE");
      if (decades_kernel_type) {
        KERNEL_TYPE = decades_kernel_type;
      }

      auto decades_run_dir = getenv("DECADES_RUN_DIR");
      if (decades_run_dir) {
        RUN_DIR = decades_run_dir;
      }


      LLVMContext& ctx = M.getContext();
      printuBR = (Function *) M.getOrInsertFunction(k1, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx)).getCallee();
      
      printBR = (Function *) M.getOrInsertFunction(k2, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx)).getCallee();
      
      std::vector<Type*> targs;
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt32Ty(ctx));
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt32Ty(ctx));
      FunctionType *sF = FunctionType::get(Type::getVoidTy(ctx), targs, true);
      printSw  = (Function *) M.getOrInsertFunction(k3, sF).getCallee();
      printMem = (Function *) M.getOrInsertFunction(k4, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt1Ty(ctx), Type::getInt64Ty(ctx), Type::getInt32Ty(ctx)).getCallee();

      print_matmul = (Function *) M.getOrInsertFunction("print_matmul", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)).getCallee();

      print_sdp = (Function *) M.getOrInsertFunction("print_sdp", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)).getCallee();
      
      print_conv2d_layer = (Function *) M.getOrInsertFunction("print_conv2d_layer", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt1Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt1Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)).getCallee();

      print_dense_layer = (Function *) M.getOrInsertFunction("print_dense_layer", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), Type::getInt32Ty(ctx)).getCallee();

      tracer_cleanup = (Function *) M.getOrInsertFunction("tracer_cleanup",
						   FunctionType::get(Type::getVoidTy(M.getContext()), false)).getCallee();
      
      auto decades_kernel_str = getenv("DECADES_KERNEL_STR");
      if (decades_kernel_str) {
	KERNEL_STR = decades_kernel_str;
      }
      auto decades_from_numba = getenv("DECADES_FROM_NUMBA");
      if (decades_from_numba) {
	if (atoi(decades_from_numba) == 1)
	  FROM_NUMBA = true;
      }



      for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
        if (isFoI(*fI)) {
          assignID(*fI);
          //errs() << fI->getName() << "\n";
          instInspect(*fI);
        }
      }

      for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
	//errs() << "[" << fI->getName().str() << "]\n";
	if (isMain(*fI)) {
	  //errs() << "[Found Main]\n";	  
	  //errs() << "[" << fI->getName().str() << "]\n";
	  instrument_main(*fI);
	}
      }


      
      return false;
    }
    void assignID(Function &F) {
      int bb_id = 0;
      int id = 0;
      for (BasicBlock &B : F)  {
        bb_id_table.insert(std::make_pair(&B, bb_id));
        bb_id++;
        for(Instruction &I : B) {
          id_table.insert(std::make_pair(&I, id));
          id++;
        }
      }
    }
    int findBID(BasicBlock *bb)
    {
      return bb_id_table.at(bb);
    }
    int findID(Instruction *ins)
    {
      return id_table.at(ins);
    }
    
    bool isFoI(Function &func) {
      if (!FROM_NUMBA) {
	return (func.getName().str().find(KERNEL_STR) != std::string::npos);
      }
      else {
	return (func.getName().str().find(KERNEL_STR) != std::string::npos) &&
	  (func.getName().str().find("cpython") == std::string::npos) ;
      }
    }
    
    void printInvoke(InvokeInst *pi) {
      IRBuilder<> Builder(pi);
      LLVMContext& ctx = mod->getContext();
      BasicBlock *bb = pi->getNormalDest();
      std::string temp= std::to_string(findBID(bb));
      StringRef p = temp;
      //errs() << "[invoke] " << *pi << " - " << p << "\n";
      std::string bbname = std::to_string(findBID(pi->getParent()));
      Value *name = Builder.CreateGlobalStringPtr(bbname);
      Value *name1= Builder.CreateGlobalStringPtr(p);
      Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
      Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
      Value* args[] = {name, ktype, rdir, name1};
      CallInst* cI = Builder.CreateCall(printuBR, args);
    }
    void printBranch(BranchInst *pi, int conditional) {
      IRBuilder<> Builder(pi);
      LLVMContext& ctx = mod->getContext();
      if(conditional) {
        std::string temp1, temp2;
        Value *v = pi->getCondition();
        if(v->getType()->isIntegerTy()) {
          BasicBlock *b1 = pi->getSuccessor(1);
          BasicBlock *b2 = pi->getSuccessor(0);
          temp1 = std::to_string(findBID(b1));
          temp2 = std::to_string(findBID(b2));
        }
        else
          assert(false);
        StringRef p1, p2;
        Value *castI;
        p1 = temp1;
        p2 = temp2;
        //errs() << "[Branch] " << *pi << " - " << p1 << " / " << p2 << "\n";
        castI = Builder.CreateZExt(v, Type::getInt32Ty(ctx), "castInst");
        std::string bbname = std::to_string(findBID(pi->getParent()));
        Value *name = Builder.CreateGlobalStringPtr(bbname);
        Value *name1= Builder.CreateGlobalStringPtr(p1);
        Value *name2= Builder.CreateGlobalStringPtr(p2);
	Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
	Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
        Value* args[] = {name, ktype, rdir, castI, name1, name2};
        CallInst* cI = Builder.CreateCall(printBR, args);
      }
      else {
        BasicBlock *bb = pi->getSuccessor(0);
        std::string temp= std::to_string(findBID(bb));
        StringRef p = temp;
        //errs() << "[uBranch] " << *pi << " - " << p << "\n";
        std::string bbname = std::to_string(findBID(pi->getParent()));
        Value *name = Builder.CreateGlobalStringPtr(bbname);
        Value *name1= Builder.CreateGlobalStringPtr(p);
	Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
	Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
        Value* args[] = {name, ktype, rdir, name1};
        CallInst* cI = Builder.CreateCall(printuBR, args);
      }
    }
    void printSwitch(SwitchInst *pi) {
      IRBuilder<> Builder(pi);
      LLVMContext& ctx = mod->getContext();
      Value *v = pi->getCondition();
      if(!v->getType()->isIntegerTy()) {
        assert(false);
      }
      //errs() << "[Switch] " << pi->getNumCases() << " : " << *pi << "\n";
      Value *castI;
      if (v->getType()->getIntegerBitWidth() < 32) {
	castI = Builder.CreateZExt(v, Type::getInt32Ty(ctx), "castInst");
      }
      else if (v->getType()->getIntegerBitWidth() > 32) {
	castI = Builder.CreateTrunc(v, Type::getInt32Ty(ctx), "castInst");
      }
      else {
	castI = v;
      }     
      
      std::vector<Value*> args;
      int len = pi->getNumCases();
      ConstantInt *length = llvm::ConstantInt::get(ctx, llvm::APInt(32, len , false));
      std::string namestr = std::to_string(findBID(pi->getParent()));//pi->getParent()->getName().str() + "/" + std::to_string(id);
      Value *name = Builder.CreateGlobalStringPtr(namestr);
      BasicBlock *bdefault = dyn_cast<BasicBlock>(pi->getOperand(1));
      std::string defstr = std::to_string(findBID(bdefault));
      Value *def = Builder.CreateGlobalStringPtr(defstr);
      Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
      Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
      args.push_back(name);
      args.push_back(ktype);
      args.push_back(rdir);
      args.push_back(castI);
      args.push_back(def);
      args.push_back(length);
      for(int i=0; i<pi->getNumCases(); i++) {
        BasicBlock *bop = dyn_cast<BasicBlock>(pi->getOperand(3+i*2));
        std::string bbstr = std::to_string(findBID(bop));
        errs() << "BB : " << bbstr << "\n";
        Value *n = Builder.CreateGlobalStringPtr(bbstr);
        args.push_back(pi->getOperand(2+i*2));
        args.push_back(n);
      }
      CallInst* cI = Builder.CreateCall(printSw, args);
    }
    void printMemory(Instruction *pi) {
      IRBuilder<> Builder(pi);
      bool isLoad = false;
      LLVMContext& ctx = mod->getContext();
      DataLayout* dl = new DataLayout(mod);
      Value *v, *castI, *name;
      ConstantInt *ctype;
      if(auto *li = dyn_cast<LoadInst>(pi)) {
        isLoad = true;
        //errs() << "[Load] " << *pi << "\n";
        v = li->getPointerOperand();
        std::string namestr = std::to_string(findID(pi));//pi->getParent()->getName().str() + "/" + std::to_string(id);
        name = Builder.CreateGlobalStringPtr(namestr);
        ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 0, false));
      }
      else if(auto *si = dyn_cast<StoreInst>(pi)) {
        isLoad = false;
        //errs() << "[Store] " << *pi << "\n";
        v = si->getPointerOperand();
        std::string namestr = std::to_string(findID(pi));//pi->getParent()->getName().str() + "/" + std::to_string(id);
        name = Builder.CreateGlobalStringPtr(namestr);
        ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 1, false));
      }
      else
        assert(false);
      castI = Builder.CreatePtrToInt(v, llvm::Type::getInt64Ty(ctx), "castInst");
      PointerType* vPtrType = cast<PointerType>(v->getType());
      uint64_t storeSize = dl->getTypeStoreSize(vPtrType->getPointerElementType());
      ConstantInt *csize = llvm::ConstantInt::get(ctx, llvm::APInt(32, storeSize, true));
      Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
      Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
      Value* args[] = {name, ktype, rdir, ctype, castI, csize};
  
      CallInst* cI = Builder.CreateCall(printMem, args);
  
    }
    void instInspect(Function &F) {
      for (inst_iterator iI = inst_begin(F), iE = inst_end(F);iI != iE; iI++) {
        Instruction *inst = &(*iI);
        if (auto *bI = dyn_cast<BranchInst>(inst)) {
          if(bI->isConditional()) {
            printBranch(bI, 1);
          }
          else
            printBranch(bI, 0);
        }
        else if (auto *bI = dyn_cast<IndirectBrInst>(inst)) {
          assert(false);
        }
        else if (auto *bI = dyn_cast<SwitchInst>(inst)) {
          printSwitch(bI);
        }
        else if(auto *lI = dyn_cast<LoadInst>(inst)) {
          printMemory(inst);
        }
        //print out the memory traces
        else if(CallInst *cinst = dyn_cast<CallInst>(inst)) {
          for (Use &use : inst->operands()) {
            Value *v = use.get();
            if(Function *f = dyn_cast<Function>(v)) {
              //luwa here or all the atomic instructions
              if(f->getName().str().find("supply_consume") != std::string::npos || f->getName().str().find("DECADES_FETCH_ADD") != std::string::npos || f->getName().str().find("DECADES_COMPARE_AND_SWAP") != std::string::npos ||  f->getName().str().find("DECADES_FETCH_MIN") != std::string::npos || f->getName().str().find("DECADES_FETCH_ADD_FLOAT") != std::string::npos) {
               
                //errs() << "[STADDR]"<< *inst << "\n";
                //LLVMContext& ctx = mod->getContext();
                //auto arg_it=f->arg_begin();
                //Value* addr=cinst->getArgOperand(0);
                Value* v, *castI, *name;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();
                v=(cinst->getArgOperand(0));
                
                castI = Builder.CreatePtrToInt(v, llvm::Type::getInt64Ty(ctx), "castInst");
                ConstantInt* ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 1, false));
                std::string namestr = std::to_string(findID(inst));
                name = Builder.CreateGlobalStringPtr(namestr);
                PointerType* vPtrType = cast<PointerType>(v->getType());
                uint64_t storeSize = dl->getTypeStoreSize(vPtrType->getPointerElementType());
                ConstantInt *csize = llvm::ConstantInt::get(ctx, llvm::APInt(32, storeSize, true));
		Value* ktype = Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
                Value* args[] = {name, ktype, rdir, ctype, castI, csize};
 
                CallInst* cI = Builder.CreateCall(printMem, args);
                //for(int i=0; i<1000; i++) {
                //  CallInst* cI = Builder.CreateCall(printMem, args);
                //}
                //assert(false);
                //errs() << "STADDR Addr: " << *(cinst->getArgOperand(0)) << "\n";
                
              }
              
              else if(f->getName().str().find("supply_load_produce") != std::string::npos) {
                
                //errs() << "[STADDR]"<< *inst << "\n";
                //LLVMContext& ctx = mod->getContext();
                //auto arg_it=f->arg_begin();
                //Value* addr=cinst->getArgOperand(0);
                Value* v, *castI, *name;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();
                v=(cinst->getArgOperand(0));
                
                castI = Builder.CreatePtrToInt(v, llvm::Type::getInt64Ty(ctx), "castInst");
                ConstantInt* ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 0, false));
                std::string namestr = std::to_string(findID(inst));
                name = Builder.CreateGlobalStringPtr(namestr);
                PointerType* vPtrType = cast<PointerType>(v->getType());
                uint64_t storeSize = dl->getTypeStoreSize(vPtrType->getPointerElementType());
                ConstantInt *csize = llvm::ConstantInt::get(ctx, llvm::APInt(32, storeSize, true));
		Value *ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		Value *rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
                Value* args[] = {name, ktype, rdir, ctype, castI, csize};
 
                CallInst* cI = Builder.CreateCall(printMem, args);
                //for(int i=0; i<1000; i++) {
                //  CallInst* cI = Builder.CreateCall(printMem, args);
                //}
                //assert(false);
                //errs() << "STADDR Addr: " << *(cinst->getArgOperand(0)) << "\n";
                
              }
              //extract accelerators below
              /*from TF api 
                void decadesTF_matmul(
                volatile int rowsA, volatile int colsA, volatile int rowsB, volatile int colsB, void int batch, volatile int a2,
                float *A, float *B, float *out,
                int tid, int num_threads)
              */
              else if(f->getName().str().find("decadesTF_matmul") != std::string::npos) {
                
         
                Value *ktype, *rdir, *rowsa_val, *colsa_val, *rowsb_val, *colsb_val, *batch_val, *castI, *name_val, *acc_name_val;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();
                rowsa_val=(cinst->getArgOperand(0));
                colsa_val=(cinst->getArgOperand(1));                
                rowsb_val=(cinst->getArgOperand(2));
                colsb_val=(cinst->getArgOperand(3));
                batch_val=(cinst->getArgOperand(4));
                ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
 

                castI = Builder.CreatePtrToInt(colsa_val, llvm::Type::getInt32Ty(ctx), "castInst");
                
                
                
                std::string namestr = std::to_string(findID(inst));
                std::string acc_name("decadesTF_matmul");
                //instr id
                name_val = Builder.CreateGlobalStringPtr(namestr);
                acc_name_val = Builder.CreateGlobalStringPtr(acc_name);
                Value* args[] = {name_val,ktype,rdir,acc_name_val,rowsa_val,colsa_val,rowsb_val, colsb_val, batch_val};
                CallInst* cI = Builder.CreateCall(print_matmul, args);
                
                //for(int i=0; i<1000; i++) {
                //  CallInst* cI = Builder.CreateCall(printMem, args);
                //}
                //assert(false);
                //errs() << "STADDR Addr: " << *(cinst->getArgOperand(0)) << "\n";
                
              }

              /*
                void decadesTF_sdp(
                volatile int working_mode, volatile int size, volatile int a1, 
                volatile int a2, volatile int a3, volatile int a4,
                float *A, float *B, float *out,
                int tid, int num_threads)
              */
              else if(f->getName().str().find("decadesTF_sdp") != std::string::npos) {
                
                Value *ktype, *rdir, *working_mode, *size, *name_val, *acc_name_val;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();
                working_mode=(cinst->getArgOperand(0));
                size=(cinst->getArgOperand(1));
                ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
                                
                std::string namestr = std::to_string(findID(inst));
                std::string acc_name("decadesTF_sdp");
                //instr id
                name_val = Builder.CreateGlobalStringPtr(namestr);
                acc_name_val = Builder.CreateGlobalStringPtr(acc_name);
                Value* args[] = {name_val,ktype,rdir,acc_name_val,working_mode, size};
                CallInst* cI = Builder.CreateCall(print_sdp, args);
              }

              /*
void decadesTF_conv2d_layer(
    0 volatile int batch, 1 volatile int in_height,
    2 volatile int in_width, 3 volatile int in_channels,
    4 volatile int filter_height, 5 volatile int filter_width,
    6 volatile int out_channels, 7 volatile bool zero_pad,
    8 volatile int vert_conv_stride, 9 volatile int horiz_conv_stride,
    10 volatile bool pooling, 11 volatile int pool_height, 12 volatile int pool_width,
    13 volatile int vertical_pool_stride, 14 volatile int horizontal_pool_stride,
    15 float *in, 16 float *filters,  17 float *out,
    // software-only parameters
    float *bias, bool bias_add, bool activation, int activation_type,
    float *prelu_filters, int pooling_type, bool lrn, int lrn_radius,
    int lrn_bias, int lrn_alpha, int lrn_beta, int tid, int num_threads)

void decadesTF_conv2d_layer(
    0 volatile int batch, 1 volatile int in_height,
    2 volatile int in_width, 3 volatile int in_channels,
    4 volatile int filter_height, 5 volatile int filter_width, 6 volatile int out_channels, 
    7 volatile bool zero_pad, 8 volatile int vert_conv_stride, 9 volatile int horiz_conv_stride,
    10 volatile bool bias_add, 
    11 volatile bool activation, 12 volatile int activation_type,
    13 volatile bool pooling, 14 volatile int pooling_type, 15 volatile int pool_height, 16 volatile int pool_width,
    17 volatile bool lrn, 18 volatile int lrn_radius, 19 volatile int lrn_bias, 20 volatile int lrn_alpha, 21 volatile int lrn_beta, 
    22 float *in, float *filters, float *bias, float *prelu_filters, float *out,
    int tid, int num_threads)

// mapping: C++ -> model 
in_channels -> num_of_inputs
in_height -> input_height
in_width -> input_width
out_channels -> num_of_outputs
filter_height -> filter_height
filter_width -> filter_width
zero_pad -> zero_pad
vert_conv_stride -> vertical_conv_dim
horiz_conv_stride -> horizontal_conv_dim
pooling -> pooling
pool_height -> pool_height
pool_width -> pool_width
vertical_pool_stride -> vertical_pool_dim
horizontal_pool_stride -> horizontal_pool_dim
batch -> batch_size
              */
              else if(f->getName().str().find("decadesTF_conv2d_layer") != std::string::npos) {
                
                Value *ktype, *rdir, *name_val, *acc_name_val, *batch, *in_channels, *in_height, *in_width, *out_channels, *filter_height, *filter_width, *zero_pad, *vert_conv_stride, *horiz_conv_stride, *pooling, *pool_height, *pool_width, *vertical_pool_stride, *horizontal_pool_stride;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();

                batch=(cinst->getArgOperand(0));in_channels=(cinst->getArgOperand(3)); in_height=(cinst->getArgOperand(1));in_width=(cinst->getArgOperand(2));out_channels=(cinst->getArgOperand(6));filter_height=(cinst->getArgOperand(4));filter_width=(cinst->getArgOperand(5));zero_pad=(cinst->getArgOperand(7));vert_conv_stride=(cinst->getArgOperand(8));horiz_conv_stride=(cinst->getArgOperand(9));pooling=(cinst->getArgOperand(13));pool_height=(cinst->getArgOperand(15));pool_width=(cinst->getArgOperand(16));vertical_pool_stride=(cinst->getArgOperand(17));horizontal_pool_stride=(cinst->getArgOperand(18));
                                
                // previous version
                //batch=(cinst->getArgOperand(0));in_channels=(cinst->getArgOperand(3)); in_height=(cinst->getArgOperand(1));in_width=(cinst->getArgOperand(2));out_channels=(cinst->getArgOperand(6));filter_height=(cinst->getArgOperand(4));filter_width=(cinst->getArgOperand(5));zero_pad=(cinst->getArgOperand(7));vert_conv_stride=(cinst->getArgOperand(8));horiz_conv_stride=(cinst->getArgOperand(9));pooling=(cinst->getArgOperand(10));pool_height=(cinst->getArgOperand(11));pool_width=(cinst->getArgOperand(12));vertical_pool_stride=(cinst->getArgOperand(13));horizontal_pool_stride=(cinst->getArgOperand(14));


                
                ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
                                
                std::string namestr = std::to_string(findID(inst));
                std::string acc_name("decadesTF_conv2d_layer");
                //instr id
                name_val = Builder.CreateGlobalStringPtr(namestr);
                acc_name_val = Builder.CreateGlobalStringPtr(acc_name);
                Value* args[] = {name_val,ktype,rdir,acc_name_val, batch,in_channels, in_height, in_width, out_channels, filter_height, filter_width, zero_pad, vert_conv_stride, horiz_conv_stride, pooling, pool_height, pool_width, vertical_pool_stride, horizontal_pool_stride};
                CallInst* cI = Builder.CreateCall(print_conv2d_layer, args);                           
              }
              /*
              // mapping: C++ -> model
              in_channels -> num_of_inputs
                1 -> input_height
                1 -> input_width
                out_channels -> num_of_outputs
                1 -> filter_height
                1 -> filter_width
                0 -> zero_pad
                1 -> vertical_conv_dim
                1 -> horizontal_conv_dim
                0 -> pooling
                1 -> pool_height
                1 -> pool_width
                1 -> vertical_pool_dim
                1 -> horizontal_pool_dim
                0 -> type // here 0 is for dense, aka fc (fully connected),
                          // there's an enum in nvlda.h: enum layer_type{fc, conv};
                batch -> batch_size
                */
              /*
              void decadesTF_dense_layer(
              volatile int batch, volatile int in_channels, volatile int out_channels,
              float *in, float *filters, float *out,
              // software-only parameters
              bool bias_add, float *bias,  bool activation, int activation_type,
              float *prelu_filters, int tid, int num_threads)
              */
              else if(f->getName().str().find("decadesTF_dense_layer") != std::string::npos) {
                
                Value *ktype, *rdir, *batch, *in_channels, *out_channels, *name_val, *acc_name_val;
                IRBuilder<> Builder(inst);
                DataLayout* dl = new DataLayout(mod);
                LLVMContext& ctx = mod->getContext();

                batch=(cinst->getArgOperand(0));
                in_channels=(cinst->getArgOperand(1));
                out_channels=(cinst->getArgOperand(2));
                ktype= Builder.CreateGlobalStringPtr(KERNEL_TYPE);
		rdir = Builder.CreateGlobalStringPtr(RUN_DIR);
                                
                std::string namestr = std::to_string(findID(inst));
                std::string acc_name("decadesTF_dense_layer");
                //instr id
                name_val = Builder.CreateGlobalStringPtr(namestr);
                acc_name_val = Builder.CreateGlobalStringPtr(acc_name);
                Value* args[] = {name_val,ktype,rdir,acc_name_val, batch, in_channels, out_channels};
                CallInst* cI = Builder.CreateCall(print_dense_layer, args);
              }
              
            }
          }
        }
        
        else if(auto *sI = dyn_cast<StoreInst>(inst)) {
          //luwa delete
          
            printMemory(inst);
          
        }
        else if(auto *iI = dyn_cast<InvokeInst>(inst))
          printInvoke(iI);
        else if(inst->isTerminator()) {
          //errs() << "[WARNING] Terminator Instruction not handled : " << *inst <<"\n";
	}
	
      }
    }
  };
}
char RecordDynamicInfo::ID = 0;
static RegisterPass<RecordDynamicInfo> X("recorddynamicinfo", "Record Dynamic Information for Oracle",
                                         false /* Only looks at CFG */,
                                         false /* Analysis Pass */);
