#include "flame.h"
#include "coroutine.h"

namespace flame {
	coroutine* coroutine::current;

	void coroutine::prepare() {

	}
	php::value async() {
		// 某些销毁逻辑流程可能不存在协程上下文
		if(coroutine::current == nullptr) return nullptr;
		if(coroutine::current->status_ != 0) {
			php::fail("keyword 'yield' missing before async function");
			coroutine::current->close();
			uv_stop(flame::loop);
			return nullptr;
		}
		++ coroutine::current->status_;
		php::value v;
		ZVAL_PTR((zval*)&v, coroutine::current);
		return std::move(v);
	}
	php::value async(void* context) {
		// 某些销毁逻辑流程可能不存在协程上下文
		if(coroutine::current == nullptr) return nullptr;
		if(coroutine::current->status_ != 0) {
			php::fail("keyword 'yield' missing before async function");
			coroutine::current->close();
			uv_stop(flame::loop);
			return nullptr;
		}
		++ coroutine::current->status_;
		php::value v;
		ZVAL_PTR((zval*)&v, context);
		return std::move(v);
	}
	coroutine::coroutine(coroutine* parent)
	: status_(0)
	, parent_(parent) {
		// 用于防止“协程”未结束时提前结束
		uv_async_init(flame::loop, &async_, nullptr);
		async_.data = this;
	}
	static void finish_cb(uv_handle_t* handle) {
		delete reinterpret_cast<coroutine*>(handle->data);
	}
	void coroutine::close() {
		status_ = -1;
		uv_close((uv_handle_t*)&async_, finish_cb);
	}
	void coroutine::run() {
		while(status_ >= 0) {
			if(EG(exception) || !generator_.valid()) {
				// 可能由于 valid() 调用，引发新的异常（故需要二次捕获）
				if(EG(exception)) {
					close();
					uv_stop(flame::loop);
				}else if(this->parent_ != nullptr) {
					coroutine* parent = this->parent_;
					php::value rv = generator_.get_return();
					parent->next(rv);
					close();
				}else{
					close();
				}
				break;
			}
			php::value v = generator_.current();
			if(v.is_pointer()) { // 使用内置 IS_PTR 类型标识异步操作
				if(--status_ != 0) {
					php::fail("keyword 'yield' missing before async function");
					close();
					uv_stop(flame::loop);
				}
				break;
			}else if(v.is_generator()) { // 简化 yield from 调用方式可直接 yield
				coroutine::start(this, std::move(v));
				break;
			}else{
				generator_.send(std::move(v));
			}
		}
	}
	void coroutine::next(php::value& rv) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;
		if(rv.is_exception()) {
			while(!yields_.empty()) {
				yields_.pop_front();
			}
			generator_.throw_exception(rv);
			run();
		}else if(yields_.empty()) {
			generator_.send(rv);
			run();
		}else{
			stack_t st = yields_.front();
			yields_.pop_front();
			st.cb(rv, current, st.data);
		}
		current = old;
	}
	void coroutine::fail(const php::exception& ex) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;
		while(!yields_.empty()) {
			yields_.pop_front();
		}
		generator_.throw_exception(ex);
		run();
		current = old;
	}

}
