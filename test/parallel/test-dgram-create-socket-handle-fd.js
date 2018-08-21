'use strict';
const common = require('../common');
if (process.platform === 'win32')
  common.skip('Does not support binding fd on Windows');

const assert = require('assert');
const dgram = require('dgram');
const { UDP } = process.binding('udp_wrap');
const { TCP, constants } = process.binding('tcp_wrap');
const _createSocketHandle = dgram._createSocketHandle;

// Return a negative number if the "existing fd" is invalid.
{
  const err = _createSocketHandle(common.localhostIPv4, 0, 'udp4', 42);
  assert(err < 0);
}

// Return a negative number if the type of fd is not "UDP".
{
  // Create a handle with fd.
  const rawHandle = new UDP();
  const err = rawHandle.bind(common.localhostIPv4, 0, 0);
  assert(err >= 0, String(err));
  assert.notStrictEqual(rawHandle.fd, -1);

  const handle = _createSocketHandle(null, 0, 'udp4', rawHandle.fd);
  assert(handle instanceof UDP);
  assert.strictEqual(typeof handle.fd, 'number');
  assert(handle.fd > 0);
}

// Create a bound handle.
{
  const rawHandle = new TCP(constants.SOCKET);
  const err = rawHandle.listen();
  assert(err >= 0, String(err));
  assert.notStrictEqual(rawHandle.fd, -1);

  const handle = _createSocketHandle(null, 0, 'udp4', rawHandle.fd);
  assert(handle < 0);
  rawHandle.close();
}
