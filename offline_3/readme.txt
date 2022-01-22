How to Run
-------------------

* clone the xv6 repo (if you haven't already)
	$ git clone git://github.com/mit-pdos/xv6-public.git

* apply the patch file 
	$ git apply <studentID>.patch
	
* run it
	$ make qemu-nox
	
	

In this assignment
----------------------------

* ps 
	- shows the current process statistics (pid, ticks, tickets)

* createp [tickets(optional)]
	- creates a new process with given tickets. (Tickets are priority, a higher number of tickets indicate higher priority)
	- if no arg is given, default ticket = 1
	
* settickets [pid, tickets]
	- sets tickets of the process of the given pid
	
* simlot [time]
	- simulates lottery scheduling in the given time
	- outputs a statistics about cpu usage of the processes in the given time
