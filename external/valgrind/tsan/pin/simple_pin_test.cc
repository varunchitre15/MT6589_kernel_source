// Simple test for PIN.
// Prints the number of memory accesses.
// Run: $PIN_ROOT/pin -t `pwd`/simple_pin_test.so -- your_program
#include "pin.H"

#include <map>

// statistics
static long long dynamic_memory_access_count;
static int static_memory_access_count;

//---------- Instrumentation functions ---------
void InsertBeforeEvent_MemoryAccess(ADDRINT pc) {
  dynamic_memory_access_count++;
}

//-------------- PIN callbacks ---------------
void CallbackForTRACE(TRACE trace, void *v) {
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
      if (INS_IsStackRead(ins) || INS_IsStackWrite(ins))
        continue;
      if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins)) {
        static_memory_access_count++;
        INS_InsertCall(ins, IPOINT_BEFORE,
                         (AFUNPTR)InsertBeforeEvent_MemoryAccess,
                         IARG_INST_PTR, IARG_END);
      }
    }
  }
}

static void CallbackForFini(INT32 code, void *v) {
  printf("accesses static  : %d\n", static_memory_access_count);
  printf("accesses dynamic : %lld\n", dynamic_memory_access_count);
}

//---------------- main ---------------
int main(INT32 argc, CHAR **argv) {
  PIN_Init(argc, argv);
  PIN_InitSymbols();
  PIN_AddFiniFunction(CallbackForFini, 0);
  TRACE_AddInstrumentFunction(CallbackForTRACE, 0);
  PIN_StartProgram();
  printf("accesses static  : %d\n", static_memory_access_count);
  return 0;
}
