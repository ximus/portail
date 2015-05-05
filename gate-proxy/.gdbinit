define reset
  monitor reset
  flushregs
end

define hook-quit
  set confirm off
end

define prog
  reset
  symbol-file bin/wizzimote/gate-proxy.elf
  monitor prog bin/wizzimote/gate-proxy.elf
  reset
end

target extended-remote localhost:2000
symbol-file bin/wizzimote/gate-proxy.elf

reset
#break laser.c:48
#break laser.c:65
#break laser.c:35
#break gate_telemetry.c:94
#break gate_telemetry.c:70
#break gate_telemetry.c:86