# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_SOURCE_DIR = /home/num/projects/lms_borrow_action/project

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/num/projects/lms_borrow_action/build

# Include any dependencies generated for this target.
include CMakeFiles/lms_borrower_action.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/lms_borrower_action.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lms_borrower_action.dir/flags.make

CMakeFiles/lms_borrower_action.dir/main.cc.o: CMakeFiles/lms_borrower_action.dir/flags.make
CMakeFiles/lms_borrower_action.dir/main.cc.o: /home/num/projects/lms_borrow_action/project/main.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/num/projects/lms_borrow_action/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/lms_borrower_action.dir/main.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/lms_borrower_action.dir/main.cc.o -c /home/num/projects/lms_borrow_action/project/main.cc

CMakeFiles/lms_borrower_action.dir/main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lms_borrower_action.dir/main.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/num/projects/lms_borrow_action/project/main.cc > CMakeFiles/lms_borrower_action.dir/main.cc.i

CMakeFiles/lms_borrower_action.dir/main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lms_borrower_action.dir/main.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/num/projects/lms_borrow_action/project/main.cc -o CMakeFiles/lms_borrower_action.dir/main.cc.s

# Object files for target lms_borrower_action
lms_borrower_action_OBJECTS = \
"CMakeFiles/lms_borrower_action.dir/main.cc.o"

# External object files for target lms_borrower_action
lms_borrower_action_EXTERNAL_OBJECTS =

lms_borrower_action: CMakeFiles/lms_borrower_action.dir/main.cc.o
lms_borrower_action: CMakeFiles/lms_borrower_action.dir/build.make
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/debug/lib/libfcgi++.a
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/debug/lib/libfcgi.a
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/lib/libfcgi++.a
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/lib/libfcgi.a
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/debug/lib/libssl.a
lms_borrower_action: /home/num/vcpkg/installed/x64-linux/debug/lib/libcrypto.a
lms_borrower_action: CMakeFiles/lms_borrower_action.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/num/projects/lms_borrow_action/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable lms_borrower_action"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lms_borrower_action.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lms_borrower_action.dir/build: lms_borrower_action

.PHONY : CMakeFiles/lms_borrower_action.dir/build

CMakeFiles/lms_borrower_action.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lms_borrower_action.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lms_borrower_action.dir/clean

CMakeFiles/lms_borrower_action.dir/depend:
	cd /home/num/projects/lms_borrow_action/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/num/projects/lms_borrow_action/project /home/num/projects/lms_borrow_action/project /home/num/projects/lms_borrow_action/build /home/num/projects/lms_borrow_action/build /home/num/projects/lms_borrow_action/build/CMakeFiles/lms_borrower_action.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/lms_borrower_action.dir/depend
