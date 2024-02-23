# DiskBenchmark
Basic tool to test disk benchmark

# Usage
Options:\
&emsp;-h,--help&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp;Print this help message and exit\
&emsp;-s,--seconds INT&emsp;&emsp;&emsp;&emsp;&emsp;Duration of test in seconds (optional)\
&emsp;-i,--io_type TEXT&emsp;&emsp;&emsp;&emsp;&emsp;I/O test type (r -> read, w -> write, rw -> read/write)\
&emsp;-r,--random&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;Random read/write\
&emsp;-t,--thread INT&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&nbsp;Number of thread to use for the test\
&emsp;-o,--task INT&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;Number of I/O operation per thread\
&emsp;-u,--unaligned&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;Different starting offsets for each thread\
&emsp;-n,--file_name TEXT&emsp;&emsp;&emsp;&ensp;Name of the file to use for test\
&emsp;-z,--file_size INT&emsp;&emsp;&emsp;&emsp;&emsp;Size of the file to use for test (in Mb)\
&emsp;-b,--block_size INT&emsp;&emsp;&emsp;&ensp;&nbsp;Size of the block to read/write (in Kb)\
&emsp;-l,--log INT&emsp;&emsp;&emsp;&ensp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Show log messages