#!/usr/env/bin python3

import numpy as np


class Euler:
    def __init__(self):
        self.roll = 0
        self.pitch = 0
        self.yaw = 0

    def from_angles(self, roll=0, pitch=0, yaw=0):
        self.roll = np.radians(roll)
        self.pitch = np.radians(pitch)
        self.yaw = np.radians(yaw)

    def from_quat(self, quat):
        w = quat.a
        x = quat.b
        y = quat.c
        z = quat.d

        test = w * y - z * x
        if test > 0.4999:
            self.roll = -2 * np.arctan2(x, w)
            self.pitch = 0.5 * np.pi
            self.yaw = 0
        elif test < -0.4999:
            self.roll = 2 * np.arctan2(x, w)
            self.pitch = -0.5 * np.pi
            self.yaw = 0
        else:
            self.roll = np.arctan2(2 * (w * x + y * z), 1 - 2 * (x**2 + y**2))
            self.pitch = np.arcsin(2 * test)
            self.yaw = np.arctan2(2 * (w * z + x * y), 1 - 2 * (y**2 + z**2))

    def __str__(self) -> str:
        return f"roll:{np.degrees(self.roll):0.1f}, pitch:{np.degrees(self.pitch):0.1f}, yaw:{np.degrees(self.yaw):0.1f}"


class Quaternion:
    def __init__(self):
        self.a = 1
        self.b = 0
        self.c = 0
        self.d = 0

    def from_euler(self, euler):
        x = euler.roll
        y = euler.pitch
        z = euler.yaw

        # if y > 1.55:
        #     z -= x
        #     x = 0
        # elif y < -1.55:
        #     z += x
        #     x = 0

        c1 = np.cos(x / 2.0)
        c2 = np.cos(y / 2.0)
        c3 = np.cos(z / 2.0)

        s1 = np.sin(x / 2.0)
        s2 = np.sin(y / 2.0)
        s3 = np.sin(z / 2.0)

        self.a = c1 * c2 * c3 + s1 * s2 * s3
        self.b = s1 * c2 * c3 - c1 * s2 * s3
        self.c = c1 * s2 * c3 + s1 * c2 * s3
        self.d = c1 * c2 * s3 - s1 * s2 * c3

    def __str__(self) -> str:
        return f"x:{self.b:0.3f}, y:{self.c:0.3f}, z:{self.d:0.3f}, w:{self.a:0.3f}"

tests = (
    [-40, 90, 0],
    [-20, 90, -20],
    [-20, 90, 0],
    [-20, 90, 20],
    [0, 90, -40],
    [0, 90, -20],
    [0, 90, 0],
    [0, 90, 20],
    [0, 90, 40],
    [20, 90, -20],
    [20, 90, 0],
    [20, 90, 20],
    [40, 90, 0]
)

def do_test(j):
    for test in tests:
        e1 = Euler()
        q1 = Quaternion()

        e1.from_angles(test[0], j, test[2])
        print(e1)

        q1.from_euler(e1)
        e1.from_quat(q1)

        q1.from_euler(e1)
        e1.from_quat(q1)

        q1.from_euler(e1)
        print(e1, q1)

# do_test(1)
do_test(-90)