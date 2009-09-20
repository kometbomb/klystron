TARGET=engine
VPATH=src:src
ECHO = echo
CFG = debug
MACHINE = -march=pentium4 -mfpmath=sse -msse3


Group2_SRC = $(notdir ${wildcard src/util/*.c}) 
Group2_DEP = $(patsubst %.c, deps/Group2_$(CFG)_%.d, ${Group2_SRC})
Group2_OBJ = $(patsubst %.c, objs.$(CFG)/Group2_%.o, ${Group2_SRC})
Group0_SRC = $(notdir ${wildcard src/snd/*.c}) 
Group0_DEP = $(patsubst %.c, deps/Group0_$(CFG)_%.d, ${Group0_SRC})
Group0_OBJ = $(patsubst %.c, objs.$(CFG)/Group0_%.o, ${Group0_SRC}) $(Group2_OBJ)
Group1_SRC = $(notdir ${wildcard src/gfx/*.c}) 
Group1_DEP = $(patsubst %.c, deps/Group1_$(CFG)_%.d, ${Group1_SRC})
Group1_OBJ = $(patsubst %.c, objs.$(CFG)/Group1_%.o, ${Group1_SRC}) $(Group2_OBJ)
	
CXX = gcc -shared -std=gnu99 --no-strict-aliasing
CXXDEP = gcc -E -std=gnu99

CXXFLAGS = $(MACHINE) -mthreads -ftree-vectorize

# What include flags to pass to the compiler
INCLUDEFLAGS= -I src -I /mingw/include/sdl -I src/gfx -I src/snd -I src/util

# Separate compile options per configuration
ifeq ($(CFG),debug)
CXXFLAGS += -O3 -g -Wall ${INCLUDEFLAGS} -DDEBUG -fno-inline 
else
ifeq ($(CFG),profile)
CXXFLAGS += -O3 -pg -Wall ${INCLUDEFLAGS}
else
ifeq ($(CFG),release)
CXXFLAGS += -O3 -Wall ${INCLUDEFLAGS} -s
else
@$(ECHO) "Invalid configuration "$(CFG)" specified."
@$(ECHO) "You must specify a configuration when "
@$(ECHO) "running make, e.g. make CFG=debug"
@$(ECHO) "Possible choices for configuration are "
@$(ECHO) "'release', 'profile' and 'debug'"
@exit 1
exit
endif
endif
endif

# A common link flag for all configurations
LDFLAGS = 

all: bin.$(CFG)/lib${TARGET}_snd.a bin.$(CFG)/lib${TARGET}_gfx.a bin.$(CFG)/lib${TARGET}_util.a 

inform:
	@echo "Configuration "$(CFG)
	@echo "------------------------"
	
bin.$(CFG)/lib${TARGET}_snd.a: ${Group0_OBJ} | inform
	@$(ECHO) "Linking "$(TARGET)"..."
	@mkdir -p bin.$(CFG)
	@ar rcs $@ $^
	
bin.$(CFG)/lib${TARGET}_gfx.a: ${Group1_OBJ} | inform
	@$(ECHO) "Linking "$(TARGET)"..."
	@mkdir -p bin.$(CFG)
	@ar rcs $@ $^
	
bin.$(CFG)/lib${TARGET}_util.a: ${Group2_OBJ} | inform
	@$(ECHO) "Linking "$(TARGET)"..."
	@mkdir -p bin.$(CFG)
	@ar rcs $@ $^
	
objs.$(CFG)/Group0_%.o: snd/%.c 
	@$(ECHO) "Compiling "$(notdir $<)"..."
	@mkdir -p objs.$(CFG)
	@$(CXX) $(CXXFLAGS) -c $(CXXFLAGS) -o $@ $<

objs.$(CFG)/Group1_%.o: gfx/%.c 
	@$(ECHO) "Compiling "$(notdir $<)"..."
	@mkdir -p objs.$(CFG)
	@$(CXX) $(CXXFLAGS) -c $(CXXFLAGS) -o $@ $<

objs.$(CFG)/Group2_%.o: util/%.c
	@$(ECHO) "Compiling "$(notdir $<)"..."
	@mkdir -p objs.$(CFG)
	@$(CXX) $(CXXFLAGS) -c $(CXXFLAGS) -o $@ $<
	
deps/Group0_$(CFG)_%.d: snd/%.c 
	@mkdir -p deps
	@$(ECHO) "Generating dependencies for $<"
	@set -e ; $(CXXDEP) -MM $(INCLUDEFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,objs.$(CFG)\/Group0_\1.o $@ : ,g' \
		< $@.$$$$ > $@; \
	rm -f $@.$$$$

deps/Group1_$(CFG)_%.d: gfx/%.c 
	@mkdir -p deps
	@$(ECHO) "Generating dependencies for $<"
	@set -e ; $(CXXDEP) -MM $(INCLUDEFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,objs.$(CFG)\/Group1_\1.o $@ : ,g' \
		< $@.$$$$ > $@; \
	rm -f $@.$$$$

deps/Group2_$(CFG)_%.d: util/%.c
	@mkdir -p deps
	@$(ECHO) "Generating dependencies for $<"
	@set -e ; $(CXXDEP) -MM $(INCLUDEFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,objs.$(CFG)\/Group2_\1.o $@ : ,g' \
		< $@.$$$$ > $@; \
	rm -f $@.$$$$
	
clean:
	@rm -rf deps objs.$(CFG) bin.$(CFG)

# Unless "make clean" is called, include the dependency files
# which are auto-generated. Don't fail if they are missing
# (-include), since they will be missing in the first 
# invocation!
ifneq ($(MAKECMDGOALS),clean)
-include ${Group0_DEP}
-include ${Group1_DEP}
-include ${Group2_DEP}
endif
