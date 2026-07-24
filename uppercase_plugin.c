/* uppercase_plugin.c
 *
 * Example oshot plugin: reads the OCR output, uppercases it on
 * button press, writes the result back into the value store.
 * Also demonstrates the preferences tab: a toggle for automatic
 * transform on OCR completion, and an optional output prefix.
 *
 * Build (Linux example):
 *   cc -shared -fPIC uppercase_plugin.c -o uppercase_plugin.so
 */

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "oshot_plugin.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PLUGIN_ABI_VERSION 1u

/* ------------------------------------------------------------------ */
/* Plugin preferences. Kept as a plain POD struct so render_preferences */
/* can detect "did anything change" via field-by-field comparison,      */
/* without the host needing to know this struct's shape at all.         */
/* ------------------------------------------------------------------ */
typedef struct
{
    bool auto_uppercase;   /* run the transform automatically in on_ocr_done */
    char prefix[64];       /* optional text prepended to the transformed output */
} uppercase_prefs_t;

static bool prefs_equal(const uppercase_prefs_t* a, const uppercase_prefs_t* b)
{
    return a->auto_uppercase == b->auto_uppercase && strcmp(a->prefix, b->prefix) == 0;
}

/* ------------------------------------------------------------------ */
/* Plugin-private state. Allocated in init(), freed in destroy().     */
/* The host never sees the layout of this struct -- it's only ever    */
/* handled as an opaque void*.                                        */
/* ------------------------------------------------------------------ */
typedef struct
{
    char buf[4096]; /* owns the transformed text */
    int  has_error;

    uppercase_prefs_t prefs_saved;    /* committed values; source of truth */
    uppercase_prefs_t prefs_working;  /* live edits from render_preferences,
                                        * not visible to the host until Save */
} uppercase_state_t;

/* ------------------------------------------------------------------ */
/* Helper: log an error through the host, not stderr directly --      */
/* keeps plugin output going through oshot's own logging sink rather  */
/* than an unmanaged console write (matters on Windows GUI builds     */
/* where there may be no attached console at all).                    */
/* ------------------------------------------------------------------ */
static void log_error(uppercase_state_t* s, const char* msg)
{
    s->has_error = 1;
    oshot_log(OSHOT_LOG_ERROR, oshot_str_borrow(msg));
}

/* Bounds-checked, always-null-terminated copy out of a borrowed
 * oshot_str_t. oshot_str_t is {ptr, len}, not guaranteed to be
 * null-terminated -- never pass res->text.p straight to "%s". */
static void copy_bounded(char* dst, size_t dst_size, oshot_str_t src)
{
    size_t n = src.len < dst_size - 1 ? src.len : dst_size - 1;
    memcpy(dst, src.p, n);
    dst[n] = '\0';
}

/* ----------------------------------------------------------------- */
/* init / destroy                                                    */
/* ----------------------------------------------------------------- */
static void* uppercase_init()
{
    if (oshot_get_abi_version() != PLUGIN_ABI_VERSION)
    {
        /* Can't log meaningfully yet if the ABI mismatch is bad enough
         * that the struct layout itself might differ -- refuse to load. */
        return NULL;
    }

    uppercase_state_t* s = calloc(1, sizeof(uppercase_state_t));
    if (!s)
        return NULL;

    s->buf[0] = '\0';

    /* Load committed preferences. oshot_config_get_* is implicitly
     * scoped to this plugin's namespace on the host side. */
    s->prefs_saved.auto_uppercase = oshot_config_get_bool("auto-uppercase", false);

    oshot_str_t prefix = oshot_config_get_string("prefix", oshot_str_borrow(""));
    copy_bounded(s->prefs_saved.prefix, sizeof(s->prefs_saved.prefix), prefix);
    oshot_str_free(&prefix);

    s->prefs_working = s->prefs_saved;  /* working starts equal to saved: nothing dirty yet */

    oshot_log(OSHOT_LOG_INFO, oshot_str_borrow("uppercase plugin initialized"));
    return s;
}

static void uppercase_destroy(void* state)
{
    free(state);
}

/* ------------------------------------------------------------------ */
/* The actual transform.                                              */
/* ------------------------------------------------------------------ */
static void do_uppercase(uppercase_state_t* s, oshot_str_t text)
{
    char upper[4096 - 64]; /* leave room for prefix; generous enough for the demo */

    if (text.len >= sizeof(upper))
    {
        log_error(s, "uppercase plugin: text too large for local buffer");
        return;
    }

    for (size_t i = 0; i < text.len; ++i)
        upper[i] = (char)toupper((unsigned char)text.p[i]);
    upper[text.len] = '\0';

    snprintf(s->buf, sizeof(s->buf), "%s%s", s->prefs_saved.prefix, upper);
    oshot_set_text("uppercase_out", oshot_str_borrow(s->buf));
    s->has_error = 0;
}

