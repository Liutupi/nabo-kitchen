// Generate expected states from the unchanged production prototype, with a test clock.
import {Timers} from './fixtures/prototype-v0.1/timers.mjs';
let now = 1000, service = new Timers(() => now);
const trace = [];
function step(op, arg = 0) {
  let result = null;
  if (op === 'add') result = !!service.add(arg, 'label', 'source').timer;
  if (op === 'advance') now += arg;
  if (op === 'toggle') service.toggle(arg);
  if (op === 'cancel') service.cancel(arg);
  if (op === 'tick') result = service.tick();
  if (op === 'alarm') result = service.alarm()?.id ?? 0;
  if (op === 'ack') service.acknowledge(arg);
  if (op === 'restore') service = new Timers(() => now, service.serialize());
  trace.push({op, arg, now, result, nextId: service.nextId, items: service.snapshot().map(t => ({
    id:t.id,totalMs:t.totalMs,leftMs:t.leftMs,deadline:t.deadline,status:t.status,
    acknowledged:t.acknowledged,remainingMs:t.remainingMs
  }))});
}
for (const [op,arg] of [['add',0],['add',5],['add',6],['add',7],['add',1],
  ['advance',2000],['toggle',1],['advance',10000],['tick'],['alarm'],['ack',2],
  ['alarm'],['restore'],['toggle',1],['advance',3000],['tick'],['alarm'],
  ['cancel',2],['add',10],['toggle',4],['advance',20000],['toggle',4],
  ['advance',10000],['alarm'],['ack',1],['cancel',1],['cancel',3],['cancel',4]]) step(op,arg);
// Deterministic mixed operations exercise many interleavings and absent IDs.
let seed = 0x12345678;
for (let i = 0; i < 2000; ++i) {
  seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
  const op = ['add','advance','toggle','tick','alarm','ack','cancel','restore'][seed % 8];
  seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
  step(op, op === 'add' ? seed % 10 : op === 'advance' ? seed % 5000 : seed % (service.nextId + 2));
}
process.stdout.write(JSON.stringify(trace));
