### `namespace flame\log`
提供简单 log 日志 API，并提供 “ROTATE” 和输出文件指定。

#### `flame\log\rotate()`
重新打开输出文件；

#### `flame\log\set_output(string $output)`
设置输出目标：
1. "stdout" - 将日志输出到“标准输出”；
2. "stdlog"/"stderr" - 将日志输出到“标准错误”；
3. 文件路径 - 将日志输出到指定文件；

#### `yield flame\log\fail(mixed $message, ...)`
#### `yield flame\log\warn(mixed $message, ...)`
#### `yield flame\log\info(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳和日志等级；数组将会被自动转换为 JSON 串，其他均使用 PHP 内置 toString() 转换为字符串输出。

#### `yield flame\log\write(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳（不含日志等级外与上述 `fail`/`warn`/`info` 等函数一致）

**日志**：
* 每项函数参数输出前时均会前置一个空格；

### `class flame\log\logger`
日志类封装，外层 `flame\log\*` 提供的 API 即对全局的默认 logger 实例进行操作。

#### `logger::rotate()`
#### `logger::set_output(string $output)`
#### `yield logger::fail(mixed $message, ...)`
#### `yield logger::warn(mixed $message, ...)`
#### `yield logger::info(mixed $message, ...)`
与上述全局 API 功能一致，但仅对当前 logger 实例生效。
