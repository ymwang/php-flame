## `namespace flame\db\mongodb`
提供基本的异步 mongodb 协程式客户端封装；

### `class flame\db\mongodb\client`
mongodb 客户端（连接）；
**示例**：
``` PHP
<?php
$cli = new flame\db\mongo("mongodb://127.0.0.1:27017/database");
// 以下两行代码功能相同
$collection = $cli->col_abc;
$collection = $cli->collection("col_abc");
```

#### `client::collection(string $name) | clieng::$collection_name`
获取指定名称的 collection 集合对象；`client` 类实现了魔术函数 `__get()` 可以直接用集合名称，通过属性形式访问对应的 collection 集合；

#### `client::close()`
立刻关闭当前数据库连接；

**注意**：
* 当连接关闭后，由 `client` 对象生成出的相关对象均不能继续使用，例如由 `collection` 对象；

### `class flame\db\mongodb\object_id`
#### `object_id::__toString()`
#### `object_id::jsonSerialize()`
#### `object_id::timestamp()`
秒级时间戳，表示当前 `object_id` 的创建时间

### `class flame\db\mongodb\date_time`
#### `date_time::__toString()`
#### `date_time::jsonSerialize()`
#### `date_time::timestamp()`
秒级时间戳
#### `date_time::timestamp_ms()`
毫秒级时间戳
#### `date_time::to_datetime()`
转换为 [`DateTime`](http://php.net/manual/en/class.datetime.php) PHP 内置类型对象；

### `class flame\db\mongodb\bulk_result`
#### `boolean bulk_result::success()`
检查批量操作的总体结果，`true` 全部成功，`false` 失败或部分失败；

#### `integer bulk_result::$nInserted`
#### `integer bulk_result::$nMatched`
#### `integer bulk_result::$nModified`
#### `integer bulk_result::$nRemoved`
#### `integer bulk_result::$nUpserted`

### `class flame\db\mongodb\collection`
封装访问 mongodb 数据集合的基础 API；

**示例**：
``` PHP
<?php
// ... 通过 $client 获得 $collection 对象
$result = yield $collection->insert_one(['a'=>'b']);
$result = yield $collection->insert_many([['a'=>'b1'], ['a'=>'b2']]);
$result = yield $collection->delete_one(['a'=>['$ne'=>'b']]);
$result = yield $collection->delete_many(['a'=>'b']);
$result = yield $collection->update_one(['a'=>'b'],['$set'=>['a'=>'c']]);
$result = yield $collection->update_many(['a'=>'b'],['a'=>'c']);
$cursor = yield $collection->find_many(['a'=>'b']);
$doc = yield $collection->find_one(['a'=>['$gt'=>'b']]);
$keys  = yield $collection->distinct('a', ['x'=>'y']);
$count = yield $collection->count(['x'=>'y']);
```

**注意**：
* 对象标识 `ObjectID` 字段需要使用单独的类型 `object_id`；
* 日期时间 `DateTime` 字段需要使用单独的类型 `date_time`；
* 暂不支持 时间戳、正则 等在 Web 开发中不常用的字段类型；
* 其他字段对应相关 PHP 内置类型；
* 请尽量使用 `'` 单引号，防止功能符号被转义，例如 `$gt = "aaa"; $filter = ["a" => ["$gt"=>1234]]`；

#### `yield collection::count([array $query])`
获取当前集合中（符合条件的）文档数量；

#### `yield collection::insert_one(array $doc)`
向当前集合插入一条新的 `$doc` 文档；

**注意**：
* `$doc` 必须是关联数组；
* 可以自行为 `$doc` 创建 `_id` 字段（参考 `object_id` 类型）；

#### `yield collection::insert_many(array $docs)`
向当前集合插入 `$docs` 若干文档；返回批量结果对象 `bulk_result`；

**注意**：
* `$docs` 为二维数组，其中第一维度为下标数组，每个元素标识一个文档（关联数组）；

#### `yield collection::remove_one(array $query)`
从当前集合中删除第一个符合 `$query` 查询的文档；删除成功返回 `true`，否则抛出发生的错误；

#### `yield collection::delete_many(array $query)`
从当前集合中删除所有符合 `$query` 查询的文档；删除成功返回 `true`，否则抛出发生的错误；

#### `yield collection::update_one(array $query, array $data)`
从当前集合中查询第一个符合 `$query` 的文档，并使用 `$data` 更新它；

#### `yield collection::update_many(array $query, array $data)`
从当前集合中查询所有符合 `$query` 的文档，并使用 `$data` 更新它们；

#### `yield collection::find_many(array $query[, array $sort[, integer $skip[, integer $count[, array $fields]]]])`
查询当前集合，返回结果集指针 `cursor` 对象；
* `$query` - 查询内容
* `$sort`  - 排序
* `$skip`  - 结果集起始跳过；
* `$count` - 限制结果集数量；
* `$fields` - 仅返回特定字段；

#### `yield collection::find_one(array $query, array $sort)`
查询当前集合，返回单条文档记录（关联数组）；


### `class flame\db\mongodb\cursor`
封装结果集指针类型，用于访问查询结果数据；

#### `yield cursor::next()`
遍历底层指针，返回下一个文档；当遍历结束时，返回 null;

**示例**：
``` PHP
<?php
// ... 通过 collection->find_many() 获取 $cursor
while($doc = yield $cursor->next()) {
	echo json_encode($doc);
}
```

#### `yield cursor::toArray([callable $cb])`
遍历底层指针，返回结果集中的所有文档（关联数组）组成的数组；可选使用 `$cb` 回调函数对结果集的每个文档进行过滤；

**示例**：
``` PHP
<?php
// $cursor = .....
$cursor->toArray(function(&$doc) { // COW 故也可以不使用引用
	if($doc["a"] == "bbb") return true;
	return false;
});
```

**注意**：
* 不要混合使用 `next()` 和 `toArray()` 两种读取方式，可能导致未知问题；
