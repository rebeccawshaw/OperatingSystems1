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
CMAKE_SOURCE_DIR = /home/student/cs281/shawr1/assignment1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/student/cs281/shawr1/assignment1

# Include any dependencies generated for this target.
include CMakeFiles/mergesort.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mergesort.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mergesort.dir/flags.make

CMakeFiles/mergesort.dir/mergesort.cxx.o: CMakeFiles/mergesort.dir/flags.make
CMakeFiles/mergesort.dir/mergesort.cxx.o: mergesort.cxx
	$(CMAKE_COMMAND) -E cmake_progress_report /home/student/cs281/shawr1/assignment1/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/mergesort.dir/mergesort.cxx.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/mergesort.dir/mergesort.cxx.o -c /home/student/cs281/shawr1/assignment1/mergesort.cxx

CMakeFiles/mergesort.dir/mergesort.cxx.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mergesort.dir/mergesort.cxx.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/student/cs281/shawr1/assignment1/mergesort.cxx > CMakeFiles/mergesort.dir/mergesort.cxx.i

CMakeFiles/mergesort.dir/mergesort.cxx.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mergesort.dir/mergesort.cxx.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/student/cs281/shawr1/assignment1/mergesort.cxx -o CMakeFiles/mergesort.dir/mergesort.cxx.s

CMakeFiles/mergesort.dir/mergesort.cxx.o.requires:
.PHONY : CMakeFiles/mergesort.dir/mergesort.cxx.o.requires

CMakeFiles/mergesort.dir/mergesort.cxx.o.provides: CMakeFiles/mergesort.dir/mergesort.cxx.o.requires
	$(MAKE) -f CMakeFiles/mergesort.dir/build.make CMakeFiles/mergesort.dir/mergesort.cxx.o.provides.build
.PHONY : CMakeFiles/mergesort.dir/mergesort.cxx.o.provides

CMakeFiles/mergesort.dir/mergesort.cxx.o.provides.build: CMakeFiles/mergesort.dir/mergesort.cxx.o

# Object files for target mergesort
mergesort_OBJECTS = \
"CMakeFiles/mergesort.dir/mergesort.cxx.o"

# External object files for target mergesort
mergesort_EXTERNAL_OBJECTS =

mergesort: CMakeFiles/mergesort.dir/mergesort.cxx.o
mergesort: CMakeFiles/mergesort.dir/build.make
mergesort: CMakeFiles/mergesort.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable mergesort"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mergesort.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mergesort.dir/build: mergesort
.PHONY : CMakeFiles/mergesort.dir/build

CMakeFiles/mergesort.dir/requires: CMakeFiles/mergesort.dir/mergesort.cxx.o.requires
.PHONY : CMakeFiles/mergesort.dir/requires

CMakeFiles/mergesort.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mergesort.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mergesort.dir/clean

CMakeFiles/mergesort.dir/depend:
	cd /home/student/cs281/shawr1/assignment1 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/student/cs281/shawr1/assignment1 /home/student/cs281/shawr1/assignment1 /home/student/cs281/shawr1/assignment1 /home/student/cs281/shawr1/assignment1 /home/student/cs281/shawr1/assignment1/CMakeFiles/mergesort.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mergesort.dir/depend

