#pragma once

namespace flame {
class coroutine;
namespace net {
	typedef struct connect_request_t {
		coroutine*     co;
		php::object   obj;
		uv_connect_t  req;
	} connect_request_t;

	template <typename UV_TYPE_T, class MY_SOCKET_T>
	class client_impl {
	public:
		typedef client_impl<UV_TYPE_T, MY_SOCKET_T> impl_t;
		client_impl(MY_SOCKET_T* sock)
		: self_(sock)
		, co_(nullptr)
		, closing_(false) {
			stream.data = this;
		}
		UV_TYPE_T stream;
		php::value read(php::parameters& params) {
			if(params.length() > 0) {
				if(params[0].is_long()) {
					d_size_ = params[0];
					d_type_ = 1;
					if(d_size_ == 0) d_type_ = 0;
				}else if(params[0].is_string()) {
					d_endl_ = params[0];
					d_type_ = 2;
				}else{
					throw php::exception("read completion condition not supported");
				}
			}else{
				d_type_ = 0;
			}
			if(read()) {
				return rv_;
			}
			if(closing_) return nullptr; // 已关闭

			ref_ = self_; // 保留对象引用，防止异步调用丢失对象 // 当前对象的引用
			co_ = coroutine::current;
			uv_read_start((uv_stream_t*)&stream, alloc_cb, read_cb);
			return flame::async();
		}
		php::value write(php::parameters& params) {
			if(closing_) throw php::exception("stream is already closed");
			write_request_t* ctx = new write_request_t {
				.co  = coroutine::current,
				.ch  = this,
				.obj = self_,
				// .buf = params[0],
			};
			ctx->buf = params[0];
			ctx->req.data = ctx;
			uv_buf_t data {.base = ctx->buf.data(), .len = ctx->buf.length()};
			uv_write(&ctx->req, (uv_stream_t*)&stream, &data, 1, write_cb);
			return flame::async();
		}
		php::value close(php::parameters& params) {
			if(close(true)) {
				co_ = coroutine::current;
				return flame::async();
			}else{
				return nullptr;
			}
		}
		bool close(bool stop_read) {
			if(closing_) return false;
			closing_ = true;
			uv_close((uv_handle_t*)&stream, close_cb);
			if(stop_read && co_ != nullptr) { // 读取协程恢复
				co_->next();
			}
			co_ = nullptr;
			return true;
		}
	private:
		typedef struct write_request_t {
			coroutine*      co;
			client_impl* ch;
			php::value     obj;
			php::string    buf;
			uv_write_t     req;
		} write_request_t;

		MY_SOCKET_T*  self_;

		php::buffer    buf_;
		coroutine*     co_; // 当前协程
		php::value     rv_;
		php::value     ref_; // 当前对象的引用

		size_t      d_size_;
		php::string d_endl_;
		int         d_type_;

		bool closing_;

		bool read() {
			switch(d_type_) {
			case 0:
				if(buf_.size() > 0) {
					rv_ = std::move(buf_);
					return true;
				}
			break;
			case 1:
				if(buf_.size() >= d_size_) {
					rv_ = std::move(buf_);
					php::string& data = rv_;
					if(data.length() > d_size_) {
						std::memcpy(buf_.put(data.length() - d_size_), data.data() + d_size_, data.length() - d_size_);
						data.length() = d_size_;
					}
					return true;
				}
			break;
			case 2:
			{
				auto ff = std::search(buf_.data(), buf_.data() + buf_.size(),
					d_endl_.data(), d_endl_.data() + d_endl_.length());
				if(ff != buf_.data() + buf_.size()) { // 找到
					ff += d_endl_.length();
					d_size_ = ff - buf_.data();

					rv_ = std::move(buf_);
					php::string& data = rv_;
					if(data.length() > d_size_) {
						std::memcpy(buf_.put(data.length() - d_size_), data.data() + d_size_, data.length() - d_size_);
						data.length() = d_size_;
					}
					return true;
				}
			}
			break;
			}
			return false;
		}
		static void alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf) {
			// static char buffer[2048];
			// buf->base = buffer;
			// buf->len  = sizeof(buffer);
			auto self = reinterpret_cast<impl_t*>(handle->data);
			buf->base = self->buf_.rev(2048);
			buf->len  = 2048;
		}
		static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
			auto self = static_cast<impl_t*>(handle->data);
			if(nread == UV_EOF) {
				self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
				self->close(true);
			}else if(nread < 0) {
				self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
				self->close(false);
				self->co_->fail(uv_strerror(nread), nread);
			}else if(nread == 0) {
			}else{
				self->buf_.adv(nread);
				if(self->read()) {
					uv_read_stop((uv_stream_t*)&self->stream);
					self->ref_ = nullptr; // 重置引用须前置，防止继续执行时的副作用
					self->co_->next(self->rv_);
				}
			}
		}
		static void write_cb(uv_write_t* handle, int status) {
			auto ctx = static_cast<write_request_t*>(handle->data);
			if(status == UV_ECANCELED) {
				ctx->co->next();
			}else if(status < 0) {
				ctx->ch->close(false);
				ctx->co->fail(uv_strerror(status));
			}else{
				ctx->co->next();
			}
			delete ctx;
		}
		static void close_cb(uv_handle_t* handle) {
			impl_t* self = reinterpret_cast<impl_t*>(handle->data);
			if(self->co_) {
				self->co_->next();
			}
			delete self;
		}
	};
}
}
