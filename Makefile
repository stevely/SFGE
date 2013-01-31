# Makefile for SFGE
# By Steven Smith

CC= clang
ANALYZE= clang --analyze
WARNS= -W -Werror -Wall
CFLAGS= -g $(WARNS)
LFLAGS=
# Frameworks are a part of OS X compilation
FRAMEWORKS= Cocoa OpenGL IOKit
LIBS= glfw3
SRC= src
BUILD= build
LIBDIR= lib

# Sources
SFGE_S= engine.c renderer.c gamelogic.c drawset.c messages.c

# Final executable
SFGE= game

# Extra libs
EXTRAS= sst sdf tinycthread

# Helper function
getobjs= $(patsubst %.c,$(BUILD)/%.o,$(1))
cond_= $(if $(1), $(2) $(1))
cond= $(call cond_, $(wildcard $(1)), $(2))

# Derivative values
FWS= $(foreach fw,$(FRAMEWORKS),-framework $(fw))
LBS= $(foreach lb,$(LIBS),-l$(lb))
EXS= $(foreach ex,$(EXTRAS),$(LIBDIR)/$(ex))
INC= $(foreach ex,$(EXS),-I $(ex))

all: $(EXS) $(SFGE)

$(SFGE): $(call getobjs, $(SFGE_S)) | $(EXS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(foreach ex,$|,$(wildcard $(ex)/*.o)) $(LBS) $(FWS)

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(ANALYZE) $(WARNS) $(INC) $<
	$(CC) $(CFLAGS) -I $(SRC) $(INC) -c -o $@ $<

$(EXS): %: %.tar
	$(call cond, $@, $(RM) -r)
	mkdir $@
	tar -xf $< -C $@

clean:
	$(call cond, $(BUILD)/*.o, $(RM))
	$(call cond, $(BUILD), rmdir)
	$(call cond, $(EXAMPLES), $(RM))
	$(call cond, $(EXS), $(RM) -r)

.PHONY: clean
