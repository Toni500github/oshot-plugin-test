/* Unit tests for src/plugin.c, run against oshot's mock host
 * (oshot/oshot_mock_host.h in the SDK) instead of oshot itself:
 *
 *   cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build
 *
 * The mock keeps everything in memory. The test sets config values, the
 * capture and the OCR text, sends the events oshot would send, clicks widgets
 * by their labels, and reads back what the plugin did.
 */

#include <stdio.h>
#include <string.h>

#include "oshot/oshot_mock_host.h"

static int failures = 0;

#define CHECK(cond)                                                                  \
    do                                                                               \
    {                                                                                \
        if (!(cond))                                                                 \
        {                                                                            \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

#define CHECK_STR(actual, expected)                                                                                  \
    do                                                                                                               \
    {                                                                                                                \
        const char* a_ = (actual);                                                                                   \
        if (!a_ || strcmp(a_, (expected)) != 0)                                                                      \
        {                                                                                                            \
            fprintf(                                                                                                 \
                stderr, "%s:%d: got \"%s\", expected \"%s\"\n", __FILE__, __LINE__, a_ ? a_ : "(null)", (expected)); \
            ++failures;                                                                                              \
        }                                                                                                            \
    } while (0)

/* A loaded plugin in an open capture with a 4x3 selection */
static oshot_mock_t* start(void)
{
    static const uint8_t pixels[4 * 3 * 4] = { 0 };

    oshot_mock_t* m = oshot_mock_new();
    oshot_mock_set_capture(m, 4, 3, pixels);
    if (!oshot_mock_load(m, oshot_plugin_entry()))
    {
        fprintf(stderr, "the plugin didn't load\n");
        exit(1);
    }
    oshot_mock_capture_start(m);
    return m;
}

static void test_button(void)
{
    oshot_mock_t* m = start();
    oshot_mock_ocr(m, "hello world", NULL, 0);
    CHECK_STR(oshot_mock_text(m, OSHOT_TEXT_OCR), "hello world");

    oshot_mock_render(m);
    CHECK(oshot_mock_drew(m, "button Uppercase the OCR output"));

    oshot_mock_click(m, "Uppercase the OCR output");
    oshot_mock_render(m);
    CHECK_STR(oshot_mock_text(m, OSHOT_TEXT_OCR), "HELLO WORLD");

    oshot_mock_render(m);
    CHECK(oshot_mock_drew(m, "text Selection: 4x3"));

    /* A new capture forgets the selection */
    oshot_mock_capture_end(m);
    oshot_mock_capture_start(m);
    oshot_mock_render(m);
    CHECK(!oshot_mock_drew(m, "Selection:"));

    CHECK(oshot_mock_log_count(m, OSHOT_LOG_WARN) == 0);
    oshot_mock_free(m);
}

static void test_settings(void)
{
    oshot_mock_t* m = start();

    /* Off by default */
    CHECK_STR(oshot_mock_ocr(m, "abc", NULL, 0), "abc");

    /* As if the person changed them in Preferences and saved */
    oshot_mock_set_bool(m, OSHOT_STORE_CONFIG, "auto-uppercase", true);
    oshot_mock_set_string(m, OSHOT_STORE_CONFIG, "prefix", "> ");
    oshot_mock_save_preferences(m);
    CHECK_STR(oshot_mock_ocr(m, "abc", NULL, 0), "> ABC");

    /* Nothing read, so on_ocr_done() gets empty text and leaves it alone */
    CHECK_STR(oshot_mock_ocr(m, "", NULL, 0), "");

    oshot_mock_free(m);
}

static void test_old_oshot(void)
{
    /* The plugin refuses to load in an oshot from before the widgets */
    oshot_mock_t* m = oshot_mock_new();
    oshot_mock_set_host_size(m, offsetof(oshot_host_t, ui_text));
    CHECK(!oshot_mock_load(m, oshot_plugin_entry()));
    oshot_mock_free(m);
}

int main(void)
{
    test_button();
    test_settings();
    test_old_oshot();

    /* Every host string and image the plugin got was freed */
    CHECK(oshot_mock_live_allocations() == 0);

    if (failures)
        fprintf(stderr, "%d check(s) failed\n", failures);
    return failures ? 1 : 0;
}
