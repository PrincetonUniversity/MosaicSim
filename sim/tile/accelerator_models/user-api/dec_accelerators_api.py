#
# Sample API for DECADES accelerators
#

# import libraries
import sys
import numpy as np

def dec_error(class_name, message):
    dec_error_string = "[DEC ERROR] "
    print(dec_error_string + class_name + ": " + message)
    

# Python API layer for GEMM accelerator
# TODO add activation capabilities
class gemm_acc:

    def configuration(self, inA, inB, out):
        (self.rowsA, self.colsA, self.batchA) = inA.shape
        (self.rowsB, self.colsB, self.batchB) = inB.shape
        (self.rowsOut, self.colsOut, self.batchOut) = out.shape
        self.inA_ndim = inA.ndim
        self.inB_ndim = inB.ndim
        self.out_ndim = out.ndim


    def compliance_check(self):
        error = False

        if not (self.inA_ndim == self.inB_ndim == self.out_ndim):
            error = True
            dec_error(self.__class__.__name__, \
                      "number of dimensions of inputs and output differ.")
            return error

        if self.inA_ndim == 3:
            if not (self.batchA == self.batchB == self.batchOut):
                error = True
                dec_error(self.__class__.__name__, \
                          "batch size of inputs and output differ.")

        if not (self.colsA == self.rowsB):
            error = True
            dec_error(self.__class__.__name__, \
                      "cols of matrix A differ from rows of matrix B.")

        if not (self.rowsA == self.rowsOut):
            error = True
            dec_error(self.__class__.__name__, \
                      "rows of matrix A differ from rows of output.")

        if not (self.colsB == self.colsOut):
            error = True
            dec_error(self.__class__.__name__, \
                      "cols of matrix B differ from rows of output.")

        return error


    def invoke(self, inA, inB, out):

        # save configuration
        self.configuration(inA, inB, out)

        # check compliance of arguments
        if not (self.compliance_check() == 0):
            dec_error(self.__class__.__name__, \
                      "parameters not compliant, see DECADES documentation")
            sys.exit()
            return

        # functionally correct computation
        out = np.matmul(inA, inB)

        # simulator api call
        # sim_gemm(inA, inB, out, self.rowsA, self.colsA, self.colsB, self.batchA)

# Python API layer for CONV-2D accelerator
# TODO add activation and pooling capabilities
class conv2d_acc:
    def _init_(self):
        self.dummy = 0

# Python API layer for element-wise arithmetic accelerator
# SDP: single data-point engine
class sdp_acc:
    def _init_(self):
        self.dummy = 0

# Python API layer for FFT accelerator
class fft_acc:
    def _init_(self):
        self.dummy = 0


# Python API layer for sparse GEMM accelerator
# TODO add activation capabilities
class spgemm_acc:
    def _init_(self):
        self.dummy = 0
