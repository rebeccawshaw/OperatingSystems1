# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/project/KernelProject/relay

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/project/KernelProject/relay

# Include any dependencies generated for this target.
include CMakeFiles/relay.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/relay.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/relay.dir/flags.make

CMakeFiles/relay.dir/userspace.c.o: CMakeFiles/relay.dir/flags.make
CMakeFiles/relay.dir/userspace.c.o: userspace.c
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/project/KernelProject/relay/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/relay.dir/userspace.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/relay.dir/userspace.c.o   -c /mnt/project/KernelProject/relay/userspace.c

CMakeFiles/relay.dir/userspace.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/relay.dir/userspace.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /mnt/project/KernelProject/relay/userspace.c > CMakeFiles/relay.dir/userspace.c.i

CMakeFiles/relay.dir/userspace.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/relay.dir/userspace.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /mnt/project/KernelProject/relay/userspace.c -o CMakeFiles/relay.dir/userspace.c.s

CMakeFiles/relay.dir/userspace.c.o.requires:
.PHONY : CMakeFiles/relay.dir/userspace.c.o.requires

CMakeFiles/relay.dir/userspace.c.o.provides: CMakeFiles/relay.dir/userspace.c.o.requires
	$(MAKE) -f CMakeFiles/relay.dir/build.make CMakeFiles/relay.dir/userspace.c.o.provides.build
.PHONY : CMakeFiles/relay.dir/userspace.c.o.provides

CMakeFiles/relay.dir/userspace.c.o.provides.build: CMakeFiles/relay.dir/userspace.c.o

# Object files for target relay
relay_OBJECTS = \
"CMakeFiles/relay.dir/userspace.c.o"

# External object files for target relay
relay_EXTERNAL_OBJECTS =

relay: CMakeFiles/relay.dir/userspace.c.o
relay: CMakeFiles/relay.dir/build.make
relay: CMakeFiles/relay.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable relay"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/relay.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/relay.dir/build: relay
.PHONY : CMakeFiles/relay.dir/build

CMakeFiles/relay.dir/requires: CMakeFiles/relay.dir/userspace.c.o.requires
.PHONY : CMakeFiles/relay.dir/requires

CMakeFiles/relay.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/relay.dir/cmake_clean.cmake
.PHONY : CMakeFiles/relay.dir/clean

CMakeFiles/relay.dir/depend:
	cd /mnt/project/KernelProject/relay && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/project/KernelProject/relay /mnt/project/KernelProject/relay /mnt/project/KernelProject/relay /mnt/project/KernelProject/relay /mnt/project/KernelProject/relay/CMakeFiles/relay.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/relay.dir/depend

