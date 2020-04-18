CXX=g++
GDB=gdb

# Adjust and uncomment to enable the corresponding tests
# STLPORT46_ROOT_DIR=/usr/local/stlport-4.6.2
# STLPORT50_ROOT_DIR=/usr/local/stlport-5.0.3
# STLPORT51_ROOT_DIR=/usr/local/stlport-5.1.7
# STLPORT52_ROOT_DIR=/usr/local/stlport-5.2.1

# To enable Boost support you might need to patch
# $(STLPORT52_ROOT_DIR)/include/stlport/stl/type_traits.h
# to include <boost/type_traits/remove_const.hpp>.
# STLPORT52_USE_BOOST_SUPPORT=1



# Common flags for STLport 4.6.x tests
STLPORT46_FLAGS = \
  $(CXXFLAGS) $(LDFLAGS) \
  -I$(STLPORT46_ROOT_DIR)/include/stlport \
  -L$(STLPORT46_ROOT_DIR)/lib \
  -Wl,-R$(STLPORT46_ROOT_DIR)/lib \
  -pthread

# Common flags for STLport 5.0.x tests
STLPORT50_FLAGS = \
  $(CXXFLAGS) $(LDFLAGS) \
  -I$(STLPORT50_ROOT_DIR)/include/stlport \
  -L$(STLPORT50_ROOT_DIR)/lib \
  -Wl,-R$(STLPORT50_ROOT_DIR)/lib \
  -pthread

# Common flags for STLport 5.1.x tests
STLPORT51_FLAGS = \
  $(CXXFLAGS) $(LDFLAGS) \
  -I$(STLPORT51_ROOT_DIR)/include/stlport \
  -L$(STLPORT51_ROOT_DIR)/lib \
  -Wl,-R$(STLPORT51_ROOT_DIR)/lib \
  -pthread

# Common flags for STLport 5.2.x tests
STLPORT52_FLAGS = \
  $(CXXFLAGS) $(LDFLAGS) \
  -I$(STLPORT52_ROOT_DIR)/include/stlport \
  -L$(STLPORT52_ROOT_DIR)/lib \
  -Wl,-R$(STLPORT52_ROOT_DIR)/lib \
  -pthread

ifneq ($(STLPORT52_USE_BOOST_SUPPORT),)
STLPORT52_FLAGS += -D_STLP_USE_BOOST_SUPPORT
endif



# Add STLport 4.6.x tests to $(TARGETS) (if enabled)
ifneq ($(STLPORT46_ROOT_DIR),)
TARGETS += test_stlport46 test_stlport46d
endif

# Add STLport 5.0.x tests to $(TARGETS) (if enabled)
ifneq ($(STLPORT50_ROOT_DIR),)
TARGETS += test_stlport50 test_stlport50d
endif

# Add STLport 5.1.x tests to $(TARGETS) (if enabled)
ifneq ($(STLPORT51_ROOT_DIR),)
TARGETS += test_stlport51 test_stlport51d
endif

# Add STLport 5.2.x tests to $(TARGETS) (if enabled)
ifneq ($(STLPORT52_ROOT_DIR),)
TARGETS += test_stlport52 test_stlport52d
endif



default: run
ifeq ($(TARGETS),)
	@echo "You need to configure the STLport directory at the start of the Makefile."
endif

run: build
ifneq ($(TARGETS),)
	for TARGET in $(TARGETS); do \
	  echo "Running test for $$TARGET"; \
	  $(GDB) -batch -x script ./$$TARGET; \
	done
endif

build: $(TARGETS)

test_stlport46: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT46_FLAGS) -lstlport_gcc

test_stlport46d: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT46_FLAGS) -lstlport_gcc_stldebug -D_STLP_DEBUG

test_stlport50: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT50_FLAGS) -lstlport

test_stlport50d: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT50_FLAGS) -lstlportstlg -D_STLP_DEBUG

test_stlport51: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT51_FLAGS) -lstlport

test_stlport51d: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT51_FLAGS) -lstlportstlg -D_STLP_DEBUG

test_stlport52: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT52_FLAGS) -lstlport

test_stlport52d: test.cpp Makefile
	$(CXX) -o $@ $< -g $(STLPORT52_FLAGS) -lstlportstlg -D_STLP_DEBUG

clean:
	rm -f test_stlport*
