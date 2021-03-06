#include "time.h"
#include "../coroutine.h"
#include "ticker.h"

namespace flame {
namespace time {
	static void sleep_timer_cb(uv_timer_t* handle) {
		auto ctx = static_cast<coroutine_context<uv_timer_t, void>*>(handle->data);
		ctx->routine()->next();
		delete ctx;
	}
	static php::value sleep(php::parameters& params) {
		auto ctx = new coroutine_context<uv_timer_t, void>(coroutine::current, nullptr);
		int64_t ms = params[0];
		uv_timer_init(flame::loop, ctx->watcher());
		// default_timer_cb -> 继续协程 删除 context 对象
		uv_timer_start(ctx->watcher(), sleep_timer_cb, ms, 0);
		// 标记异步任务的特殊返回值
		return flame::async();
	}
	// 示例：包裹 sleep 异步函数
	// -------------------------------------------------------------------------
	static void sleep2_timer_cb(php::value& rv, coroutine* co, void* data) {
		// std::fprintf(stderr, "sleep2\n");
	}
	static php::value sleep_wrapper(php::parameters& params) {
		sleep(params);
		coroutine::current->yield(sleep2_timer_cb);
		// !!! 使用了 yield 函数，当发生同步异常时，需要调用 clear 删除
		return flame::async();
	}
	// -------------------------------------------------------------------------
	static php::value now(php::parameters& params) {
		return now();
	}
	static int64_t real_time_diff;
	void init(php::extension_entry& ext) {
		uv_update_time(flame::loop);
		int64_t rtime = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		real_time_diff = uv_now(flame::loop) - rtime;

		ext.add<flame::time::sleep>("flame\\time\\sleep");
		ext.add<flame::time::now>("flame\\time\\now");
		php::class_entry<ticker> class_ticker("flame\\time\\ticker");
		class_ticker.add(php::property_entry("interval", 1000));
		class_ticker.add(php::property_entry("repeat", bool(true)));
		class_ticker.add<&ticker::__construct>("__construct");
		class_ticker.add<&ticker::start>("start");
		class_ticker.add<&ticker::stop>("stop");
		ext.add(std::move(class_ticker));
		ext.add<flame::time::tick>("flame\\time\\tick");
		ext.add<flame::time::after>("flame\\time\\after");
	}
	int64_t now() {
		// 代价较每次取系统时间要低一些
		return uv_now(flame::loop) - real_time_diff;
	}
}
}
