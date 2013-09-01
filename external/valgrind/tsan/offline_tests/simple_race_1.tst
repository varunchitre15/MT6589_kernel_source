# Start thread T0.
THR_START 0 0 0 0


# Create two locks.
LOCK_CREATE 0 ff0 7777 0
LOCK_CREATE 0 ff1 7778 0


# Start thread T1
THR_START 1 0 0 0

# Call few functions in T0
RTN_CALL 0 ca000001 ca000002 0
RTN_CALL 0 ca000002 ca000003 0

# Call few functions in T1
RTN_CALL 1 ca100001 ca100002 0
RTN_CALL 1 ca100002 ca100003 0

# Allocate 0xff bytes of memory in T0
MALLOC 0 cdeffedc abcd0 ff

# Malloc some more (unrelated)
MALLOC 0 cdeffedc ccc ff
MALLOC 0 cdeffedc cccccccc ff

# Acquire lock 7777 in T0
WRITER_LOCK 0 aa 7777 0

# Write to 0xabcde in T0
SBLOCK_ENTER 0 ca000003 0 0
WRITE 0 aa008001 abcde 1

# Acquire reader lock 7778 in T1
READER_LOCK 1 bb 7778 0

##############
# Race here: #
##############
#
# Read 0xabcde in T1
READ 1 aa108001 abcde 1
