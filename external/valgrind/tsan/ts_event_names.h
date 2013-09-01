static const char *kEventNames[] =  {
  "NOOP",               // Should not appear.
  "READ",               // {tid, pc, addr, size}
  "WRITE",              // {tid, pc, addr, size}
  "READER_LOCK",        // {tid, pc, lock, 0}
  "WRITER_LOCK",        // {tid, pc, lock, 0}
  "UNLOCK",             // {tid, pc, lock, 0}
  "UNLOCK_OR_INIT",     // {tid, pc, lock, 0}
  "LOCK_CREATE",        // {tid, pc, lock, 0}
  "LOCK_DESTROY",       // {tid, pc, lock, 0}
  "THR_CREATE_BEFORE",  // Parent thread's event. {tid, pc, 0, 0}
  "THR_CREATE_AFTER",   // Parent thread's event. {tid, 0, 0, child_tid}
  "THR_START",          // Child thread's event {tid, CallStack, 0, parent_tid}
  "THR_FIRST_INSN",     // Used only by valgrind.
  "THR_END",            // {tid, 0, 0, 0}
  "THR_JOIN_AFTER",     // {tid, pc, joined_tid}
  "THR_STACK_TOP",      // {tid, pc, stack_top, stack_size_if_known}
  "RTN_EXIT",           // {tid, 0, 0, 0}
  "RTN_CALL",           // {tid, pc, 0, 0}
  "SBLOCK_ENTER",       // {tid, pc, 0, 0}
  "SIGNAL",             // {tid, pc, obj, 0}
  "WAIT",               // {tid, pc, obj, 0}
  "CYCLIC_BARRIER_INIT",         // {tid, pc, obj, n}
  "CYCLIC_BARRIER_WAIT_BEFORE",  // {tid, pc, obj, 0}
  "CYCLIC_BARRIER_WAIT_AFTER",   // {tid, pc, obj, 0}
  "PCQ_CREATE",         // {tid, pc, pcq_addr, 0}
  "PCQ_DESTROY",        // {tid, pc, pcq_addr, 0}
  "PCQ_PUT",            // {tid, pc, pcq_addr, 0}
  "PCQ_GET",            // {tid, pc, pcq_addr, 0}
  "STACK_MEM_DIE",      // deprecated.
  "MALLOC",             // {tid, pc, addr, size}
  "FREE",               // {tid, pc, addr, 0}
  "MMAP",               // {tid, pc, addr, size}
  "MUNMAP",             // {tid, pc, addr, size}
  "PUBLISH_RANGE",      // may be deprecated later.
  "UNPUBLISH_RANGE",    // deprecated. TODO(kcc): get rid of this.
  "HB_LOCK",            // {tid, pc, addr, 0}
  "NON_HB_LOCK",        // {tid, pc, addr, 0}
  "IGNORE_READS_BEG",   // {tid, pc, 0, 0}
  "IGNORE_READS_END",   // {tid, pc, 0, 0}
  "IGNORE_WRITES_BEG",  // {tid, pc, 0, 0}
  "IGNORE_WRITES_END",  // {tid, pc, 0, 0}
  "SET_THREAD_NAME",    // {tid, pc, name_str, 0}
  "SET_LOCK_NAME",      // {tid, pc, lock, lock_name_str}
  "TRACE_MEM",          // {tid, pc, addr, 0}
  "EXPECT_RACE",        // {tid, descr_str, ptr, size}
  "BENIGN_RACE",        // {tid, descr_str, ptr, size}
  "EXPECT_RACE_BEGIN",  // {tid, pc, 0, 0}
  "EXPECT_RACE_END",    // {tid, pc, 0, 0}
  "VERBOSITY",          // Used for debugging.
  "STACK_TRACE",        // {tid, pc, 0, 0}, for debugging.
  "FLUSH_STATE",        // {tid, pc, 0, 0}
  "PC_DESCRIPTION",     // {0, pc, descr_str, 0}, for ts_offline.
  "PRINT_MESSAGE",      // {tid, pc, message_str, 0}, for ts_offline.
  "FLUSH_EXPECTED_RACES",  // {0, 0, 0, 0}
  "LAST_EVENT"          // Should not appear.
};
