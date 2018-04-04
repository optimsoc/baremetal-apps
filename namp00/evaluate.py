#!/usr/bin/python3

import babeltrace

import sys

trace_collection = babeltrace.TraceCollection()



# get the trace path from the first command line argument
for path in sys.argv[1:]:
    trace_collection.add_trace(path, 'ctf')

for event in trace_collection.events:
    print(event)
