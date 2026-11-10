// Exercise real fixups parsing and loader aspect state; synthetic pages, no game files.
#include "../loader.c"
#include <assert.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

struct dm_flags g_flag;
uint64_t now_ns(void) { return 0; }
void mtl_present_screen(uint32_t w, uint32_t h) { (void)w; (void)h; }   // loader.c: a screen taller than 16:9; no renderer here
void gate_register(uint64_t a, const char *k, int n, uint64_t o) { (void)a; (void)k; (void)n; (void)o; }
void hlecount_seal(void) {}
void *hlecount_wrap(const char *kind, const char *name, void *fn) { (void)kind; (void)name; return fn; }
uint64_t resolve_import(module_t *m, const char *name) { (void)m; (void)name; return 0; }
int hle_implements(const char *name) { (void)name; return 0; }
int svc_nro_deactivate(module_t *m) { m->loaded = 0; return 1; }

static _Alignas(0x4000) uint8_t image[0xc000];
int dm_mem_started(void) { return 0; }   // hle_svc.c: guest memory not laid out yet (loader.c dm_fps_state_apply)
static module_t module;
static const char *verified = "cd57b23fa4bbad65803d9788c01821ee";

static void reset(void) {
    atomic_store(&g_aspect_state, NULL);
    g_aspect_enabled = 0; g_aspect_drawable = 0; dm_fps_state = NULL;
    memset(image, 0, sizeof image);
    module = (module_t){.base = image, .size = sizeof image, .fps_state_off = 0x8000,
                        .key_page_off = 0x4000, .key_page_size = 0x4000, .camera_aspect = 1};
    strcpy(module.name, "main"); strcpy(module.build_id, verified);
    unsetenv("DM_WIDESCREEN"); setenv("DM_FPS_TARGET", "0", 1); unsetenv("DM_RENDER_SCALE");
}
static float desired(void) {
    uint32_t bits = atomic_load((_Atomic uint32_t *)(void *)(image+0x8320));
    float f; memcpy(&f, &bits, sizeof f); return f;
}
// The execution regression runs the real guest writer; this host-only fixture
// emulates its release acknowledgement to test the consumer independently.
static void acknowledge(void) {
    uint32_t bits = atomic_load((_Atomic uint32_t *)(void *)(image+0x8320));
    atomic_store_explicit((_Atomic uint32_t *)(void *)(image+0x8324), bits, memory_order_release);
}
static void preload_ordering(void) {
    reset(); dm_game_aspect_drawable(2520, 1080);
    assert(dm_game_aspect() == 0);
    install_fps_state(&module);
    assert(desired() == 0 && dm_game_aspect() == 0); // preload must not read launcher settings
    dm_fps_state_apply();
    assert(desired() == 2520.0f/1080 && dm_game_aspect() == 0);
    acknowledge(); assert(dm_game_aspect() == desired());
    dm_game_aspect_drawable(3840, 1080);
    assert(desired() == 3840.0f/1080 && dm_game_aspect() == 2520.0f/1080);
    acknowledge(); assert(dm_game_aspect() == desired());
    dm_game_aspect_drawable(1920, 1080);
    assert(desired() == 0 && dm_game_aspect() == 3840.0f/1080);
    acknowledge(); assert(dm_game_aspect() == 0);

    // Opposite notification order also works, including an iOS opt-out set after preload.
    reset(); install_fps_state(&module); setenv("DM_WIDESCREEN", "0", 1);
    dm_fps_state_apply(); dm_game_aspect_drawable(2520, 1080);
    assert(desired() == 0 && dm_game_aspect() == 0);
    unsetenv("DM_WIDESCREEN"); dm_fps_state_apply();
    assert(desired() == 2520.0f/1080); acknowledge(); assert(dm_game_aspect() == desired());
    setenv("DM_WIDESCREEN", "0", 1); dm_fps_state_apply();
    assert(desired() == 0 && dm_game_aspect() == 2520.0f/1080);
    acknowledge(); assert(dm_game_aspect() == 0);
}
static void unsupported(void) {
    for (unsigned mode = 0; mode < 3; mode++) {
        reset();
        if (mode == 0) module.camera_aspect = 0; // old output, no hook
        if (mode == 1) strcpy(module.build_id, "00000000000000000000000000000000");
        if (mode == 2) strcpy(module.name, "sdk");
        install_fps_state(&module); dm_game_aspect_drawable(2520, 1080); dm_fps_state_apply();
        assert(desired() == 0 && dm_game_aspect() == 0);
    }
    reset(); install_fps_state(&module); dm_fps_state_apply();
    for (unsigned i = 0; i < 3; i++) {
        dm_game_aspect_drawable(i ? 1024 : 1920, i == 2 ? 1024 : 1080);
        assert(desired() == 0);
    }
    dm_game_aspect_drawable(2520, 1080);
    for (unsigned i = 0; i < 3; i++) dm_game_aspect_drawable(i ? NAN : 0, 1080);
    assert(desired() == 2520.0f/1080); // transient invalid drawable notifications do not disable
}
static void layout(void) {
    reset();
    char dir[] = "/tmp/dm-aspect-XXXXXX"; assert(mkdtemp(dir));
    char path[128]; snprintf(path, sizeof path, "%s/main.fixups.tsv", dir);
    FILE *f = fopen(path, "w"); assert(f);
    fprintf(f, "# PATCH_OFF\t0xc000\n# KEY_PAGE_OFF\t0x4000\n# KEY_PAGE_SIZE\t0x4000\n"
               "# KEY_K18_IDX\t0\n# KEY_KTLS_IDX\t1\n# KEY_KTLR_IDX\t2\n# KEY_KGATE_IDX\t3\n"
               "# FPS_STATE_OFF\t0x8000\n# CAMERA_ASPECT\t1\n# BUILD_ID\t%s\n", verified);
    fclose(f); module.camera_aspect = 0; module.fps_state_off = 0; module.build_id[0] = 0;
    read_layout(&module, dir);
    assert(module.camera_aspect && module.fps_state_off == 0x8000 && !strcmp(module.build_id, verified));
    unlink(path); rmdir(dir);
    module.size = 0x8328; install_fps_state(&module); // exact required extent is valid
    pid_t child = fork(); assert(child >= 0);
    if (!child) {
        struct rlimit lim = {0, 0}; setrlimit(RLIMIT_CORE, &lim);
        module.size--; install_fps_state(&module); _exit(0);
    }
    int status; assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
static void *notifications(void *unused) {
    (void)unused;
    for (unsigned i = 0; i < 10000; i++) dm_game_aspect_drawable(i & 1 ? 2520 : 3840, 1080);
    return NULL;
}
static void *guest_acknowledgements(void *unused) {
    (void)unused;
    for (unsigned i = 0; i < 10000; i++) acknowledge();
    return NULL;
}
static void concurrency(void) {
    reset(); install_fps_state(&module); dm_fps_state_apply();
    pthread_t writer, guest;
    assert(!pthread_create(&writer, NULL, notifications, NULL));
    assert(!pthread_create(&guest, NULL, guest_acknowledgements, NULL));
    for (unsigned i = 0; i < 10000; i++) {
        float f = dm_game_aspect(); assert(f == 0 || f == 2520.0f/1080 || f == 3840.0f/1080);
    }
    assert(!pthread_join(writer, NULL)); assert(!pthread_join(guest, NULL));
    acknowledge(); assert(dm_game_aspect() == desired());
}
// What holds DM_RENDER_SCALE (dm_fps_state_apply): the stub's largest scale where it names one above 1 (RENDER_SCALE n), the profile's render_scale_max, and on a build whose
// stubs grow the render buffer pool (RENDER_POOL) what the platform can reserve; the smaller where two apply
static float scale_for(const char *asked, uint32_t stub, uint64_t pool) {
    reset(); g_render_scale_max = 1.0f; dm_mem_extra = 0;
    module.render_scale_max = stub; module.render_pool = pool;
    install_fps_state(&module); setenv("DM_RENDER_SCALE", asked, 1); dm_fps_state_apply(); unsetenv("DM_RENDER_SCALE");
    float sc; memcpy(&sc, image + 0x8010, sizeof sc); return sc;
}
static void render_scale_ceilings(void) {
    const float profile = DM_GAME_RENDER_SCALE_MAX;   // 1.0 unless the profile measured its pools
    char top[16]; snprintf(top, sizeof top, "%.2f", (double)profile);
    assert(scale_for("0.75", 1, 0) == 0.75f && scale_for("0.4", 1, 0) == 1.0f);
    assert(scale_for(top, 1, 0) == strtof(top, NULL) && scale_for("3.0", 1, 0) == (profile >= 3.0f ? 3.0f : 1.0f));   // a stub that only lowers the size: the profile's ceiling
    assert(scale_for("3.0", 3, 0) == (profile > 1.0f && profile < 3.0f ? 1.0f : 3.0f));   // a stub that takes 3 (Hades II): 3, or the profile's where it names a smaller one
    assert(scale_for("2.0", 3, 0) == (profile > 1.0f && profile < 2.0f ? 1.0f : 2.0f));
    assert(scale_for("3.5", 3, 0) == 1.0f);
    float pool = render_pool_max(0x7300000);   // 3.0 on the Mac
    if (pool >= 2.0f) {
        assert(scale_for("2.0", 1, 0x7300000) == 2.0f && dm_mem_extra);   // the pool grows: the platform's ceiling, whatever the profile's
        assert(scale_for("2.0", 2, 0x7300000) == 2.0f && scale_for("2.5", 2, 0x7300000) == 1.0f && !dm_mem_extra);   // and the stub's where it names a smaller one
    }
    g_render_scale_max = 1.0f; dm_mem_extra = 0;
}
int main(void) {
    render_scale_ceilings();
    preload_ordering(); unsupported(); layout(); concurrency();
    verified = "277178b7dba1b6d4949a84778d4abc58";
    preload_ordering(); unsupported(); layout(); concurrency();
    puts("BOTW/TOTK camera aspect: build/capability gates, settings, defaults, bounds and concurrency: OK");
    return 0;
}
