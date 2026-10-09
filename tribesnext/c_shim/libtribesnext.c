#include <ruby.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#if !defined(__i386__)
#error "Build as 32-bit x86 (-m32)"
#endif

/*
 * TribesNEXT runtime bridge for the final Loki tribes2.dynamic build 25034.
 *
 * This library is deliberately tied to one 32-bit executable.
 * It redirects a single game initialization CALL to a nearby trampoline,
 * registers the rubyEval console command, and handle the ruby callback.
 *
 * The addresses below were recovered from the Loki build 25034.
 */
#define HOOK_ADDR ((uintptr_t)0x080ef2a0u)
#define ORIGINAL_INIT ((uintptr_t)0x080ed328u)
#define REGISTER_COMMAND ((uintptr_t)0x080e7dccu)
#define CONSOLE_EVAL ((uintptr_t)0x080e88f4u)

static const unsigned char original_call[5] = {0xe8, 0x83, 0xe0, 0xff, 0xff};
static uint32_t command_record[16];
static int registered;
static int debug;

static void die(const char *reason)
{
    fprintf(stderr, "[tribesnext-native] %s: %s\n", reason, strerror(errno));
    _exit(127);
}

/* The Ruby runtime lives in the game's main thread. No subprocess or IPC. */
#define MAX_TORQUE_OUTPUT 0x1200u
static int ruby_ready;
static int ruby_busy;
static char torque_output[MAX_TORQUE_OUTPUT + 3];
static size_t torque_length;

static void report_ruby_exception(const char *operation)
{
    VALUE exception = rb_errinfo();
    VALUE description = rb_funcall(exception, rb_intern("full_message"), 0);
    fprintf(stderr, "[tribesnext-ruby] %s: %s\n", operation,
            StringValueCStr(description));
    rb_set_errinfo(Qnil);
}

/* Called by Ruby */
static VALUE ruby_ts_eval(VALUE self, VALUE command)
{
    (void)self;
    VALUE str = rb_obj_as_string(command);
    long len = RSTRING_LEN(str);
    if (len > 0) {
        if ((size_t)len > MAX_TORQUE_OUTPUT - torque_length) {
            rb_raise(rb_eRuntimeError, "tsEval output exceeds %u bytes",
                     (unsigned)MAX_TORQUE_OUTPUT);
        }
        memcpy(torque_output + torque_length, RSTRING_PTR(str), (size_t)len);
        torque_length += (size_t)len;
    }
    return Qnil;
}

static int initialize_ruby(void)
{
    int state = 0;
    const char *script = getenv("TRIBESNEXT_RUBY_SCRIPT");
    if (!script || !*script) script = "./rubybase_embedded.rb";

    /* ruby_init must run on the same thread that subsequently evaluates Ruby. */
    RUBY_INIT_STACK;
    ruby_init();
    ruby_init_loadpath();
    ruby_script("tribesnext");
    rb_define_global_function("tsEval", ruby_ts_eval, 1);
    rb_load_protect(rb_str_new_cstr(script), 0, &state);
    if (state) {
        report_ruby_exception("loading rubybase_embedded.rb");
        return -1;
    }
    ruby_ready = 1;
    fprintf(stderr, "[tribesnext-ruby] Ruby initialized: %s\n", script);
    return 0;
}

static void ruby_eval_callback(void *object, int argc, const char **argv)
{
    (void)object;
    if (argc < 2 || !argv || !argv[1]) return;
    if (ruby_busy) {
        fprintf(stderr, "[tribesnext-ruby] nested rubyEval rejected\n");
        return;
    }
    ruby_busy = 1;
    if (!ruby_ready && initialize_ruby() != 0) {
        ruby_busy = 0;
        return;
    }

    if (debug) {
        fprintf(stderr, "%s\n", argv[1]);
    }

    torque_length = 0;
    int state = 0;
    rb_eval_string_protect(argv[1], &state);
    if (state) {
        report_ruby_exception("evaluating rubyEval");
        torque_length = 0; /* Do not execute a partial response on failure. */
    }

    /* The old bridge executed one concatenated buffer, followed by LF/CR.
     * An empty Ruby response previously became a single space. */
    if (torque_length == 0) torque_output[torque_length++] = ' ';
    torque_output[torque_length++] = '\n';
    torque_output[torque_length++] = '\r';
    torque_output[torque_length] = '\0';
    typedef void (*console_eval_fn)(const char *, int, int);
    ((console_eval_fn)CONSOLE_EVAL)(torque_output, 1, 0);
    ruby_busy = 0;
}

/* Same registration arguments as the injected stub:
 * record, NULL, "rubyEval", callback, "rubyEval(consoleString)", 2, 2. */
