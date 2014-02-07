include Makefile.arch

# ----------------------------------------------------------------------- #
# stuff to make
# ----------------------------------------------------------------------- #
SOURCES := $(wildcard *.cc)                  \
           $(wildcard tcmet/*.cc)            \
           $(wildcard QuarkGluonTagger/*.cc) \
		   $(wildcard jetcorr/*.cc)          \
		   $(wildcard jetsmear/*.cc)         \
		   $(wildcard MT2/*.cc)
OBJECTS := $(SOURCES:.cc=.o)
DEPS    := $(SOURCES:.cc=.d)
LIBS    := libCMS2NtupleMacrosCORE.so

# ----------------------------------------------------------------------- #
# how to make it
# ----------------------------------------------------------------------- #
$(LIBS): $(OBJECTS) 
	$(LD) $(LDFLAGS) $(SOFLAGS) $(OBJECTS) $(ROOTLIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ----------------------------------------------------------------------- #
# target to build
# ----------------------------------------------------------------------- #
.PHONY: all
all: $(DEPS) $(LIBS)

# for troubleshooting
.PHONY: test
test: 
	@echo SOURCES = $(SOURCES)
	@echo OBJECTS = $(OBJECTS)
	@echo LIBS    = $(LIBS)
	@echo DEPS    = $(DEPS)

# ----------------------------------------------------------------------- #
# clean target
# ----------------------------------------------------------------------- #
.PHONY: clean
clean:
	@echo "removed object and shared object files"
	@find . -name "*.o"  | xargs -I {} rm {}
	@find . -name "*.d"  | xargs -I {} rm {}
	@find . -name "*.so" | xargs -I {} rm {}

# ----------------------------------------------------------------------- #
# check dependencies
# ----------------------------------------------------------------------- #
-include $(DEPS)

%.d: %.cc
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$;                \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
