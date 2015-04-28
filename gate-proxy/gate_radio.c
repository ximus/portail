#include "cc110x_legacy.h"
#include "radio.h"
#include "gate_telemetry.h"
#include "timex.h"
#include "vtimer.h"
#include "stdint.h"


// TODO: 3 tries is just a spur of the moment number
#define NUM_BROADCAST_TRIES  (3)


extern uint8_t gate_radio_config[];
static char old_conf[CC1100_CONF_SIZE];

static const uint8_t gate_toggle_code[] = {0xb2, 0xd9, 0x2d, 0x92, 0x58};
// static const uint8_t gate_toggle_code[] = {0x58, 0x92, 0x2d, 0xd9, 0xb2};


static void broadcast_code(void)
{
    // MCSM1[TXOFF_MODE]=01 meaning after tx radio will go back to FSTXON state
    cc110x_writeburst_reg(RF_TXFIFOWR, gate_toggle_code, sizeof(gate_toggle_code));
    cc110x_strobe(RF_STX);
}

static void wait_for_broadcast_end(void)
{
    /* Wait for GDO2 to be set -> sync word transmitted */
    while (cc110x_get_gdo2() == 0);
    /* Wait for GDO2 to be cleared -> end of packet */
    while (cc110x_get_gdo2() != 0);
}

static void sleep_until_gate_reacts(void)
{
    vtimer_sleep(timex_from_uint64(telemetry.reactivity_us));
}

static void store_conf(void)
{
    cc110x_readburst_reg(0x0, old_conf, CC1100_CONF_SIZE);
    // cc110x_read_reg(CC1100_PATABLE, pa_table[pa_table_index]);
}

static void restore_conf(void)
{
    cc110x_writeburst_reg(0x00, old_conf, CC1100_CONF_SIZE);
    // cc110x_write_reg(CC1100_PATABLE, pa_table[pa_table_index]);
}

uint8_t old_radio_state;

static void prepare(void)
{
    radio_claim();

    // unsigned int cpsr = disableIRQ();

    old_radio_state = radio_state;

    store_conf();
    cc110x_writeburst_reg(0x00, gate_radio_config, CC1100_CONF_SIZE);

    // not sure I need this:
    // cc110x_strobe(CC1100_SFSTXON);
}

static void finish(void)
{
    // back to idle
    cc110x_strobe(RF_SIDLE);
    radio_state = RADIO_IDLE;

    restore_conf();

    /* Have to put radio back to WOR/RX if old radio state
     * was WOR/RX, otherwise no action is necessary */
    if ((old_radio_state == RADIO_WOR) || (old_radio_state == RADIO_RX)) {
        cc110x_switch_to_rx();
    }

    radio_release();
    // restoreIRQ(cpsr);
}


// Main function defining gate rf manipulation workflow
uint8_t gate_radio_strobe(gate_state_t target_state)
{
    if (gate_get_state() == target_state) {
        return 0;
    }

    int ret = 1;
    prepare();

    for (int i = 0; i < NUM_BROADCAST_TRIES; ++i)
    {
        broadcast_code();
        wait_for_broadcast_end();
        sleep_until_gate_reacts();
        if (gate_get_state() == target_state)
        {
            ret = 0;
            break;
        }
    }

    finish();
    return ret;
}

void gate_radio_send_code(void)
{
    prepare();
    broadcast_code();
    wait_for_broadcast_end();
    finish();
}

uint8_t gate_radio_learn_code(void)
{
    radio_claim();
    return 0;
}