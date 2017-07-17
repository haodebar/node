// Flags: --expose-http2
'use strict';

// Verifies that uploading data from a client works

const common = require('../common');
const assert = require('assert');
const http2 = require('http2');
const fs = require('fs');
const path = require('path');

const loc = path.join(common.fixturesDir, 'person.jpg');
let fileData;

assert(fs.existsSync(loc));

fs.readFile(loc, common.mustCall((err, data) => {
  assert.ifError(err);
  fileData = data;

  const server = http2.createServer();

  server.on('stream', common.mustCall((stream) => {
    let data = Buffer.alloc(0);
    stream.on('data', (chunk) => data = Buffer.concat([data, chunk]));
    stream.on('end', common.mustCall(() => {
      assert.deepStrictEqual(data, fileData);
    }));
    stream.respond();
    stream.end();
  }));

  server.listen(0, common.mustCall(() => {
    const client = http2.connect(`http://localhost:${server.address().port}`);
    const req = client.request({ ':method': 'POST' });
    req.on('response', common.mustCall());
    req.resume();
    req.on('end', common.mustCall(() => {
      server.close();
      client.destroy();
    }));
    fs.createReadStream(loc).pipe(req);
  }));
}));
