#pragma once

namespace flame {
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class coroutine {
	private:
		coroutine(): status_(0) {}
		int                    status_;
		php::generator         generator_;
		typedef void (*callback_t)(php::value& rv, coroutine* co, void* data);
		typedef struct stack_t {
			callback_t cb;
			void*      data;
		} stack_t;
		std::deque<stack_t> yields_; // 用于伪装 yield 指令，实现异步动作的包裹
		template <class K, void (K::*method)(php::value& rv, coroutine* co)>
		static void method_wrapper(php::value& rv, coroutine* co, void* data) {
			(static_cast<K*>(data)->*method)(rv, co);
		}
		void run();
	public:
		static coroutine* current;

		template <typename ...Args>
		static bool start(php::callable& cb, const Args&... argv) {
			coroutine* old = current;
			current = new coroutine();
			php::value gen = cb(argv...);
			if(gen.is_generator()) {
				current->generator_ = std::move(gen);
				current->run();
				current = old;
				return true;
			}else{
				delete current;
				current = old;
				return false;
			}
		}
		inline void yield(callback_t cb, void* data = nullptr) {
			yields_.push_back(stack_t {.cb = cb, .data = data});
		}
		template <class K, void (K::*method)(php::value& rv, coroutine* co)>
		void yield(K* k) {
			yields_.push_back(stack_t {.cb = method_wrapper<K, method>, .data = k});
		}
		void next(php::value&& rv);
		void next();
		void fail(const php::exception& ex);
		static void default_close_cb(uv_handle_t* handle);
		static void prepare();
	};

	template <typename uv_type_t, typename my_type_t>
	class coroutine_context {
	protected:
		coroutine*  co_;
		my_type_t*  ct_;
		// uv_type_t 大小不固定，为支持强制转换，一定要在末尾
		uv_type_t   uv_;
	public:
		coroutine_context(coroutine* c, my_type_t* m)
		: co_(c)
		, ct_(m) { // 保存对象的引用，防止临时对象在异步过程丢失
			uv_.data = this;
		}
		inline uv_type_t* watcher() {
			return &uv_;
		}
		inline coroutine* routine() {
			return co_;
		}
		inline my_type_t* context() {
			return ct_;
		}
	};
	template <typename uv_type_t, typename my_type_t>
	class coroutine_ref: public coroutine_context<uv_type_t, my_type_t> {
	protected:
		php::value  rf_;
	public:
		coroutine_ref(coroutine* c, my_type_t* m)
		: rf_( *(php::value*)m ) // 保存对象的引用，防止临时对象在异步过程丢失
		, coroutine_context<uv_type_t, my_type_t>(c, (my_type_t*)&rf_) {
		}
	};
	php::value async();
	php::value async(void* context);
}
