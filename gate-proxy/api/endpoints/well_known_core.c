#include <string.h>
#include "coap.h"

#define RSP_LEN (60)
static char rsp[RSP_LEN] = "";

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

/**
 * this method will overflow `rsp` if it isn't big engough
 * TODO: rewite it so that it returns an error if it runs out of space
 * TODO: any way to generate this string at compile time?
 * @param ep list of endpoints
 */
void well_known_core_build(const coap_endpoint_t *ep)
{
    uint16_t len = RSP_LEN;
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