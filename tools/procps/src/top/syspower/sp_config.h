/*
 * Settings for building syspower's code inside top.
 * Included automatically before every syspower file (see Makefile.am).
 */
#ifndef SP_CONFIG_H__
#define SP_CONFIG_H__

// program name used in syspower's error messages
#define NAME "top-syspower"

// send syspower's error messages to a log file instead of the terminal,
// where they would scramble top's screen
void sp_log(const char *fmt, ...);
#define CFG_PRINTF_ERR(...) sp_log(__VA_ARGS__)

#endif
