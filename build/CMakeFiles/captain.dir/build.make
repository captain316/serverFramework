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


# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_SOURCE_DIR = /home/lk/gitRepo/serverFramework

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lk/gitRepo/serverFramework/build

# Include any dependencies generated for this target.
include CMakeFiles/captain.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/captain.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/captain.dir/flags.make

CMakeFiles/captain.dir/captain/config.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/config.cpp.o: ../captain/config.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/captain.dir/captain/config.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/config.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/config.cpp.o -c /home/lk/gitRepo/serverFramework/captain/config.cpp

CMakeFiles/captain.dir/captain/config.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/config.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/config.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/config.cpp > CMakeFiles/captain.dir/captain/config.cpp.i

CMakeFiles/captain.dir/captain/config.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/config.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/config.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/config.cpp -o CMakeFiles/captain.dir/captain/config.cpp.s

CMakeFiles/captain.dir/captain/fiber.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/fiber.cpp.o: ../captain/fiber.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/captain.dir/captain/fiber.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/fiber.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/fiber.cpp.o -c /home/lk/gitRepo/serverFramework/captain/fiber.cpp

CMakeFiles/captain.dir/captain/fiber.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/fiber.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/fiber.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/fiber.cpp > CMakeFiles/captain.dir/captain/fiber.cpp.i

CMakeFiles/captain.dir/captain/fiber.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/fiber.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/fiber.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/fiber.cpp -o CMakeFiles/captain.dir/captain/fiber.cpp.s

CMakeFiles/captain.dir/captain/log.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/log.cpp.o: ../captain/log.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/captain.dir/captain/log.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/log.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/log.cpp.o -c /home/lk/gitRepo/serverFramework/captain/log.cpp

CMakeFiles/captain.dir/captain/log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/log.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/log.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/log.cpp > CMakeFiles/captain.dir/captain/log.cpp.i

CMakeFiles/captain.dir/captain/log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/log.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/log.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/log.cpp -o CMakeFiles/captain.dir/captain/log.cpp.s

CMakeFiles/captain.dir/captain/scheduler.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/scheduler.cpp.o: ../captain/scheduler.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/captain.dir/captain/scheduler.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/scheduler.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/scheduler.cpp.o -c /home/lk/gitRepo/serverFramework/captain/scheduler.cpp

CMakeFiles/captain.dir/captain/scheduler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/scheduler.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/scheduler.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/scheduler.cpp > CMakeFiles/captain.dir/captain/scheduler.cpp.i

CMakeFiles/captain.dir/captain/scheduler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/scheduler.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/scheduler.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/scheduler.cpp -o CMakeFiles/captain.dir/captain/scheduler.cpp.s

CMakeFiles/captain.dir/captain/thread.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/thread.cpp.o: ../captain/thread.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/captain.dir/captain/thread.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/thread.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/thread.cpp.o -c /home/lk/gitRepo/serverFramework/captain/thread.cpp

CMakeFiles/captain.dir/captain/thread.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/thread.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/thread.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/thread.cpp > CMakeFiles/captain.dir/captain/thread.cpp.i

CMakeFiles/captain.dir/captain/thread.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/thread.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/thread.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/thread.cpp -o CMakeFiles/captain.dir/captain/thread.cpp.s

CMakeFiles/captain.dir/captain/util.cpp.o: CMakeFiles/captain.dir/flags.make
CMakeFiles/captain.dir/captain/util.cpp.o: ../captain/util.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/captain.dir/captain/util.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"captain/util.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/captain.dir/captain/util.cpp.o -c /home/lk/gitRepo/serverFramework/captain/util.cpp

CMakeFiles/captain.dir/captain/util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/captain.dir/captain/util.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/util.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lk/gitRepo/serverFramework/captain/util.cpp > CMakeFiles/captain.dir/captain/util.cpp.i

CMakeFiles/captain.dir/captain/util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/captain.dir/captain/util.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"captain/util.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lk/gitRepo/serverFramework/captain/util.cpp -o CMakeFiles/captain.dir/captain/util.cpp.s

# Object files for target captain
captain_OBJECTS = \
"CMakeFiles/captain.dir/captain/config.cpp.o" \
"CMakeFiles/captain.dir/captain/fiber.cpp.o" \
"CMakeFiles/captain.dir/captain/log.cpp.o" \
"CMakeFiles/captain.dir/captain/scheduler.cpp.o" \
"CMakeFiles/captain.dir/captain/thread.cpp.o" \
"CMakeFiles/captain.dir/captain/util.cpp.o"

# External object files for target captain
captain_EXTERNAL_OBJECTS =

../lib/libcaptain.so: CMakeFiles/captain.dir/captain/config.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/captain/fiber.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/captain/log.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/captain/scheduler.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/captain/thread.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/captain/util.cpp.o
../lib/libcaptain.so: CMakeFiles/captain.dir/build.make
../lib/libcaptain.so: CMakeFiles/captain.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lk/gitRepo/serverFramework/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking CXX shared library ../lib/libcaptain.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/captain.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/captain.dir/build: ../lib/libcaptain.so

.PHONY : CMakeFiles/captain.dir/build

CMakeFiles/captain.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/captain.dir/cmake_clean.cmake
.PHONY : CMakeFiles/captain.dir/clean

CMakeFiles/captain.dir/depend:
	cd /home/lk/gitRepo/serverFramework/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lk/gitRepo/serverFramework /home/lk/gitRepo/serverFramework /home/lk/gitRepo/serverFramework/build /home/lk/gitRepo/serverFramework/build /home/lk/gitRepo/serverFramework/build/CMakeFiles/captain.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/captain.dir/depend

