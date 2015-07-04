#!/usr/bin/env ruby

require 'csv'
require 'pry'
require 'terminal-table'

path = ARGV[0]
if !path || path.size == 0
  puts("usage: stat-map-file.rb [path]")
  exit(1)
end

lines = File.read(path).split("\n")

bss_items_header = ['id', 'size', 'path']
bss_items = []

# parse bss items
lines.each_with_index do |line, i|
  match = line.match(/^\s*\.bss\.(\w*)/)
  next unless match
  id = match[1].chomp
  next if id.empty?

  match = lines[i+1].match(/\s*0x(\w*)\s*0x(\w*)\s*(.*)/)
  next unless match
  size = match[2]
  path = match[3]
  next if size.empty?
  # hex string to int
  size = size.to_i(16)

  bss_items << [id, size, path]
end

# sort by size
sorted = bss_items.sort do |item1, item2|
  item2[1] <=> item1[1]
end

puts Terminal::Table.new(rows: [bss_items_header].concat(sorted))