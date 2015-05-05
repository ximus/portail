define reset
  monitor reset
  flushregs
end

define hook-quit
  set confirm off
end

define prog
  reset
  monitor prog bin/wizzimote/gateway.elf
  symbol-file bin/wizzimote/gateway.elf
  reset
end

target extended-remote localhost:2000
symbol-file bin/wizzimote/gateway.elf