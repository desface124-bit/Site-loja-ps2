EE_BIN = PS2STORE.ELF
EE_OBJS = src/main.o

EE_INCS = -I$(PS2DEV)/gsKit/include -I.
EE_LDFLAGS = -L$(PS2DEV)/gsKit/lib
EE_LIBS = -lgskit -ldmakit -lfontm -lpad -lkernel

all: $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
