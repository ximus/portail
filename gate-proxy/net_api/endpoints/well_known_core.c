#include <string.h>
#include "coap.h"

// TODO(max): this needs a thought, I lowered from 1500
// to get the .bss down
const uint16_t rsplen = 30;
static char rsp[30] = "";

const coap_endpoint_path_t path_well_known_core = {2, {".well-known", "core"}};

int handle_get_well_known_core(
    coap_rw_buffer_t    *scratch,
    const coap_packet_t *inpkt,
    coap_packet_t       *outpkt,
    uint8_t             id_hi,
    uint8_t             id_lo
) {
    return coap_make_response(
        scratch,
        outpkt,
        (const uint8_t *)rsp,
        strlen(rsp),
        id_hi,
        id_lo,
        &inpkt->tok,
        COAP_RSPCODE_CONTENT,
        COAP_CONTENTTYPE_APPLICATION_LINKFORMAT
    );
}


void well_known_core_build(const coap_endpoint_t *ep)
{
    uint16_t len = rsplen;
    int i;

    len--; // Null-terminated string

    while(NULL != ep->handler)
    {
        if (NULL == ep->core_attr) {
            ep++;
            continue;
        }

        if (0 < strlen(rsp)) {
            strncat(rsp, ",", len);
            len--;
        }

        strncat(rsp, "<", len);
        len--;

        for (i = 0; i < ep->path->count; i++) {
            strncat(rsp, "/", len);
            len--;

            strncat(rsp, ep->path->elems[i], len);
            len -= strlen(ep->path->elems[i]);
        }

        strncat(rsp, ">;", len);
        len -= 2;

        strncat(rsp, ep->core_attr, len);
        len -= strlen(ep->core_attr);

        ep++;
    }
}