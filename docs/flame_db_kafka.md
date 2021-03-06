### `namespace flame\db\kafka`
提供基本的异步 Kafka 协程式客户端封装；基于 [librdkafka]((https://github.com/edenhill/librdkafka/) 封装。

**示例**：
``` PHP
<?php
// 生产
flame\go(function() {
	$producer = new flame\db\kafka\producer([
		"bootstrap.servers"  => "127.0.0.1:19092, 127.0.0.1:29092",
		"compression.codec"  => "snappy",
		"batch.num.messages" => "1000",
	], [
		"partitioner_cb" => function($key, $count) {
			return crc32($key) % $count;
		}
	], "test_topic"); // 生产者指定单个 topic 进行生产
	yield $producer->produce("this is a message", "this is the key");
	yield $producer->produce("this is a message", "this is the key");
});
// 消费
flame\go(function() {
	$consumer = new flame\db\kafka\consumer([
		"bootstrap.servers"  => "127.0.0.1:19092, 127.0.0.1:29092",
	], [
		"partitioner_cb" => function($key, $count) {
			return crc32($key) % $count;
		}
	], ["test_topic"]); // 消费者可指定多个 topic 进行消费
	// 以下两种消费方式不可混合使用，这里仅作示范代码
	// 方式 1.
	$msg = yield $producer->consume();
	// 方式 2.
	yield $consumer->consume(function($msg) {
		// $msg
	});
});
```

#### `class flame\db\kafka\producer`
生产者

##### `producer::__construct(array $global_opt, array $topic_opt,string $topic)`
使用 Kafka 配置对象构建生产者；选项 `$global_opt` / `$topic_opt` 请参考 [CONFIGURATION.md](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md) 进行配置；目标话题由 `$topic` 指定；

**注意**：
* 所有选项中所有 KEY / VAL 均会被转换为字符串进行设置；
* 全局选项 `$global_opt` 中 `bootstrap.servers` 或 `metadata.broker.list` 必须存在；
* 全局选项 `$global_opt` 中特殊 `set with .....` 选项均不支持；
* 话题选项 `$topic_opt` 中特殊 `set with .....` 仅支持如下选项：
	* partitioner_cb - 可以设置 `"consistent"` - 一致性哈希；`"random"` - 随机；`"consistent_random"` - 默认，设定了消息 KEY 则使用一致哈希，否则随机；`callable` - 自定义回调函数（参见顶部示例）；

##### `yield producer::produce(string $payload[, string $key])`
生产一条消息，可选的指定消息 KEY（分区使用）；

#### `class flame\db\kafka\consumer`
消费者

**注意**：
* 不能将同一个 consumer 用于多个协程；

##### `consumer::__construct(array $global_opt, array $topic_opt, array $topics)`
创建消费者，准备消费指定的所有话题消息；选项 `$global_opt` | `$topic_opt` 请参考 [CONFIGURATION.md](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md) 进行配置；被消费话题由 `$topics` 数组指定；

**注意**：
* 所有选项中所有 KEY / VAL 均会被转换为字符串进行设置；
* 全局选项 `$global_opt` 中 `bootstrap.servers` | `metadata.broker.list` 必须存在；
* 全局选项 `$global_opt` 及 话题选项 `$topic_opt` 中所有标注 `set with .....` 的特殊选项**均不支持**；

##### `yield consumer::consume()`
消费形式 1：消费一条消息；

**注意**：
* **不能**将同一个 consumer 用于多个协程进行消费；
* 消费的两种形式，不能对同一消费者同时使用；


**示例**：
``` PHP
<?php
// $consumer = ...
$msg = yield $consumer->consume();
```

##### `yield consumer::consume(callable $cb)`
消费形式 2：消息消费回调

**注意**：
* **不能**将同一个 consumer 用于多个协程进行消费；
* 消费形式 2，每个回调都在单独的“协程”中运行；
* 消费的两种形式，不能对同一消费者同时使用；


**示例**：
``` PHP
<?php
// $consumer = ...
yield $consumer->consume(function($msg) {
	// $msg ...
});
```

##### `yield consumer::commit(object $msg)`
手动提交消息偏移；

**注意**：
* 需要手动提交，请设置 `enable.auto.commit` 为 `"false"`
* 相较之下，手动提交效率较低，请斟酌使用；

#### `class flame\db\kafka\message`
消费者消费过程返回的消息对象，用于包裹实际的消息内容和其相关数据

##### `string  message::$key`
消息生产时指定的 KEY；

##### `integer message::$time`
消息时间戳（毫秒级）；

##### `string message::$val`
消息体，与 `__toString()` 一致；

##### `message::__toString()`
消息体，与 `$val` 一致；
