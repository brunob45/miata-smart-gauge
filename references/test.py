#!/usr/env/bin python3

bins = [(i * 500 + 500) for i in range (16)]
print(bins)

RATIO_BIN = 0.3
rpm = 1200
x = 1
while x < 15:
    if (rpm <= bins[x]):
        break
    x += 1

rpm1 = bins[x-1]
rpm2 = bins[x]
ax = (rpm - rpm1) / (rpm2 - rpm1)
if (ax <= RATIO_BIN):
    x2 = x = (x - 1)
elif (ax >= (1 - RATIO_BIN)):
    x2 = x
else:
    x2 = x
    x = x - 1

print(rpm, bins[x], bins[x2], ax)