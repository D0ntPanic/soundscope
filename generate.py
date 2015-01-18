#!/usr/bin/env python
import struct
import math
import random

allsigs = open("all.wav", "wb")

def write(name, samples, count = 200):
	f = open(name, "wb")

	# fuzyll's added .wav stuff
	f.write("RIFF")
	f.write(struct.pack("<i", (len(samples)*8)+36)) # chunk size
	f.write("WAVE")
	f.write("fmt ")
	f.write(struct.pack("<i", 16))         # PCM data chunk is 16 bytes in size
	f.write(struct.pack("<h", 1))          # audio format for PCM data is 1 (linear quantization)
	f.write(struct.pack("<h", 2))          # num channels is 2
	f.write(struct.pack("<i", 44100))      # sample rate is 44100hz
	f.write(struct.pack("<i", 44100*2*2))  # byte rate is (sample rate * num channels * bytes per sample)
	f.write(struct.pack("<h", 2*2))        # block align is (num channels * bytes per sample)
	f.write(struct.pack("<h", 16))         # bits per sample is (bytes per sample * 8)
	f.write("data")
	f.write(struct.pack("<i", len(samples)*8)) # data size

	for i in xrange(0, count):
		for sample in samples:
			f.write(struct.pack("<hh", int(sample[0] * 32767), int(sample[1] * 32767)))
			allsigs.write(struct.pack("<hh", int(sample[0] * 32767), int(sample[1] * 32767)))
	f.close()

def gen(pts, width):
	perpt = 196.0 / len(pts)
	pts.append(pts[0])
	result = []
	for i in xrange(0, 196):
		a = int(i / perpt)
		b = a + 1
		frac = (i / perpt) - a
		x = (pts[a][0] * (1.0 - frac)) + (pts[b][0] * frac)
		y = (pts[a][1] * (1.0 - frac)) + (pts[b][1] * frac)
		result.append((x * -0.75 * width, y * 0.75))
	return result

def gen3d(pts, frames = 1):
	perpt = (196.0 * frames) / len(pts)
	pts.append(pts[0])
	result = []
	for i in xrange(0, 196 * frames):
		a = int(i / perpt)
		b = a + 1
		frac = (i / perpt) - a
		x = (pts[a][0] * (1.0 - frac)) + (pts[b][0] * frac)
		y = (pts[a][1] * (1.0 - frac)) + (pts[b][1] * frac)
		z = (pts[a][2] * (1.0 - frac)) + (pts[b][2] * frac) + 5
		result.append(((x / z) * -1.5, (y / z) * 1.5))
	return result

def rotatept(vec, axis, angle):
	dist = math.sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2])
	axis = (axis[0] / dist, axis[1] / dist, axis[2] / dist)
	x = math.sin(angle * 0.5) * axis[0]
	y = math.sin(angle * 0.5) * axis[1]
	z = math.sin(angle * 0.5) * axis[2]
	w = math.cos(angle * 0.5)
	matrix = ((1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y + w * z), 2.0 * (x * z - w * y)),
			(2.0 * (x * y - w * z), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z + w * x)),
			(2.0 * (x * z + w * y), 2.0 * (y * z - w * x), 1.0 - 2.0 * (x * x + y * y)))
	return (matrix[0][0] * vec[0] + matrix[1][0] * vec[1] + matrix[2][0] * vec[2],
		matrix[0][1] * vec[0] + matrix[1][1] * vec[1] + matrix[2][1] * vec[2],
		matrix[0][2] * vec[0] + matrix[1][2] * vec[1] + matrix[2][2] * vec[2])

def rotate(pts, axis, angle):
	result = []
	for pt in pts:
		result.append(rotatept(pt, axis, angle))
	return result

def spline(val, frac):
	blendmatrix = [[0, 1, 0, 0], [-0.5, 0, 0.5, 0], [1, -2.5, 2.0, -0.5], [-0.5, 1.5, -1.5, 0.5]]
	result = 0
	poly = 1
	for i in xrange(0, 4):
		factor = 0
		for j in xrange(0, 4):
			factor += val[j] * blendmatrix[i][j]
		result += factor * poly
		poly *= frac
	return result

write("square.wav", gen([(-1, -1), (1, -1), (1, 1), (-1, 1)], 1))

p = []
for i in xrange(0, 1024):
	p += gen3d(rotate([(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, -1),
		(-1, -1, 1), (-1, 1, 1), (-1, 1, -1),
		(1, 1, -1), (1, 1, 1), (-1, 1, 1),
		(-1, -1, 1), (1, -1, 1), (1, 1, 1),
		(1, -1, 1), (1, -1, -1), (1, 1, -1),
		(1, 1, 1), (-1, 1, 1), (-1, 1, -1)], (0.5, 1, 0.25), 2 * math.pi * (i / 1024.0)), 1)
write("cube.wav", p, 4)

p = []
for i in xrange(0, 256):
	f = []
	for j in xrange(0, 8):
		for k in xrange(0, 37):
			a = 2.0 * math.pi * ((k + (j * 4)) / 32.0)
			if (j & 1) == 0:
				z = 0.25
			else:
				z = -0.25
			f.append((math.sin(a), math.cos(a), z))
	p += gen3d(rotate(f, (0.5, 1, 0.25), 2 * math.pi * (i / 256.0)), 6)
