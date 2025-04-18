#!/usr/bin/env python3

import numpy as np
import quaternion


def from_euler(x, y, z):
    c1 = np.cos(y / 2.0)
    c2 = np.cos(z / 2.0)
    c3 = np.cos(x / 2.0)

    s1 = np.sin(y / 2.0)
    s2 = np.sin(z / 2.0)
    s3 = np.sin(x / 2.0)

    return np.quaternion(
        c1 * c2 * c3 + s1 * s2 * s3,
        c1 * c2 * s3 - s1 * s2 * c3,
        s1 * c2 * c3 + c1 * s2 * s3,
        c1 * s2 * c3 - s1 * c2 * s3,
    )


def to_euler(quat):
    w = quat.w
    x = quat.x
    y = quat.y
    z = quat.z

    # Check to circumvent gimbal lock - assume yaw is 0
    test = w * y - z * x
    if test > 0.4999:  # asin(2*0.4999) = 88.85 degrees
        roll = -2 * np.atan2(x, w)
        pitch = 0.5 * np.pi
        yaw = 0
    elif test < -0.4999:
        roll = 2 * np.atan2(x, w)
        pitch = -0.5 * np.pi
        yaw = 0
    else:
        roll = np.atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
        pitch = np.asin(2 * test)
        yaw = np.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))

    return (np.rad2deg(roll), np.rad2deg(pitch), np.rad2deg(yaw))


# roll, pitch, yaw
qf = from_euler(np.deg2rad(-2.64), np.deg2rad(14.18), np.deg2rad(0))
print(qf)
print(to_euler(qf))

qa = np.quaternion(0, -2.54, -0.35, 9.58)
qa = qf * qa * qf.conjugate()  # qf.rotate(qa)

print(qa)
