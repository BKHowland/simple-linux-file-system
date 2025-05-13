###############################################################################################
# Make file template
# Author:	Ahmad Abdullah
#					University of Victoria
#					Computer Science Department
#					CSC 360 Introduction to Operating Systems
#					Spring 2023
#
# Last Updated: January 27, 2023
###############################################################################################
#
# When using make, please remeber:
#
# 1) When you type make or make [target], the Make will look through your current directory for
#    a Makefile. This file must be called makefile or Makefile.
# 2) Make will then look for the corresponding target in the makefile. If you don�t provide a
#    target, Make will just run the first target it finds.
# 3) If the target is found, the target�s dependencies will be run as needed, then the target 
#    commands will be run.
# 4) If any dependency need to be updated, the dependency will be updated first.

# First let's define some handy variables

COMPILER=gcc
OPTIONS=

# The following line commented (will not be used) and left to show other gcc options
#OPTIONS=-g -std=c++17 -pedantic -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code

COMPILE=$(COMPILER) $(OPTIONS)
BUILD=build

# Update the following with your program name
PROGRAM1=diskinfo
PROGRAM2=disklist
PROGRAM3=diskget
PROGRAM4=diskput

# Compile main by default
all: program1 program2 program3 program4

# $(BUILD)/*.o expands to all .o files in the $(BUILD) directory
# In this case, we'll get $(BUILD)/file1.o $(BUILD)/file2.o
#   $@ expands to the target name, i.e. target1
#   $< expands to the first dependency, i.e. dependency1
#   $^ expands to the complete list of dependencies, i.e. dependency1 dependency2


program1: diskinfo.c dirinfo_def.h
	$(COMPILE) $< -o $(PROGRAM1)

program2: disklist.c dirinfo_def.h
	$(COMPILE) $< -o $(PROGRAM2)
	
program3: diskget.c dirinfo_def.h
	$(COMPILE) $< -o $(PROGRAM3)

program4: diskput.c dirinfo_def.h
	$(COMPILE) $< -o $(PROGRAM4)

# Make the build directory if it doesn't exist

# Delete the build directory and program
clean:
	
# rm -rf diskinfo
# rm -rf disklist
# rm -rf diskget
# rm -rf diskput

# These rules do not correspond to a specific file
.PHONY: build clean