write("ring.wav", p, 4)

factors = []
for i in xrange(0, 16):
	cur = []
	for j in xrange(0, 8):
		cur.append(random.random() * 0.5)
	cur.append(cur[0])
	factors.append(cur)

p = []
for i in xrange(0, 2048):
	f = []
	b = int(i / 128)
	a = (b - 1) & 15
	c = (b + 1) & 15
	d = (c + 1) & 15
	curfactors = []
	frac = (i / 128.0) - b
	for j in xrange(0, 9):
		curfactors.append(spline((factors[a][j], factors[b][j], factors[c][j], factors[d][j]), frac))
	for j in xrange(0, 64):
		angle = 2.0 * math.pi * (j / 64.0)
		b = int(j / 8)
		a = (b - 1) & 7
		c = (b + 1) & 7
		d = (c + 1) & 7
		frac = (j / 8.0) - b
		dist = 0.5 + spline((curfactors[a], curfactors[b], curfactors[c], curfactors[d]), frac)
		f.append((math.sin(angle) * dist, math.cos(angle) * dist))
	p += gen(f, 1)
write("distort.wav", p, 4)

write("a.wav", gen([(-1, 1), (0, -1), (1, 1), (0.5, 0), (-0.5, 0)], 0.6))
write("b.wav", gen([(-1, 1), (-1, 0), (0.7, 0), (1, 0.3), (1, 0.7), (0.7, 1), (-1, 1), (-1, -1), (0.7, -1), (1, -0.7), (1, -0.3), (0.7, 0), (-1, 0)], 0.6))
write("c.wav", gen([(-1, 1), (1, 1), (-1, 1), (-1, -1), (1, -1), (-1, -1)], 0.6))
write("d.wav", gen([(-1, 1), (-1, -1), (0.7, -1), (1, -0.7), (1, 0.7), (0.7, 1)], 0.6))
write("e.wav", gen([(-1, 1), (-1, -1), (1, -1), (-1, -1), (-1, 0), (0.7, 0), (-1, 0), (-1, 1), (1, 1)], 0.6))
write("f.wav", gen([(-1, 1), (-1, -1), (1, -1), (-1, -1), (-1, 0), (0.7, 0), (-1, 0)], 0.6))
write("g.wav", gen([(1, -1), (-1, -1), (-1, 1), (1, 1), (1, 0), (0.5, 0), (1, 0), (1, 1), (-1, 1), (-1, -1)], 0.6))
write("h.wav", gen([(-1, 1), (-1, -1), (-1, 0), (1, 0), (1, -1), (1, 1), (1, 0), (-1, 0)], 0.6))
write("i.wav", gen([(-0.25, -1), (0.25, -1), (0, -1), (0, 1), (-0.25, 1), (0.25, 1), (0, 1), (0, -1)], 0.6))
write("j.wav", gen([(1, -1), (1, 1), (-1, 1), (1, 1)], 0.6))
write("k.wav", gen([(-1, -1), (-1, 1), (-1, 0), (1, -1), (-1, 0), (1, 1), (-1, 0)], 0.6))
write("l.wav", gen([(-1, -1), (-1, 1), (1, 1), (-1, 1)], 0.6))
write("m.wav", gen([(-1, 1), (-1, -1), (0, 0), (1, -1), (1, 1), (1, -1), (0, 0), (-1, -1)], 0.6))
write("n.wav", gen([(-1, 1), (-1, -1), (1, 1), (1, -1), (1, 1), (-1, -1)], 0.6))
write("o.wav", gen([(-1, -0.7), (-0.7, -1), (0.7, -1), (1, -0.7), (1, 0.7), (0.7, 1), (-0.7, 1), (-1, 0.7)], 0.6))
write("p.wav", gen([(-1, 1), (-1, -1), (1, -1), (1, 0), (-1, 0)], 0.6))
write("q.wav", gen([(-1, 1), (-1, -1), (1, -1), (1, 1), (0.7, 0.7), (1.1, 1.1), (1, 1)], 0.6))
write("r.wav", gen([(-1, 1), (-1, -1), (1, -1), (1, 0), (0, 0), (1, 1), (0, 0), (-1, 0)], 0.6))
write("s.wav", gen([(1, -1), (-1, -1), (-1, 0), (1, 0), (1, 1), (-1, 1), (1, 1), (1, 0), (-1, 0), (-1, -1)], 0.6))
write("t.wav", gen([(-1, -1), (1, -1), (0, -1), (0, 1), (0, -1)], 0.6))
write("u.wav", gen([(-1, -1), (-1, 1), (1, 1), (1, -1), (1, 1), (-1, 1)], 0.6))
write("v.wav", gen([(-1, -1), (0, 1), (1, -1), (0, 1)], 0.6))
write("w.wav", gen([(-1, -1), (-1, 1), (0, 0), (1, 1), (1, -1), (1, 1), (0, 0), (-1, 1)], 0.6))
write("x.wav", gen([(-1, -1), (1, 1), (0, 0), (1, -1), (-1, 1), (0, 0)], 0.6))
write("y.wav", gen([(-1, -1), (0, 0), (0, 1), (0, 0), (1, -1), (0, 0)], 0.6))
write("z.wav", gen([(-1, -1), (1, -1), (-1, 1), (1, 1), (-1, 1), (1, -1)], 0.6))

