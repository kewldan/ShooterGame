#!/usr/bin/env python3
"""Generates the placeholder sound effects in data/sounds/ (16-bit mono WAV, 22.05 kHz).

Everything is synthesised from noise bursts, decaying sines and sweeps shaped by envelopes, so
the repository does not depend on any recorded samples. Run from the repository root:

    python tools/gen_sounds.py

Only the standard library is used.
"""
import math
import os
import random
import struct
import wave

RATE = 22050
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "sounds")


def seconds(duration):
    return int(duration * RATE)


def silence(duration):
    return [0.0] * seconds(duration)


def mix(target, source, at=0.0, gain=1.0):
    """Adds `source` into `target` starting at `at` seconds (grows target when needed)."""
    start = seconds(at)
    end = start + len(source)
    if end > len(target):
        target.extend([0.0] * (end - len(target)))
    for i, s in enumerate(source):
        target[start + i] += s * gain
    return target


def noise(duration, rng):
    return [rng.uniform(-1.0, 1.0) for _ in range(seconds(duration))]


def sine(duration, freq_start, freq_end=None):
    """A sine that sweeps linearly from freq_start to freq_end over the duration."""
    n = seconds(duration)
    freq_end = freq_start if freq_end is None else freq_end
    out, phase = [], 0.0
    for i in range(n):
        f = freq_start + (freq_end - freq_start) * i / max(n - 1, 1)
        phase += 2.0 * math.pi * f / RATE
        out.append(math.sin(phase))
    return out


def exp_decay(samples, tau, attack=0.0):
    """Multiplies by an exponential decay (time constant `tau` s) after a linear attack."""
    a = seconds(attack)
    for i in range(len(samples)):
        env = math.exp(-(i / RATE) / tau)
        if a > 0 and i < a:
            env *= i / a
        samples[i] *= env
    return samples


def lowpass(samples, cutoff):
    """One-pole low-pass filter."""
    rc = 1.0 / (2.0 * math.pi * cutoff)
    alpha = (1.0 / RATE) / (rc + 1.0 / RATE)
    out, y = [], 0.0
    for s in samples:
        y += alpha * (s - y)
        out.append(y)
    return out


def highpass(samples, cutoff):
    """One-pole high-pass filter."""
    rc = 1.0 / (2.0 * math.pi * cutoff)
    alpha = rc / (rc + 1.0 / RATE)
    out, y, prev = [], 0.0, 0.0
    for s in samples:
        y = alpha * (y + s - prev)
        prev = s
        out.append(y)
    return out


def normalize(samples, peak=0.9):
    m = max(abs(s) for s in samples) or 1.0
    return [s / m * peak for s in samples]


def soft_clip(samples, drive=1.0):
    return [math.tanh(s * drive) for s in samples]


def write(name, samples):
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))
    print(f"{path}: {len(samples) / RATE:.2f} s, {os.path.getsize(path) // 1024} KB")


def gunshot(rng):
    out = silence(0.35)
    # Crack: a bright noise burst with a very fast decay.
    mix(out, exp_decay(highpass(noise(0.12, rng), 800), 0.02), gain=1.0)
    # Body: a darker burst that rings a little longer.
    mix(out, exp_decay(lowpass(noise(0.3, rng), 1200), 0.06), gain=0.9)
    # Thump: a low sweep for the pressure wave.
    mix(out, exp_decay(sine(0.25, 140, 45), 0.07), gain=0.8)
    return normalize(soft_clip(out, 2.5))


def dryfire(rng):
    out = silence(0.08)
    mix(out, exp_decay(highpass(noise(0.03, rng), 2000), 0.006), gain=0.6)
    mix(out, exp_decay(sine(0.06, 2600), 0.012), gain=0.5)
    return normalize(out, 0.5)


def reload(rng):
    out = silence(1.5)
    # Magazine release click.
    mix(out, exp_decay(highpass(noise(0.03, rng), 1500), 0.008), at=0.08, gain=0.8)
    mix(out, exp_decay(sine(0.05, 1900), 0.015), at=0.08, gain=0.4)
    # Magazine sliding out, then in.
    mix(out, exp_decay(lowpass(noise(0.12, rng), 2500), 0.05, attack=0.03), at=0.25, gain=0.5)
    mix(out, exp_decay(lowpass(noise(0.08, rng), 2000), 0.03, attack=0.01), at=0.75, gain=0.7)
    mix(out, exp_decay(sine(0.08, 900), 0.02), at=0.8, gain=0.4)
    # Slide racked: two metallic clacks.
    for at, gain in ((1.1, 0.9), (1.22, 1.0)):
        mix(out, exp_decay(highpass(noise(0.04, rng), 1200), 0.01), at=at, gain=gain)
        mix(out, exp_decay(sine(0.1, 2400), 0.02), at=at, gain=0.35 * gain)
        mix(out, exp_decay(sine(0.1, 3300), 0.015), at=at, gain=0.2 * gain)
    return normalize(out, 0.85)


def footstep(rng):
    out = silence(0.16)
    mix(out, exp_decay(lowpass(noise(0.12, rng), 900), 0.03, attack=0.004), gain=1.0)
    mix(out, exp_decay(sine(0.1, 110, 70), 0.03), gain=0.5)
    return normalize(out, 0.7)


def jump(rng):
    out = silence(0.22)
    # A cloth-like rustle with a rising cutoff, plus a short push-off thump.
    mix(out, exp_decay(lowpass(noise(0.2, rng), 1800), 0.08, attack=0.02), gain=0.8)
    mix(out, exp_decay(sine(0.12, 90, 60), 0.04), gain=0.5)
    return normalize(out, 0.6)


def land(rng):
    out = silence(0.28)
    mix(out, exp_decay(sine(0.2, 80, 40), 0.06), gain=1.0)
    mix(out, exp_decay(lowpass(noise(0.18, rng), 700), 0.05, attack=0.003), gain=0.9)
    return normalize(out, 0.8)


def hit(rng):
    out = silence(0.22)
    # A wooden knock: two decaying partials and a click.
    mix(out, exp_decay(sine(0.2, 230), 0.05), gain=1.0)
    mix(out, exp_decay(sine(0.15, 410), 0.03), gain=0.5)
    mix(out, exp_decay(lowpass(noise(0.05, rng), 3000), 0.01), gain=0.7)
    return normalize(out, 0.8)


def main():
    rng = random.Random(1337)  # deterministic output
    write("gunshot", gunshot(rng))
    write("dryfire", dryfire(rng))
    write("reload", reload(rng))
    write("footstep", footstep(rng))
    write("jump", jump(rng))
    write("land", land(rng))
    write("hit", hit(rng))


if __name__ == "__main__":
    main()
