#!/usr/bin/env python3

import math

resolution = 12
res_max = pow(2, resolution) - 1
sine_points = 128

points = [
    round((res_max / 2) + (res_max / 2) * math.sin(2 * math.pi * i / sine_points))
    for i in range(sine_points)
]

lines = []
line = "{"
for i, value in enumerate(points):
    token = f" {value}" + ("," if i < len(points) - 1 else " ")
    if len(line) + len(token) > 100:
        lines.append(line)
        line = ""
    line += token
lines.append(line + "}")
print("\n".join(lines))
