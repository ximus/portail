#include "coap.h"

const coap_endpoint_path_t path_get_temp = {2, {".well-known", "core"}};

int handle_get_temp(
    coap_rw_buffer_t    *scratch,
    const coap_packet_t *inpkt,
    coap_packet_t       *outpkt,
    uint8_t             id_hi,
    uint8_t             id_lo
) {
    return;
}