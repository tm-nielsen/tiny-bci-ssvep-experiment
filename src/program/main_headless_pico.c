/**
* @file main_headless_pico.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Minimal TinyBCI SSVEP for Raspberry Pico 2W on-device build.
 *
 *
 * Epochs are printed over USB/UART stdio as they arrive. Commands are
 * polled non-blockingly each tick (no OS threads on bare-metal).
 *
 * ## Build
 *
 *   cmake --build . --target headless_tiny_bci_ssvep_experiment_pico
 *
 * ## Flash
 *
 *   Copy the resulting .uf2 to the Pico in BOOTSEL mode, or use picotool/OpenOCD.
 */


# include "pico/stdlib.h"
# include "pico/cyw43_arch.h"

# include "pipeline.h"
# include "program/constants.h"
# include "data/unicorn_eeg_source.h"
# include "serial/serial.h"

# define UNICORN_NAME      "UN-2024.09.14"
# define IDLE_TIMEOUT_MS   8
# define SELF_TEST_TARGET  1   // starts a trial without a presenter; 0 = off

static void setLed(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on); }

static void halt(const char *message)
{
    printf("FATAL: %s\n", message);
    for (;;) { setLed(true); sleep_ms(100); setLed(false); sleep_ms(100); }
}

// --- the same tick as headless on the Pi, without locks or LSL

static uint64_t maxTickUs = 0;

static void updateProgram(void)
{
    waitForUnicornEEGSourceData(IDLE_TIMEOUT_MS);
    updateUnicornEEGSource();

    if (!isUnicornEEGSourceConnected()) halt("Unicorn disconnected");

    uint64_t start = time_us_64();
    if (updateTinyBCIPipeline() != 0) halt("pipeline update failed");
    uint64_t elapsed = time_us_64() - start;
    if (elapsed > maxTickUs) maxTickUs = elapsed;
}

static void awaitFilterStabilization(void)
{
    printf("Waiting for filter to settle...\n");
    uint64_t end = time_us_64() + (uint64_t)(FILTER_STABILIZATION_DELAY * 1000000.0f);
    while (time_us_64() < end) updateProgram();
    printf("Filter settled.\n");
}

int main(void)
{
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);   // dev only: wait for a terminal
    printf("TinyBCI Pico engine\n");

    if (cyw43_arch_init()) halt("radio init failed");
    setLed(true);

    printf("Connecting to %s...\n", UNICORN_NAME);
    connectUnicornEEGSource(UNICORN_NAME, 0);
    setLed(false);

    if (initializeTinyBCIPipeline(
            FREQUENCIES,
            getUnicornEEGSourceChannelCount(),
            getUnicornEEGSourceSampleRate()) != 0) halt("pipeline init failed");
    if (startTinyBCIPipeline() != 0) halt("pipeline start failed");
    printf("Pipeline running.\n");

    awaitFilterStabilization();

# if SELF_TEST_TARGET
    pushTrigger(SELF_TEST_TARGET);   // sliding mode needs a trial to start
# endif

    uint64_t nextReport = time_us_64() + 1000000;
    for (;;)
    {
        updateProgram();

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            printf("%llu | inference %d (%.0f%%)\n",
                time_us_64(), inference.predictedLabel, inference.confidence * 100.0f);
        }

        if (time_us_64() >= nextReport)
        {
            printf("max pipeline tick: %llu us\n", maxTickUs);
            maxTickUs = 0;
            nextReport += 1000000;
        }
    }
}