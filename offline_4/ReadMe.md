## Implementing the Paging Mechanism of Memory Management in xv6.

[ This assignment was done in a very small amount of time, so the implementation is not perfect. Anyone following this implementation is suggested not to follow everything here so strictly, rather develop the baseline understanding of how paging works in xv6 and develop the mechanisms with gathered intution. ]

### User Functions
	- createp
		creates a process. this process runs forever. useful to check page directory and page table entries for a new process. also to test fork
	
	- ^P (ctrl + P)
		prints the page directory (address and contents) and the page table entries under them
	
	- test
		creates a new process with dynamically allocating memory using sbrk. the argument in sbrk can be varied to create process with different amount of memory
