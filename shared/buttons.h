#include "gpioint.h"

#define BUTTON_FLAGS (GPIOINT_DEBOUNCE | GPIOINT_FALLING_EDGE)


static inline on_button1(fp_irqcb callback)
{
    gpioint_set(1, BIT6, BUTTON_FLAGS, callback);
}

static inline on_button2(fp_irqcb callback)
{
    gpioint_set(1, BIT5, BUTTON_FLAGS, callback);
}

static inline on_button3(fp_irqcb callback)
{
    gpioint_set(1, BIT4, BUTTON_FLAGS, callback);
}