static void do_uppercase_from_host_text(uppercase_state_t* s)
{
    oshot_str_t ocr;

    if (!oshot_get_text("ocr_output", &ocr))
    {
        log_error(s, "uppercase plugin: ocr_output is empty, nothing to transform");
        return;
    }

    do_uppercase(s, ocr);
    oshot_str_free(&ocr);
}

/* ------------------------------------------------------------------ */
/* on_ocr_done: clear stale output whenever fresh OCR text arrives,   */
/* or auto-transform immediately if the user enabled that preference. */
/* ------------------------------------------------------------------ */
static void uppercase_on_ocr_done(void* state, const oshot_ocr_result_t* res)
{
    uppercase_state_t* s = state;
    s->has_error          = 0;

    if (s->prefs_saved.auto_uppercase)
        do_uppercase(s, res->text);
    else
        s->buf[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* render: called once per frame while this plugin's section is drawn */
/* on the main overlay.                                               */
/* ------------------------------------------------------------------ */
static void uppercase_render(void* state)
{
    uppercase_state_t* s = state;

    oshot_capture_t cap = oshot_get_capture();
    if (cap.rgba)
        igTextDisabled("Capture: %dx%d", cap.w, cap.h);
    else
        igTextDisabled("No Capture?");

    igInputTextMultiline("##uppercase_out", s->buf, sizeof(s->buf),
                          (ImVec2_c){ -1.0f, 80.0f },
                          ImGuiInputTextFlags_ReadOnly, NULL, NULL);

    if (s->has_error)
        igTextColored((ImVec4_c){ 1.0f, 0.4f, 0.4f, 1.0f }, "Transform failed, see log.");

    if (igButton("Uppercase OCR Output", (ImVec2_c){0,0}))
        do_uppercase_from_host_text(s);
}

/* ------------------------------------------------------------------ */
/* render_preferences: draws the plugin's own tab in the Preferences  */
/* window. Edits prefs_working only -- nothing here touches config    */
/* until on_save_preferences commits it. Returns true if prefs_working */
/* differs from prefs_saved, so the host knows this plugin is dirty.  */
/* ------------------------------------------------------------------ */
static bool uppercase_render_preferences(void* state)
{
    uppercase_state_t* s = state;

    igCheckbox("Auto-uppercase on OCR completion", &s->prefs_working.auto_uppercase);

    igInputText("Output prefix", s->prefs_working.prefix, sizeof(s->prefs_working.prefix),
                ImGuiInputTextFlags_None, NULL, NULL);
    igTextDisabled("Prepended to the transformed text, e.g. \">> \"");

    return !prefs_equal(&s->prefs_working, &s->prefs_saved);
}

/* ------------------------------------------------------------------ */
/* on_save_preferences: commit prefs_working -> prefs_saved and push  */
/* it into the host-persisted config.                                  */
/* ------------------------------------------------------------------ */
static void uppercase_on_save_preferences(void* state)
{
    uppercase_state_t* s = state;

    s->prefs_saved = s->prefs_working;

    oshot_config_set_bool("auto-uppercase", s->prefs_saved.auto_uppercase);
    oshot_config_set_string("prefix", oshot_str_borrow(s->prefs_saved.prefix));
}

/* ------------------------------------------------------------------ */
/* on_discard_preferences: throw away edits, revert to last committed. */
/* No config calls needed -- nothing was written to the host yet.     */
/* ------------------------------------------------------------------ */
static void uppercase_on_discard_preferences(void* state)
{
    uppercase_state_t* s = state;
    s->prefs_working      = s->prefs_saved;
}

/* ------------------------------------------------------------------ */
/* Plugin descriptor + the one exported symbol the host looks up.     */
/* ------------------------------------------------------------------ */
static oshot_plugin_t plugin_desc = {
    .abi_version             = PLUGIN_ABI_VERSION,
    .name                    = "Uppercase Transform",
    .id                      = "com.test.uppercas_transform",
    .init                    = uppercase_init,
    .destroy                 = uppercase_destroy,
    .render                  = uppercase_render,
    .on_ocr_done             = uppercase_on_ocr_done,
    .render_preferences      = uppercase_render_preferences,
    .on_save_preferences     = uppercase_on_save_preferences,
    .on_discard_preferences  = uppercase_on_discard_preferences,
};

oshot_plugin_t* oshot_host_get_plugin(void)
{
    return &plugin_desc;
}
