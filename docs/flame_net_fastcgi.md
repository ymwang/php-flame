### `namespace flame\net\fastcgi`
提供对 `fastcgi/1.1` 协议的简化封装，支持挂接到 nginx 等 Web 服务器提供应用服务。

### `class flame\net\fastcgi\handler`
监听 `unix socket` 提供后端服务，用于 tcp_server / unix_server 解析 `fastcgi/1.1` 协议，并提供 HTTP 服务；

**示例**：
`test/flame/net/fastcgi/server.php`

#### `server::get/post/put/remove(string $path, callable $cb)`
分别用于设置 GET / POST / PUT / DELETE 请求方法对应路径的处理回调；

#### `server::handle(callable $cb)`
设置默认处理回调（未匹配路径回调），或设置指定路径的请求处理回调（`$path` 参数可选）；回调函数接收两个参数：
* `$request` - 类型 `class flame\net\http\server_request` 的实例，请参考 `flame\net\http` 命名空间中的相关说明；
* `$response` - 类型 `class flame\net\fastcgi\server_response` 的实例，请参考下文；

**示例**：
``` PHP
<?php
$server->get("/hello", function($req, $res) {
	var_dump($req->header["host"]);
	yield $res->write("hello world\n");
	yield $res->end();
})->handle(function($req, $res) {
	yield $res->end('{"error":"router not found"}');
});
```

**注意**：
* `handler` 会为每次回调启用新的协程，故 `$cb` 必须为 `Generator Function`（含有 `yield` 关键字）而非普通回调函数；

### `class flame\net\fastcgi\server_response`
由上述 `handler` 生成，并回调传递给处理函数使用，用于返回响应数据给 Web 服务器；

#### `array server_response::$header`
响应头部 KEY/VAL 数组，默认包含 `Content-Type: text/plain`（注意**区分**大小写）；其他响应头请酌情考虑添加、覆盖；

**示例**：
``` PHP
<?php
$res->header["Content-Type"] = "text/html";
$res->header["X-Server"] = "Flame/0.7.0";
```

#### `yield server_response::write_header(integer $status_code)`
设置并输出响应头部（含上述响应头 KEY/VAL 及响应状态行），请参照标准 HTTP/1.1 STATUS_CODE 设置数值；

#### `yield server_response::write(string $data)`
输出指定响应内容，请参考 `handle()` 的相关示例；

**注意**：
* 不要在多个协程中使用 `write`/`end` 函数；

#### `yield server_response::end([string $data])`
结束请求，可选输出响应内容（输出功能与 `write()` 完全相同）；

**注意**：
* 若服务端设置保留 fastcgi 连接（NGINX 配置 `fastcgi_keep_conn on;`），在结束响应后与 Web 服务器的连接将被保留；否则将会立即关闭；
* 当 server_response 对象被销毁时，会自动结束响应（详单与调用 `end()` 函数）；
