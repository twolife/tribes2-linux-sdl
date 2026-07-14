BINS=tribes2shim.so

CFLAGS ?= -O2 -m32 -fPIC -Wall

all: $(BINS)

%.so: %.o
	$(LD) -m elf_i386 -shared -o $@ $^ $(LDFLAGS) --no-as-needed -l:libdl.so.2

clean:
	rm -f $(BINS) $(addsuffix .o,$(basename $(BINS)))
