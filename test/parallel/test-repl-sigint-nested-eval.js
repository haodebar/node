'use strict';
const common = require('../common');
if (process.platform === 'win32') {
  // No way to send CTRL_C_EVENT to processes from JS right now.
  common.skip('platform not supported');
}
if (!common.isMainThread)
  common.skip('No signal handling available in Workers');

const assert = require('assert');
const spawn = require('child_process').spawn;

process.env.REPL_TEST_PPID = process.pid;
const child = spawn(process.execPath, [ '-i' ], {
  stdio: [null, null, 2]
});

let stdout = '';
child.stdout.setEncoding('utf8');
child.stdout.on('data', function(c) {
  stdout += c;
});

child.stdout.once('data', common.mustCall(() => {
  process.on('SIGUSR2', common.mustCall(() => {
    process.kill(child.pid, 'SIGINT');
    child.stdout.once('data', common.mustCall(() => {
      // Make sure REPL still works.
      child.stdin.end('"foobar"\n');
    }));
  }));

  child.stdin.write('process.kill(+process.env.REPL_TEST_PPID, "SIGUSR2");' +
                    'vm.runInThisContext("while(true){}", ' +
                    '{ breakOnSigint: true });\n');
}));

child.on('close', function(code) {
  const expected = 'Script execution was interrupted by `SIGINT`';
  assert.ok(
    stdout.includes(expected),
    `Expected stdout to contain "${expected}", got ${stdout}`
  );
  assert.ok(
    stdout.includes('foobar'),
    `Expected stdout to contain "foobar", got ${stdout}`
  );
});
