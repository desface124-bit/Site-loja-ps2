EE_BIN = PS2STORE.ELF
EE_OBJS = src/main.o

EE_INCS = -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/common/include -I$(PS2SDK)/ee/include
EE_LDFLAGS = -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ee/lib
EE_LIBS = -lgskit -ldmakit -lpad -lkernel

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
