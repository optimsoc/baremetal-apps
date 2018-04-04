#!/usr/bin/python3

import babeltrace
import numpy
import sys

trace_collection = babeltrace.TraceCollection()

# get the trace path from the first command line argument
for path in sys.argv[1:]:
    trace_collection.add_trace(path, 'ctf')

tf = 0
tt = 15

begins = []
senddone = []
ends = []

for event in trace_collection.events:
    if event["id"] == 0x400 and event["cpu_id"] == tf:
        begins.append(event.cycles)
    elif event["id"] == 0x401 and event["cpu_id"] == tf:
        senddone.append(event.cycles)
    elif event["id"] == 0x411 and event["cpu_id"] == tt:
        ends.append(event.cycles)

os = numpy.subtract(senddone, begins)
lat = numpy.subtract(ends, begins)
gap = numpy.diff(begins)

print(os)
print(lat)
print(gap)
