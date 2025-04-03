package common

import "base:intrinsics"
import "core:math"

Damped :: struct($T: typeid) where intrinsics.type_is_numeric(T) {
	started, enabled: bool,
	_xp, _yd, _y:     T,
	_k1, _k2, _k3:    f32,
	freq, damp, resp: f32,
}

NewDamped :: proc(x0: $T) -> (new: Damped(T)) {
	new.enabled = true
	new.started = true

	new.freq = 1
	new.damp = 1
	new.resp = 1

	new._xp = x0
	new._y = x0

	DampedSetCurrent(&new)
	return
}

DampedSet :: proc(d: ^Damped($T), f: f32 = 1, z: f32 = 1, r: f32 = 0) {
	d.freq = f
	d.damp = z
	d.resp = r

	DampedSetCurrent(d)
}

@(private = "file")
DampedSetCurrent :: proc(d: ^Damped($T)) {
	d._k1 = d.damp / (math.PI * d.freq)
	d._k2 = 1 / ((math.TAU * d.freq) * (math.TAU * d.freq))
	d._k3 = d.resp * d.damp / (math.TAU * d.freq)
}

DampedUpdate :: proc(d: ^Damped($T), delta: f32, x: T) {
	if !d.started {
		DampedStart(d)
		return
	}

	if !d.enabled {
		d._y = x
		return
	}

	xd := (x - d._xp) / delta
	d._xp = x
	d._y = d._y + delta * d._yd

	k2_stable := max(d._k2, 1.1 * (delta * delta / 4 + delta * d._k1 / 2))
	d._yd = d._yd + delta * (x + d._k3 * xd - d._y - d._k1 * d._yd) / k2_stable
}

@(private = "file")
Damped_Test :: proc() {
	a := NewDamped(2)
	b := NewDamped([4]f32{0.2, 0.7, 23.0, 9})
	c := NewDamped([2]i32{400, 650})
}
