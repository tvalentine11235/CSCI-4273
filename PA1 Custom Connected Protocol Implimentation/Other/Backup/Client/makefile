# client
EXE=client

# Main target
all: $(EXE)

#  MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -w
CLEAN=del *.exe *.o *.a

else
#  OSX

ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -w -Wno-deprecated-declarations
#  Linux/Unix/Solaris

else
CFLG=-O3 -w

endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a

endif
#  Compile and Link
client:udp_client.c
	gcc $(CFLG) -o $@ $^
#  Clean
clean:
	$(CLEAN)
