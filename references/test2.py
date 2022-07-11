#!/usr/env/bin python3

from random import randrange


bins = [30 for i in range (16)]
bins[15] = 200
value = 30
print(bins)

trigger = False
for i in range(16):
    print(abs(bins[i] - value), 400-i*15)
    if abs(bins[i] - value) > (400-i*15):
        trigger = True
    if i < 15:
        bins[i] = bins[i+1]
    else:
        bins[i] = value

print(trigger, bins)