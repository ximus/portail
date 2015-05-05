define reset
  monitor reset
  flushregs
end

define hook-quit
  set confirm off
end

target extended-remote localhost:2000
symbol-file bin/wizzimote/portail.elf
reset
monitor prog bin/wizzimote/portail.elf
reset

continue