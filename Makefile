ROOT   = .
VM_ROOT = .
USER_OBJ = \
   $(OBJ)/jit$(OBJ_SUFF) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB)
