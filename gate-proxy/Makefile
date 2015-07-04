# Set the name of your application:
export APPLICATION = gate-proxy

# If no BOARD is found in the environment, use this default:
export BOARD ?= wizzimote

# This has to be the absolute path to the RIOT base directory:
export RIOTBASE ?= $(CURDIR)/../../RIOT

export USE_MSPGCC = 1

# Uncomment these lines if you want to use platform support from external
# repositories:
#export RIOTCPU ?= $(CURDIR)/../../RIOT/thirdparty_cpu
#export RIOTBOARD ?= $(CURDIR)/../../RIOT/thirdparty_boards

# Uncomment this to enable scheduler statistics for ps:
# CFLAGS += -DSCHEDSTATISTICS
CFLAGS += -DSCHED_TEST_STACK


ASFLAGS += -gdwarf-2
CFLAGS += -gdwarf-2

# avoid "unknown type name 'time_t'"
CFLAGS += -DCBOR_NO_CTIME

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# laser value stream facility
# CFLAGS += -DLASER_STREAM

# Change this to 0 show compiler invocation lines by default:
export QUIET ?= 0

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ps
USEMODULE += vtimer
USEMODULE += cc110x_legacy
USEMODULE += cbor
USEMODULE += auto_init
# USEMODULE += config

# USEPKG += libcoap

#export INCLUDES += -Iapplication_include

DIRS 			+= api
USEMODULE += api

DIRS 			+= api/endpoints
USEMODULE += endpoints

DIRS 			+= $(CURDIR)/../shared
INCLUDES  += -I$(CURDIR)/../shared
USEMODULE += shared

DIRS 	    += $(CURDIR)/../shared/pkg/coap
INCLUDES  += -I$(CURDIR)/../shared/pkg/coap
USEMODULE += coap



# Specify custom dependencies for your application here ...
# export APPDEPS = app_data.h config.h

include $(RIOTBASE)/Makefile.include

# ... and define them here (after including Makefile.include,
# otherwise you modify the standard target):
#proj_data.h: script.py data.tar.gz
# ./script.py

print-%: ; @echo $*=$($*)
