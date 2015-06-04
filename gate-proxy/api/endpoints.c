#include "endpoints/well_known_core.c"
#include "endpoints/gate.c"

const coap_endpoint_t endpoints[] =
{
    {COAP_METHOD_GET, handle_get_well_known_core, &path_well_known_core, "ct=40"},
    {COAP_METHOD_GET, handle_get_gate, &path_gate, "ct=42"},
    // {COAP_METHOD_PUT, handle_put_gate, &path_gate, NULL},
    {(coap_method_t)0, NULL, NULL, NULL}
};