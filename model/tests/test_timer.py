"""Replay JavaScript prototype traces against production C, including mutation checks."""
import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'model/kitchen_timer.c'

class Timer(C.Structure):
    _fields_ = [('id', C.c_uint32), ('total', C.c_int64), ('left', C.c_int64),
                ('deadline', C.c_int64), ('status', C.c_int), ('ack', C.c_int)]
class Service(C.Structure):
    _fields_ = [('items', Timer * 3), ('count', C.c_uint32), ('next_id', C.c_uint32)]

def compile_lib(directory, source):
    path = Path(directory) / 'candidate.c'
    path.write_text(source)
    lib = Path(directory) / 'timer.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                    '-I', str(ROOT / 'model'), str(path), '-o', str(lib)], check=True)
    dll = C.CDLL(str(lib))
    p = C.POINTER(Service)
    dll.kt_init.argtypes = [p]
    dll.kt_add.argtypes = [p, C.c_uint32, C.c_int64, C.POINTER(C.c_uint32)]
    dll.kt_remaining.argtypes = [C.POINTER(Timer), C.c_int64]
    dll.kt_remaining.restype = C.c_int64
    for name in ('kt_toggle',): getattr(dll, name).argtypes = [p, C.c_uint32, C.c_int64]
    for name in ('kt_cancel', 'kt_acknowledge'): getattr(dll, name).argtypes = [p, C.c_uint32]
    for name in ('kt_tick', 'kt_alarm'): getattr(dll, name).argtypes = [p, C.c_int64]
    dll.kt_alarm.restype = C.c_uint32
    return dll

def replay(dll, trace):
    s = Service(); dll.kt_init(C.byref(s))
    for index, event in enumerate(trace):
        op, arg, now = event['op'], event['arg'], event['now']
        result = None
        if op == 'add': result = bool(dll.kt_add(C.byref(s), arg, now, None))
        elif op == 'toggle': dll.kt_toggle(C.byref(s), arg, now)
        elif op == 'cancel': dll.kt_cancel(C.byref(s), arg)
        elif op == 'tick': result = bool(dll.kt_tick(C.byref(s), now))
        elif op == 'alarm': result = dll.kt_alarm(C.byref(s), now)
        elif op == 'ack': dll.kt_acknowledge(C.byref(s), arg)
        elif op == 'restore':
            # Model the caller restoring persisted fields, as in Timers(saved).
            s = Service.from_buffer_copy(bytes(s))
            s.next_id = max([0] + [s.items[i].id for i in range(s.count)]) + 1
        items = []
        for i in range(s.count):
            t = s.items[i]
            items.append(dict(id=t.id, totalMs=t.total, leftMs=t.left, deadline=t.deadline,
                              status=['running','paused','done'][t.status], acknowledged=bool(t.ack),
                              remainingMs=dll.kt_remaining(C.byref(t), now)))
        actual = (result, s.next_id, items)
        expected = (event['result'], event['nextId'], event['items'])
        if actual != expected:
            raise AssertionError(f'trace step {index}: {event["op"]}({arg})\n{actual!r}\n!= {expected!r}')

class TimerParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace = json.loads(subprocess.check_output(['node', str(ROOT / 'model/tests/timer_trace.mjs')]))
        cls.source = SOURCE.read_text()

    def test_prototype_trace(self):
        with tempfile.TemporaryDirectory() as tmp: replay(compile_lib(tmp, self.source), self.trace)

    def test_detects_expiry_and_capacity_mutations(self):
        mutations = [
            ('kt_remaining(t, now) == 0', 'kt_remaining(t, now) < 0'),
            ('s->count >= KITCHEN_TIMER_CAPACITY', 's->count >= KITCHEN_TIMER_CAPACITY - 1'),
        ]
        for original, replacement in mutations:
            with self.subTest(mutation=original), tempfile.TemporaryDirectory() as tmp:
                self.assertEqual(self.source.count(original), 1)
                with self.assertRaises(AssertionError):
                    replay(compile_lib(tmp, self.source.replace(original, replacement)), self.trace)

if __name__ == '__main__': unittest.main()
