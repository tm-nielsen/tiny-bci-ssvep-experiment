/**
* @file main_headless_pico.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Minimal TinyBCI SSVEP for Raspberry Pico 2W on-device build.
 *
 *
 * Headless TinyBCI engine on the Pico 2 W.
 *
 * Same flow as main_headless.c on the Pi, with bci_link over USB instead of
 * LSL: connect the headset, run the pipeline, take triggers from any client,
 * stream EEG rows, inference rows, trigger echoes and status to every client.
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
# include "inference_logger.h"
# include "microsecond_timer.h"
# include "program/constants.h"
# include "data/unicorn_eeg_source.h"
# include "serial/serial.h"

# include "engine/link_hub.h"
# include "engine/link_outputs.h"

# define UNICORN_NAME      "UN-2024.09.14"
# define IDLE_TIMEOUT_MS   8
# define STATUS_PERIOD_US  1000000
# define SELF_TEST_TARGET  0   // >0: push this trigger after stabilization (no presenter needed)

static BCIEngineState engineState = BCI_ENGINE_BOOTING;
static bool pipelineRunning = false;
static uint32_t maxTickUs = 0;

// --- link: incoming messages and periodic status

static void onTriggerReceived(uint16_t code)
{
    if (!pipelineRunning) return; // nothing to align it with yet
    uint64_t timestamp = pushTrigger(code);
    sendTriggerEchoToLink(code, timestamp);
    if (code == TRIAL_END_CODE) return;
    notifyInferenceLoggerOfNewTarget(code - 1, timestamp); // presenter sends target + 1
}

static void handleLinkMessage(const BCILinkMessage *message)
{
    switch (message->type)
    {
        case BCI_LINK_HELLO:
        {
            BCILinkHello hello;
            readBCILinkHello(message, &hello);
            printf("link: client connected (role %u, protocol v%u)\n", hello.role, hello.protocolVersion);
            sendStatusToLink(engineState, maxTickUs);
            break;
        }
        case BCI_LINK_TRIGGER:
        {
            BCILinkTrigger trigger;
            readBCILinkTrigger(message, &trigger);
            onTriggerReceived(trigger.code);
            break;
        }
        case BCI_LINK_PING:
        {
            BCILinkTimeSync timeSync;
            readBCILinkTimeSync(message, &timeSync);
            timeSync.engineTimeUs = getCurrentMicrosecondTimestamp();
            broadcastLinkMessage(makeBCILinkPong(timeSync));
            break;
        }
        default:
            break;
    }
}

// Runs whenever the firmware waits (Bluetooth, idle EEG) and between ticks.
static void serviceLink(void)
{
    static uint64_t nextStatusUs = 0;

    updateLinkHub();

    BCILinkMessage message;
    while (tryReceiveLinkMessage(&message)) handleLinkMessage(&message);

    uint64_t now = getCurrentMicrosecondTimestamp();
    if (now >= nextStatusUs)
    {
        sendStatusToLink(engineState, maxTickUs);
        maxTickUs = 0;
        nextStatusUs = now + STATUS_PERIOD_US;
    }
}

static void setLed(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on); }

static void halt(const char *message)
{
    printf("FATAL: %s\n", message);
    engineState = BCI_ENGINE_FAULT;
    for (;;)   // keep USB alive: status, logs and the 1200-baud BOOTSEL reset
    {
        serviceLink();
        setLed((time_us_32() / 100000) % 2);
    }
}

// --- pipeline tick: same shape as headless on the Pi

static void updateProgram(void)
{
    waitForUnicornEEGSourceData(IDLE_TIMEOUT_MS); // services the link while waiting
    updateUnicornEEGSource();

    if (!isUnicornEEGSourceConnected()) halt("Unicorn disconnected");

    uint64_t start = time_us_64();
    if (updateTinyBCIPipeline() != 0) halt("pipeline update failed");
    uint32_t elapsed = (uint32_t)(time_us_64() - start);
    if (elapsed > maxTickUs) maxTickUs = elapsed;

    serviceLink();
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
    initializeLinkHub();                       // USB first, so logs and status show up early
    setPicoSerialIdleCallback(&serviceLink);   // keep USB alive during Bluetooth waits

    if (cyw43_arch_init()) halt("radio init failed");
    setLed(true);

    engineState = BCI_ENGINE_CONNECTING_HEADSET;
    printf("Connecting to %s...\n", UNICORN_NAME);
    connectUnicornEEGSource(UNICORN_NAME, 0);
    setLed(false);

    if (initializeTinyBCIPipeline(
            FREQUENCIES,
            getUnicornEEGSourceChannelCount(),
            getUnicornEEGSourceSampleRate()) != 0) halt("pipeline init failed");
    installEEGLinkTap();
    if (startTinyBCIPipeline() != 0) halt("pipeline start failed");
    pipelineRunning = true;
    printf("Pipeline running.\n");

    engineState = BCI_ENGINE_STABILIZING;
    awaitFilterStabilization();
    engineState = BCI_ENGINE_READY;
    setLed(true);

# if SELF_TEST_TARGET
    onTriggerReceived(SELF_TEST_TARGET);
# endif

    for (;;)
    {
        updateProgram();

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            sendInferenceToLink(inference, timestamp);
            printf("%llu | inference %d (%.0f%%)\n",
                timestamp, inference.predictedLabel, inference.confidence * 100.0f);
        }
    }
}