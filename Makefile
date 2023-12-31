# Author : Murray Fordyce

# README
#
# I hope this is readable for anybody who wants to learn.  Please ask
# me questions if you have any, I'm happy to share and "Make" is one
# of my favourite languages.

##
# Set up the build environment and settings
##
SHELL = /bin/bash
#COLOR = $(shell echo $$TERM|grep color > /dev/null && echo always || echo auto)
COLOR = always
#CXX = clang++
CXX = /usr/lib/llvm/16/bin/clang++
LanguageVersion = -std=c++20
Warnings = -Wall -Werror -Wextra -Wshadow
NoWarn = -Wno-dangling-else -Wno-c++98-compat -Wno-padded -Wno-missing-prototypes -Wno-dangling-else -Wno-old-style-cast -Wno-unused-macros -Wno-comma -Wno-return-std-move
#SANS=address
ifeq ($(SANS),)
	SANITIZER=
else
	SANITIZER=-fsanitize=$(SANS) -ftrapv
endif

Debugging = -Wfatal-errors -fdiagnostics-color=$(COLOR) -g $(SANITIZER)
CXXFLAGS = $(LanguageVersion) $(Warnings) $(NoWarn) $(Debugging)

HEADER_FILES  = $(wildcard *.h) $(wildcard *.hpp)
CCODE_FILES   = $(wildcard *.c)
CPPCODE_FILES = $(wildcard *.cpp)
CODE_FILES    = $(CCODE_FILES) $(CPPCODE_FILES)

##
# Describe the actual program structure.
# These This is the only important line for building the program.
##
execution_test: execution.o execution_test.o
	$(CXX) $(CXXFLAGS) -o $@ $^

all: execution_test
clean_targets:
	-rm execution_test

##
# Code to check for `#include' statements.
##
DEPDIR := .dep
$(DEPDIR): ; mkdir $@
.PRECIOUS: $(DEPDIR)/%.d
$(DEPDIR)/%.d: %.cpp | $(DEPDIR)
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $< -MF $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
$(DEPDIR)/%.d: %.c | $(DEPDIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(CXXFLAGS) $< -MF $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
%.o: .dep/%.d
DEPFILES := $(CCODE_FILES:%.c=$(DEPDIR)/%.d) $(CPPCODE_FILES:%.cpp=$(DEPDIR)/%.d)
include $(DEPFILES)

##
# Tests are run from the tests.d directory and temporary results are stored in the tests.r directory
##
%.d: ; mkdir $@
TEST_DIR = tests.d
TEST_RES = tests.r
TEST_FILES   = $(wildcard $(TEST_DIR)/*)
$(TEST_RES): ; mkdir $@

##
# Make a pseudo-target for the tests
##
# Also, make a happy message at the end if they all passed.
.PHONY: tests
tests:
	@echo "+------------------------------------------------+"
	@echo "| All tests passed and All documenation is done. |"
	@echo "+------------------------------------------------+"

##
# Very small "interactive debugger"
##
.PHONY: watch
watch:
	watch --color "$(MAKE) _watch_wait && $(MAKE) tests"
.PHONY: _watch_wait
# This is a separate directive so that the lists update.
IMPORTANT_FILES = $(HEADER_FILES) $(CODE_FILES) $(TEST_FILES)
_watch_wait:
	@inotifywait $(IMPORTANT_FILES)

##
# Find TODO tags and display them at the top of the output for convenience
##
.PHONY: TODO
tests: TODO
# TODO: Don't display this if there are no TODOs
# TODO: Better sort on line numbers.
TODO:
	@echo "+---------------------------- - - -  -   --    ---"
	@echo "| List of TODO in code:"
	@echo "|"
	@grep --color=$(COLOR) -n "\(TODO\|FIXME\)" $(HEADER_FILES) $(CODE_FILES) |sort |sed 's,\(.\),| - \1,' | sed 's,\s*//\s*,\t,' || true
	@grep --color=$(COLOR) -n MAINTENANCE $(HEADER_FILES) $(CODE_FILES) |sort |sed 's,\(.\),| - \1,' | sed 's,\s*//\s*,\t,' || true
	@echo "+---------------------------- - - -  -   --    ---"
	@echo ""

##
# Run the tests
##
$(TEST_RES)/.updateded: tests.org $(TEST_RES)
	@printf "Updating Tests ... "
	@rm -rf $(TEST_DIR)
	@emacs --batch tests.org --eval "(setq org-src-preserve-indentation t)" -f org-babel-tangle
	@echo "Done"
	@touch $@

tests: test_run
.PHONY: test_run
test_run: parser simplifyer $(TEST_RES)/.updateded
	@./testrunner

##
# Give a list of missing documentation
##
Doxyfile:
	doxygen -g > /dev/null
tests: $(TEST_RES)/docs
$(TEST_RES)/docs: $(wildcard *.h) $(wildcard *.cpp) Doxyfile Makefile
	@echo "+------------------------------------------------+"
	@echo "| Tests Compleate                                |"
	@echo '| Checking for missing Documentation             |'
	@echo "+------------------------------------------------+"
	@doxygen 2>&1 | grep "warning" | sed 's,.*/,,' | tee $@.tmp | head -n 30
	@diff $@.tmp /dev/null > /dev/null
	@mv $@.tmp $@

##
# Remove the files made by this Makefile
##
clean: clean_targets
	-rm *.o
	-rm Doxyfile doxygen* docs docs.tmp
	-rm -rf html latex
	-rm -rf .dep
	-rm -rf $(TEST_RES)
