#!/usr/bin/python3

import babeltrace
import numpy
import sys, os

trace_path = sys.argv[1]
bytes = 4*(1<<int(sys.argv[2]))

trace_collection = babeltrace.TraceCollection()

trace_namp = os.path.join(trace_path, "namp")

trace_collection.add_trace(trace_namp, 'ctf')

slot_starts = {}
durations = {}
transfers = {}
ingress_ops = {}
completions = {}
retries = {}

for event in trace_collection.events:
    if event.name == "slot_active":
        tile = event["tile_id"]
        slot = event["slot"]
        if event["state"] == 1:
            if tile not in slot_starts:
                slot_starts[tile] = {}
                retries[tile] = {}
            if slot not in slot_starts[tile]:
                slot_starts[tile][slot] = []
                retries[tile][slot] = []
            retries[tile][slot].append(-1)
            slot_starts[tile][slot].append(event.cycles/2)
        else:
            if tile not in durations:
                durations[tile] = {}
            if slot not in durations[tile]:
                durations[tile][slot] = []
            durations[tile][slot].append(event.cycles/2 - slot_starts[tile][slot][-1])
    elif event.name == "egress_end":
        tile = event["tile_id"]
        slot = event["slot"]
        if event["state"] == 4:
            retries[tile][slot][-1] += 1
    elif event.name == "ingress_start":
        domain = event["domain"]
        slot = event["slot"]
        if domain not in ingress_ops:
            ingress_ops[domain] = {}
        ingress_ops[domain][slot] = event["state"]
    elif event.name == "ingress_end":
        domain = event["domain"]
        slot = event["slot"]
        if ingress_ops[domain][slot] == 33:
            if domain not in completions:
                completions[domain] = {}
            if slot not in completions[domain]:
                completions[domain][slot] = []
            completions[domain][slot].append(event.cycles/2)

gaps = []
for t in durations:
    for s in durations[t]:
        gaps += durations[t][s]

gap_avg = sum(gaps)/len(gaps)
gap_min = min(gaps)
gap_max = max(gaps)

tp_min = bytes/gap_max
tp_avg = bytes/gap_avg
tp_max = bytes/gap_min

latencies = []
for t in durations:
    for s in durations[t]:
        latencies += list(numpy.subtract(completions[t][s],slot_starts[t][s]))

lat_avg = sum(latencies)/len(latencies)
lat_min = min(latencies)
lat_max = max(latencies)

retry = []
for t in retries:
    for s in retries[t]:
        retry += retries[t][s]

retry_avg = sum(retry)/len(retry)

print("{} {} {} {} {:.4f} {:.4f} {:.4f} {} {} {} {}" .format(bytes, gap_min, gap_avg, gap_max, tp_min, tp_avg, tp_max, lat_min, lat_avg, lat_max, retry_avg))
