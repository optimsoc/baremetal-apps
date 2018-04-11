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
activity = {}
egressslot = {}
curegress = {}
egressutil = {}

for event in trace_collection.events:
    cycles = (event.cycles-1)/2
    if event.name == "slot_active":
        tile = event["tile_id"]
        slot = event["slot"]
        if event["state"] == 1:
            if tile not in slot_starts:
                activity[tile] = []
                slot_starts[tile] = {}
                retries[tile] = {}
            if slot not in slot_starts[tile]:
                slot_starts[tile][slot] = []
                retries[tile][slot] = []
            retries[tile][slot].append(-1)
            slot_starts[tile][slot].append(cycles)
        else:
            if tile not in durations:
                durations[tile] = {}
            if slot not in durations[tile]:
                durations[tile][slot] = []
            durations[tile][slot].append(cycles - slot_starts[tile][slot][-1])
        activity[tile].append((cycles,event["total"]))
    elif event.name == "egress_start":
        if tile not in egressslot:
            egressslot[tile] = {}
            egressutil[tile] = []
        if slot not in egressslot[tile]:
            egressslot[tile][slot] = []
        if event["state"] == 0:
            egressslot[tile][slot].append(0)
        curegress[tile] = cycles
        egressutil[tile].append([cycles,0])
    elif event.name == "egress_end":
        tile = event["tile_id"]
        slot = event["slot"]
        if event["state"] == 4:
            retries[tile][slot][-1] += 1
        egressslot[tile][slot][-1] += cycles - curegress[tile]
        egressutil[tile][-1][1] = cycles
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
            completions[domain][slot].append(cycles)

gaps = []
for t in durations:
    for s in durations[t]:
        gaps += durations[t][s][5:]

if len(gaps) == 0:
    gap_avg = 0 # CCM
    gap_min = 0
    gap_max = 0
else:
    gap_avg = sum(gaps)/len(gaps)
    gap_min = min(gaps)
    gap_max = max(gaps)

egressops = []
egutil = []
for t in egressslot:
    egsum = 0
    for u in egressutil[t][4:-5]:
        egsum += u[1] - u[0]
    egutil.append(egsum/(egressutil[t][-5][1] - egressutil[t][4][0]))
    for s in egressslot[t]:
        egressops += egressslot[t][s][5:]
egops_avg = sum(egressops)/len(egressops)
egops_min = min(egressops)
egops_max = max(egressops)
egutil_avg = sum(egutil)/len(egutil)

latencies = []
for t in completions:
    for s in completions[t]:
        latencies += list(numpy.subtract(completions[t][s][5:],slot_starts[t][s][5:]))

lat_avg = sum(latencies)/len(latencies)
lat_min = min(latencies)
lat_max = max(latencies)

retry = []
for t in retries:
    for s in retries[t]:
        retry += retries[t][s][5:]

retry_avg = sum(retry)/len(retry)

act_avgs = []
for t in activity:
    act_dur = activity[t][-1][0] - activity[t][5][0]
    last_time = activity[t][5][0]
    last_count = activity[t][5][1]
    act = 0
    for a in activity[t][6:]:
       act += ((a[0] - last_time)/act_dur) * last_count
       last_time = a[0]
       last_count = a[1]
    act_avgs.append(act)

act_avg = sum(act_avgs)/len(act_avgs)


trace_collection = babeltrace.TraceCollection()

trace_namp = os.path.join(trace_path, "sw")

trace_collection.add_trace(trace_namp, 'ctf')

current_slot = {}
start = {}
end_wait = {}
end = {}

for event in trace_collection.events:
    cycles = event.cycles/2
    if event.name == "event":
        tile = event["cpu_id"]
        id = event["id"]
        slot = event["value"]
        if id == 0x400:
            if tile not in start:
                start[tile] = {}
            if slot not in start[tile]:
                start[tile][slot] = []
            start[tile][slot].append(cycles)
            current_slot[tile] = slot
        elif id == 0x401:
            slot = current_slot[tile]
            if tile not in end_wait:
                end_wait[tile] = {}
            if slot not in end_wait[tile]:
                end_wait[tile][slot] = []
            end_wait[tile][slot].append(cycles)
        elif id == 0x402:
            if tile not in end:
                end[tile] = {}
            if slot not in end[tile]:
                end[tile][slot] = []
            end[tile][slot].append(cycles)

waits = []
os = []
totals = []

starts = []
ends = []
tp = []
for t in end:
    transfers = []
    for s in end[t]:
        waits += list(numpy.subtract(end_wait[t][s][5:], start[t][s][5:]))
        os += list(numpy.subtract(end[t][s][5:], end_wait[t][s][5:]))
        totals += list(numpy.subtract(end[t][s][4:-5], start[t][s][4:-5]))
        starts.append(start[t][s][4])
        ends.append(end[t][s][-5])
        transfers.append(len(start[t][s][4:-5]))
    duration = max(ends) - min(starts)
    tp.append(bytes*sum(transfers)/duration)

tp_avg = sum(tp)/len(tp)

waits_avg = sum(waits)/len(waits)
waits_min = min(waits)
waits_max = max(waits)

os_avg = sum(os)/len(os)
os_min = min(os)
os_max = max(os)

totals_avg = sum(totals)/len(totals)
totals_min = min(totals)
totals_max = max(totals)

data = {
    'bytes': bytes,
    'gap_min': gap_min,
    'gap_avg': gap_avg,
    'gap_max': gap_max,
    'egops_min': egops_min,
    'egops_avg': egops_avg,
    'egops_max': egops_max,
    'egutil_avg': egutil_avg,
    'os_min': os_min,
    'os_avg': os_avg,
    'os_max': os_max,
    'waits_min': waits_min,
    'waits_avg': waits_avg,
    'waits_max': waits_max,
    'or_min': 0,
    'or_avg': 0,
    'or_max': 0,
    'waitr_min': 0,
    'waitr_avg': 0,
    'waitr_max': 0,
    'lat_min': lat_min,
    'lat_avg': lat_avg,
    'lat_max': lat_max,
    'tp_avg': tp_avg,
    'retry_avg': retry_avg,
    'act_avg': act_avg,
    }

print("{bytes} "
      "{gap_min} {gap_avg} {gap_max} "
      "{egops_min} {egops_avg} {egops_max} "
      "{egutil_avg:.4f} "
      "{os_min} {os_avg} {os_max} "
      "{waits_min:.4f} {waits_avg:.4f} {waits_max:.4f} "
      "{tp_avg:.4f} "
      "{or_min} {or_avg} {or_max} "
      "{waitr_min} {waitr_avg} {waitr_max} "
      "{lat_min} {lat_avg} {lat_max} "
      "{retry_avg:.4f} {act_avg:.4f}".format(**data))
