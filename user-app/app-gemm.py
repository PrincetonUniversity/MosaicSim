#
# Sample app to invoke the DECADES GEMM accelerator
#

# import libraries
import sys
import numpy as np
sys.path.insert(0,'../user-api')
import dec_accelerators_api as dec_acc

# instantiate accelerator
gemm_acc = dec_acc.gemm_acc()

# hard-coded configuration for the accelerator
rowsA = 100
colsA = 100
colsB = 100
batch_size = 100

# allocate and fill data structures for the accelerator
inA = np.random.rand(rowsA, colsA, batch_size)
inB = np.random.rand(colsA, colsB, batch_size)
out = np.random.rand(rowsA, colsB, batch_size)

# accelerator invocation
gemm_acc.invoke(inA, inB, out)
