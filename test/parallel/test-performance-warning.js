// Flags: --no-warnings
'use strict';

const common = require('../common');
const { performance } = require('perf_hooks');
const assert = require('assert');

assert.strictEqual(performance.length, 1);
assert.strictEqual(performance.maxBufferSize, 150);

performance.maxBufferSize = 1;

[-1, 0xffffffff + 1, '', null, undefined, Infinity].forEach((i) => {
  common.expectsError(
    () => performance.maxBufferSize = i,
    {
      code: 'ERR_INVALID_ARG_TYPE',
      type: TypeError
    }
  );
});

common.expectWarning('Warning', [
  'Possible perf_hooks memory leak detected. There are 2 entries in the ' +
  'Performance Timeline. Use the clear methods to remove entries that are no ' +
  'longer needed or set performance.maxBufferSize equal to a higher value ' +
  '(currently the maxBufferSize is 1).']);

performance.mark('test');
