#!/usr/bin/env python3
# generate lookup table for MIDI note frequencies. 
#
#
# @author Jan-Willem Smaal <usenet@gispen.rog
# @date 20260102
# license SPDX-License-Identifier: Apache-2.0
"""
Generate a C array of precomputed frequencies for MIDI notes 0..127 (C-1 .. G9).
Usage:
    python3 gen_midi_table.py --a4 440.0 > midi_freq_table.c
Options:
    --a4 FLOAT    Set concert pitch A4 in Hz (default 440.0)
    --out FILE    Write output to FILE instead of stdout
"""
import argparse
from math import pow

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
FLAT_EQUIV = {"C#": "Db", "D#": "Eb", "F#": "Gb", "G#": "Ab", "A#": "Bb"}
A4_MIDI = 69

def midi_note_name(n):
    name = NOTE_NAMES[n % 12]
    octave = (n // 12) - 1
    if name in FLAT_EQUIV:
        return f"{name}{octave}/{FLAT_EQUIV[name]}{octave}"
    return f"{name}{octave}"

def midi_freq(n, a4_freq):
    # f = A4 * 2^((n - 69) / 12)
    return a4_freq * pow(2.0, (n - A4_MIDI) / 12.0)

def generate_c_table(a4_freq):
    lines = []
    lines.append("/* Precomputed frequencies for MIDI notes 0–127 (C-1 to G9) */")
    #lines.append("static const float midi_freq_table[128] = {")
    for i in range(128):
        f = midi_freq(i, a4_freq)
        name = midi_note_name(i)
        # Format with 10 decimal places, 'f' suffix, and aligned comment
        lines.append(f"    {f:0.10f}f,   /* {i}: {name} */")
    #lines.append("};")
    return "\n".join(lines)

def main():
    p = argparse.ArgumentParser(description="Generate C table of MIDI frequencies")
    p.add_argument("--a4", type=float, default=440.0,
                   help="Concert pitch frequency for A4 in Hz (default 440.0)")
    p.add_argument("--out", type=str, default=None,
                   help="Write output to file instead of stdout")
    args = p.parse_args()

    c_text = generate_c_table(args.a4)
    if args.out:
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(c_text + "\n")
    else:
        print(c_text)

if __name__ == "__main__":
    main()
