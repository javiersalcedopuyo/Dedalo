EXECUTABLE_NAME=ddl

CC = clang++
CFLAGS = -std=c++20\
		 -Werror -Wall -Wextra -pedantic\
		 -DENABLE_LOGS\
		 -fno-exceptions\
		 -fno-rtti

MAN_PAGE_PATH = /usr/local/share/man/man1


RELEASE_FLAGS = -O3 -DNDEBUG\
				-Wno-assume
DEBUG_FLAGS   = -MJ tmp.json\
				-fsanitize=undefined,address\
				-O0 -g

CLANG_TIDY_CHECKS = bugprone-*,\
					clang-analyzer-*,\
					misc-*,\
					modernize-*,\
					performance-*,\
					-bugprone-command-processor,\
					-bugprone-lambda-function-name,\
					-clang-diagnostic-c2y-extensions,\
					-misc-non-private-member-variables-in-classes,\
					-misc-use-anonymous-namespace,\
					-misc-use-internal-linkage,\
					-misc-include-cleaner,\
					-misc-no-recursion,\
					-modernize-use-designated-initializers,\
					-modernize-avoid-c-style-cast

ddl: dedalo.cpp
	@echo "Compiling..."
	@time $(CC) $(CFLAGS) $(DEBUG_FLAGS) dedalo.cpp -o $(EXECUTABLE_NAME)
	@echo '[' > compile_commands.json
	@cat tmp.json >> compile_commands.json
	@echo ']' >> compile_commands.json
	@rm tmp.json

.PHONY: run rebuild  release debug clean reset install uninstall tidy

run: dedalo
	./$(EXECUTABLE_NAME) run

rebuild:
	$(MAKE) clean
	$(MAKE) $(EXECUTABLE_NAME)

release:
	$(MAKE) clean
	@echo "Compiling for release..."
	@time $(CC) $(CFLAGS) $(RELEASE_FLAGS) dedalo.cpp -o $(EXECUTABLE_NAME)

debug:
	$(MAKE) rebuild

clean:
	rm -rf $(EXECUTABLE_NAME) compile_commands.json *.dSYM

reset:
	@echo "Cleaning Dedalo itself..."
	rm -rf $(EXECUTABLE_NAME) compile_commands.json *.dSYM
	@echo "Cleaning Dedalo's output..."
	rm -rf dependencies tests src build build.cpp

install:
	$(MAKE) release
	mv $(EXECUTABLE_NAME) /usr/local/bin
	cp dedalo.cpp /usr/local/include
	mkdir -p $(MAN_PAGE_PATH)
	cp ddl.1 $(MAN_PAGE_PATH)

uninstall:
	rm /usr/local/bin/$(EXECUTABLE_NAME)
	rm /usr/local/include/dedalo.cpp
	rm $(MAN_PAGE_PATH)/ddl.1

tidy:
	clang-tidy dedalo.cpp -p . --checks='$(CLANG_TIDY_CHECKS)'