static void register_ruby_eval(void)
{
    typedef void (*register_fn)(void *, void *, const char *,
                                void (*)(void *, int, const char **),
                                const char *, int, int);
    ((register_fn)REGISTER_COMMAND)(command_record, NULL, "rubyEval",
                                   ruby_eval_callback,
                                   "rubyEval(consoleString)", 2, 2);
    registered = 1;
}

/* The original hook target is an initialization function taking two stack
 * arguments. The existing payload registers rubyEval and then jumps into
 * ORIGINAL_INIT+0x20. Here C registers first and calls ORIGINAL_INIT normally. */
static void game_init_hook(void *context, unsigned int mode)
{
    if (!registered && context && mode == 0xffffu)
        register_ruby_eval();

    /* Preserve the game's original initialization behavior. */
    typedef void (*original_init_fn)(void *, unsigned int);
    ((original_init_fn)ORIGINAL_INIT)(context, mode);
}

/* A shared library may be mapped above 0x80000000, more than 2 GiB
 * away from the fixed-address game code. A near CALL cannot reach it.
 * Put a tiny absolute-jump trampoline in a free page near the game.
 * `push imm32; ret` preserves registers and leaves the CALL return address
 * on the stack, so the C function can return to the game normally. */
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

static void *allocate_near_trampoline(size_t page_size)
{
    /* All these locations are within rel32 reach of HOOK_ADDR. Never use
     * MAP_FIXED: that could overwrite an existing game mapping. */
    static const uintptr_t candidates[] = {
        0x08047000u, 0x08046000u, 0x08045000u,
        0x09000000u, 0x0a000000u, 0x10000000u,
        0x20000000u, 0x40000000u
    };
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
        uintptr_t addr = candidates[i];
        int64_t displacement = (int64_t)addr - ((int64_t)HOOK_ADDR + 5);
        if (displacement < INT32_MIN || displacement > INT32_MAX) continue;
        void *p = mmap((void *)addr, page_size,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                       -1, 0);
        if (p == (void *)addr) return p;
        if (p != MAP_FAILED) munmap(p, page_size);
    }
    return MAP_FAILED;
}

__attribute__((constructor)) static void install_bridge(void)
{
    debug = 0;
    const char *p = getenv("DEBUG");
    if(p != NULL && (strcmp(p, "0") != 0)) {
        debug = 1;
    }

    /* Stop if this is not the known, unmodified call site. */
    if (memcmp((const void *)HOOK_ADDR, original_call, sizeof(original_call))) {
        errno = EINVAL;
        die("unsupported game binary or already patched hook");
    }
    long ps = sysconf(_SC_PAGESIZE);
    if (ps <= 0 || (ps & (ps - 1))) { errno = EINVAL; die("unsupported page size"); }

    void *trampoline = allocate_near_trampoline((size_t)ps);
    if (trampoline == MAP_FAILED) {
        errno = ENOMEM;
        die("cannot reserve trampoline page near game code");
    }

    /* push imm32; ret -- transfers control to the C hook without
     * changing any general-purpose register or the CALL return address. */
    unsigned char code[6] = { 0x68, 0, 0, 0, 0, 0xc3 };
    uint32_t target = (uint32_t)(uintptr_t)&game_init_hook;
    memcpy(code + 1, &target, sizeof(target));
    memcpy(trampoline, code, sizeof(code));
    __builtin___clear_cache((char *)trampoline, (char *)trampoline + sizeof(code));
    if (mprotect(trampoline, (size_t)ps, PROT_READ | PROT_EXEC))
        die("cannot make trampoline executable");

    /* Patch only the rel32 displacement: the original E8 opcode stays. */
    uintptr_t page = HOOK_ADDR & ~((uintptr_t)ps - 1);
    int64_t disp = (int64_t)(uintptr_t)trampoline - ((int64_t)HOOK_ADDR + 5);
    if (disp < INT32_MIN || disp > INT32_MAX) {
        errno = ERANGE;
        die("trampoline outside x86 rel32 range");
    }
    if (mprotect((void *)page, (size_t)ps, PROT_READ | PROT_WRITE | PROT_EXEC))
        die("cannot make hook writable");
    int32_t rel32 = (int32_t)disp;
    memcpy((void *)(HOOK_ADDR + 1), &rel32, sizeof(rel32));
    __builtin___clear_cache((char *)HOOK_ADDR, (char *)(HOOK_ADDR + 5));
    if (mprotect((void *)page, (size_t)ps, PROT_READ | PROT_EXEC))
        die("cannot restore hook page protection");

    fprintf(stderr, "[tribesnext-native] installed: hook=%p trampoline=%p C target=%p\n",
            (void *)HOOK_ADDR, trampoline, (void *)&game_init_hook);
}
