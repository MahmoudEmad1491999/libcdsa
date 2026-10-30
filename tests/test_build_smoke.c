#include <stddef.h>
#include <stdlib.h>

/*
 * Smoke test: verifies that the library links, the compiler flags compile
 * without errors, and the sanitizer-annotated binary exits cleanly.
 *
 * Actual functional tests land in later phases once modules exist.
 */
int main(void)
{
    return EXIT_SUCCESS;
}
