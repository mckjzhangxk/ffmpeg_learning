'use strict'

let https = require('https');
let serveIndex = require('serve-index');
let express = require('express');
let app = express();

let fs = require('fs');
let options = {
  key  : fs.readFileSync('./cert/1557605_www.learningrtc.cn.key'),
  cert : fs.readFileSync('./cert/1557605_www.learningrtc.cn.pem')
}


//顺序不能换,生成索引目录
app.use(serveIndex('./public'));
app.use(express.static('./public'));


var https_server = https.createServer(options, app);
https_server.listen(8443, '0.0.0.0');