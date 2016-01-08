Team Members with their vunetid
	Jon Lee (leejk4), Rebecca Shaw (shawr1)

Project Description
	kernel
		add tracing to kernel memory allocator kmalloc()/kfree() functions using relayfs or similar mechanisms and write user-space tools for analyzing that information
	How you plan to test your solution
		use gemu, a machine emulator, inside of the class VM

Error Conditions that you will check
	each kmalloc() corresponds to one and only one kfree()

Milestones (use the milestones mentioned in the project presentation as guideline)
	march 24 - submit a report about the design
	april 9 - status update and submit a set of slides describing what you have done till now
	april 21 - final report

Expected Schedule/Timeline
	march 24 - complete yocto project
	march 31 - complete description of project and paragraphs of understandings
	april 7 - complete changes to kmalloc and kfree
	april 14 - complete user tools to analyze and track the kmalloc and kfree
	april 21 - finish project and presentation

Language and operating system used
	linux in c

Most Importantly, a list of questions that you have.
	office hours friday 3/20 at 1pm (at ISIS) with Dubey
	