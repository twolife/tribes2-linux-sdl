BINS=libtribesnext.so

CFLAGS ?= -O2 -Wall -Wextra  -Wno-unused-parameter -std=gnu11

RUBY_PKG ?= ruby-2.1
PKG_CONFIG_LIBDIR ?= /usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig
RUBY_PC = PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" pkg-config
CFLAGS += $(shell $(RUBY_PC) --cflags $(RUBY_PKG))
LDLIBS += $(shell $(RUBY_PC) --libs $(RUBY_PKG) | sed 's/-lruby-2.1/-lruby-2.1-static/')

all: $(BINS)

%.so: %.c
	$(CC) -m32 $(CFLAGS) -fPIC -shared -o $@ $^ $(LDLIBS)

clean:
	rm -f $(BINS)
