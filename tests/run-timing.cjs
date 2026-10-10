const fs = require('fs');
let avr8js;
try {
  avr8js = require('avr8js');
} catch (error) {
  if (error.code !== 'MODULE_NOT_FOUND') throw error;
  avr8js = require('../../AY-PLAYER/.tools/avr8js/package');
}
const { CPU, avrInstruction } = avr8js;
const suite = process.argv[2] || 'timing';
const bytes = fs.readFileSync('tests/' + suite + '.bin');
const program = new Uint16Array(16384);
for (let i = 0; i < bytes.length; i += 2) program[i >> 1] = bytes[i] | ((bytes[i + 1] || 0) << 8);
const symbols = fs.readFileSync('tests/' + suite + '.symbols', 'utf8');
const address = name => {
  const match = symbols.match(new RegExp('^([0-9a-f]+) \\w ' + name + '$', 'm'));
  if (!match) throw new Error('Missing symbol ' + name);
  return parseInt(match[1], 16) - 0x800000;
};
const cpu = new CPU(program);
for (let i = 0; i < 5000000 && cpu.data[address('test_done')] !== 0xA5; ++i) avrInstruction(cpu);
if (cpu.data[address('test_done')] !== 0xA5) throw new Error('Timed out');
if (cpu.data[address('test_failure')]) throw new Error('Failed assertion ' + cpu.data[address('test_failure')]);
console.log('PASS: ' + cpu.data[address('test_count')] + ' ' + suite + ' assertions on compiled AVR code');
