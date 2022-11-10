ifneq ($(VERBOSE),true)
  QUIET:=@
endif

# compiler options
ifndef CC
CC:=clang
endif

ifndef CXX
CXX:=clang++
endif

ifndef CPP_STANDARD
#  CPP_STANDARD:=-std=c++20
  CPP_STANDARD:=-std=c++17
endif

ifndef C_STANDARD
  C_STANDARD:=-std=c11
endif

# set CFLAGS
ifdef FLAGS
  CFLAGS=$(FLAGS)
endif

ifdef DEFINES
  CFLAGS += $(foreach var, $(DEFINES),-D$(var))
else # else use default options
endif

ifeq (,$(findstring -DLOCALE=,$(CFLAGS)))
  CFLAGS += -DLOCALE=US
endif

# set CPPFLAGS
ifdef FLAGS
  CPPFLAGS=$(CFLAGS)
endif
#CPPFLAGS+=-stdlib=libc++ $(CFLAGS)
#LDFLAGS+=-stdlib=libc++
LDFLAGS+=-lcurl -lsqlite3


ifndef SOURCE_PATH
  SOURCE_PATH=.
endif

ifndef BUILD_PATH
  BUILD_PATH=$(SOURCE_PATH)/bin
endif

# define what to build
ifndef BINARY
  BINARY=gpsscraper
endif

SOURCES = \
	main.cpp \
	scrapers/utilities.cpp \
	scrapers/scraper_base.cpp \
	scrapers/chargehub_scraper.cpp \
	shortjson/shortjson_strict.cpp \
	simplified/simple_sqlite.cpp

OBJS := $(SOURCES:.s=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(foreach f,$(OBJS),$(BUILD_PATH)/$(f))
SOURCES := $(foreach f,$(SOURCES),$(SOURCE_PATH)/$(f))

# !!! FIXME: Get -Wall in here, some day.
#CFLAGS += -w -fno-builtin -fno-strict-aliasing -fno-operator-names -fno-rtti -ffreestanding

# includes ...
INCLUDES:=-I.


.PHONY: all OUTPUT_DIR

$(BUILD_PATH)/%.o: $(SOURCE_PATH)/%.c
	@echo [Compiling]: $<
	$(QUIET) $(CC) -c -o $@ $< $(C_STANDARD) $(INCLUDES) $(CFLAGS)

$(BUILD_PATH)/%.o: $(SOURCE_PATH)/%.s
	@echo [Assembling]: $<
	$(QUIET) $(CC) $(CPP_STANDARD) -DELF -x assembler-with-cpp -o $@ -c $<

$(BUILD_PATH)/%.o: $(SOURCE_PATH)/%.cpp
	@echo [Compiling]: $<
	$(QUIET) $(CXX) -c -o $@ $< $(CPP_STANDARD) $(INCLUDES) $(CPPFLAGS)

$(BUILD_PATH)/scrapers/%.o: $(SOURCE_PATH)/scrapers/%.cpp
	@echo [Compiling]: $<
	$(QUIET) $(CXX) -c -o $@ $< $(CPP_STANDARD) $(INCLUDES) $(CPPFLAGS)

$(BUILD_PATH)/simplified/%.o: $(SOURCE_PATH)/simplified/%.cpp
	@echo [Compiling]: $<
	$(QUIET) $(CXX) -c -o $@ $< $(CPP_STANDARD) $(INCLUDES) $(CPPFLAGS)

$(BUILD_PATH)/shortjson/%.o: $(SOURCE_PATH)/shortjson/%.cpp
	@echo [Compiling]: $<
	$(QUIET) $(CXX) -c -o $@ $< $(CPP_STANDARD) $(INCLUDES) $(CPPFLAGS)

$(BUILD_PATH)/%.a: $(SOURCE_PATH)/%.a
	cp $< $@
	ranlib $@

$(BINARY): OUTPUT_DIR $(OBJS)
	@echo [ Linking ]: $@
	$(QUIET) $(CXX) -o $@ $(OBJS) $(LDFLAGS) $(CPP_STANDARD)
	@echo [ DONE ]

OUTPUT_DIR:
	@echo -n "Creating build directory"
	$(QUIET) mkdir -p $(BUILD_PATH)
	$(QUIET) mkdir -p $(BUILD_PATH)/scrapers
	$(QUIET) mkdir -p $(BUILD_PATH)/simplified
	$(QUIET) mkdir -p $(BUILD_PATH)/shortjson
	@echo " DONE."

clean:
	rm -f $(BINARY)
	rm -rf $(BUILD_PATH)
