/**
* @file tick_logger.h
 *
 * @brief Pipeline tick timing for Linux builds (Pi 4, Pi Zero 2 W).
 *
 * Writes tbci_ticks_<label>_<hostname>_<time>.csv with the same columns the
 * Pico collector produces (host_time_s,kind,duration_us), so one analysis
 * script covers every device. kind is R (regular tick) or D (decision tick).
 *
 * Ticks are kept in memory and written in blocks, never inside the timed call.
 * The file is closed automatically on exit, including Ctrl+C.
 */

#ifndef TICK_LOGGER_H
#define TICK_LOGGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Opens the log file. Ticks in the first warmupSeconds after the first logged
 * tick are skipped (use the filter stabilization delay, plus a margin).
 */
void initTickLogger(const char *label, double warmupSeconds);

/** Monotonic clock in nanoseconds (CLOCK_MONOTONIC_RAW). */
uint64_t tickLoggerNowNs(void);

/** Records one pipeline tick. Does nothing if the logger is not initialized. */
void logTick(uint64_t startNs, uint64_t endNs, bool isDecision);

/** Writes what is left and closes the file. Safe to call more than once. */
void closeTickLogger(void);

#endif