#include "../../flame.h"
#include "../../coroutine.h"
#include "client_request.h"
#include "client.h"
#include "client_response.h"

namespace flame {
namespace net {
namespace http {
client_request::client_request()
: easy_(nullptr)
, dns_(nullptr)
, header_(nullptr) {
	easy_ = curl_easy_init();
	dns_  = curl_slist_append(dns_, "senderb.xingyan.pdtv.io:8360:10.20.6.75");
}
php::value client_request::__construct(php::parameters& params) {
	if (params.length() >= 3) {
		prop("timeout") = static_cast<int>(params[2]);
	}else{
		prop("timeout") = 2500;
	}
	if (params.length() >= 2) {
		prop("body")   = params[1];
		prop("method") = php::string("POST");
	}else{
		prop("method") = php::string("GET");
	}
	if (params.length() >= 1) {
		prop("url") = static_cast<php::string&>(params[0]);
	}
	prop("header") = php::array(0);
	prop("cookie") = php::array(0);
	return nullptr;
}
php::value client_request::ssl(php::parameters& params) {
	php::array& opt = params[0];
	php::string vvf = opt.at("verify", 6);
	if(vvf.is_string()) {
		if(std::strncmp(vvf.c_str(), "host", 4) == 4) {
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYHOST, 1);
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYPEER, 0);
		}else if(std::strncmp(vvf.c_str(), "peer", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYPEER, 1);
		}else if(std::strncmp(vvf.c_str(), "both", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYHOST, 1);
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYPEER, 1);
		}else{ // "none"
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(easy_, CURLOPT_SSL_VERIFYPEER, 0);
		}
	}
	CURLcode r = CURLE_UNKNOWN_OPTION;
	php::string crt = opt.at("cert",4);
	if(crt.is_string()) {
		if(std::strncmp(crt.c_str() + crt.length() - 4, ".pem", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSLCERTTYPE, "PEM");
		}else if(std::strncmp(crt.c_str() + crt.length() - 4, ".der", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSLCERTTYPE, "DER");
		}
		r = curl_easy_setopt(easy_, CURLOPT_SSLCERT, crt.c_str());
	}
	if(r != CURLE_OK) {
		throw php::exception("unsupported certificate");
	}
	php::string key = opt.at("key", 3);
	if(key.is_string()) {
		if(std::strncmp(key.c_str() + key.length() - 4, ".pem", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSLCERTTYPE, "PEM");
		}else if(std::strncmp(key.c_str() + key.length() - 4, ".der", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSLCERTTYPE, "DER");
		}else if(std::strncmp(key.c_str() + key.length() - 4, ".eng", 4) == 0) {
			curl_easy_setopt(easy_, CURLOPT_SSLCERTTYPE, "ENG");
		}
		r = curl_easy_setopt(easy_, CURLOPT_SSLKEY, key.c_str());
	}
	if(r != CURLE_OK) {
		throw php::exception("unsupported private key");
	}
	php::string pass = opt.at("pass",4);
	if(pass.is_string()) {
		curl_easy_setopt(easy_, CURLOPT_KEYPASSWD, pass.c_str());
	}
	return nullptr;
}

size_t client_request::body_read_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
	client_request* self = reinterpret_cast<client_request*>(userdata);
	size_t          chunk = size * nmemb;
	if(self->size_ == 0) return 0;
	if(self->size_ <= chunk) {
		std::memcpy(ptr, self->body_, self->size_);
		chunk = self->size_;
		self->size_ = 0;
		return chunk;
	}
	std::memcpy(ptr, self->body_, chunk);
	self->size_ -= chunk;
	return chunk;
}

void client_request::build_header() {
	php::array& opt = prop("header");
	if (opt.length()) return;

	bool expect = false;
	for(auto i=opt.begin();i!=opt.end();++i) {
		php::string& key = i->first;
		php::string& val = i->second;
		if(std::strncmp(key.c_str(), "Expect", 6) == 0 || std::strncmp(key.c_str(), "expect", 6) == 0) {
			expect = true;
		}
		php::string  str(key.length() + val.length() + 3);
		sprintf(str.data(), "%.*s: %.*s", key.length(), key.data(), val.length(), val.data());
		header_ = curl_slist_append(header_, str.c_str());
	}
	if(!expect) {
		header_ = curl_slist_append(header_, "Expect: ");
	}
	curl_easy_setopt(easy_, CURLOPT_HTTPHEADER, header_);
}
void client_request::build_cookie() {
	php::buffer cookie;
	php::array& cookie_all = prop("cookie");
	for(auto i=cookie_all.begin(); i!= cookie_all.end(); ++i) {
		php::string& key = i->first.to_string();
		php::string& val = i->second.to_string();
		std::memcpy(cookie.put(key.length()), key.c_str(), key.length());
		cookie.add('=');
		std::memcpy(cookie.put(val.length()), val.c_str(), val.length());
		cookie.add(';');
		cookie.add(' ');
	}
	php::string cookie_str = std::move(cookie); // 这里会添加 '\0' 结束符
	curl_easy_setopt(easy_, CURLOPT_READDATA, cookie_str.c_str());
}
void client_request::build_option() {
	curl_easy_setopt(easy_, CURLOPT_RESOLVE, dns_);
	php::string& url = prop("url");
	if (url.is_empty()) {
		throw php::exception("client_request build failed: empty url", -1);
	}
	curl_easy_setopt(easy_, CURLOPT_URL, url.c_str());
	php::string method = prop("method", 6),
		xbody = prop("body", 4);
	if (xbody.is_array()) {
		xbody = php::build_query(xbody);
		prop("body", 4) = xbody;
	}else if (xbody.is_string()) {
		// xbody = vbody;
	}else if (!xbody.is_null()) {
		xbody.to_string();
		prop("body", 4) = xbody;
	}
	if(std::strncmp(method.c_str(), "POST", 4) == 0) {
		curl_easy_setopt(easy_, CURLOPT_POST, 1L);
		if(xbody.length() > 0) {
			curl_easy_setopt(easy_, CURLOPT_POSTFIELDS, xbody.c_str());
		}
	}else if(std::strncmp(method.c_str(), "PUT", 4) == 0) {
		curl_easy_setopt(easy_, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(easy_, CURLOPT_PUT, 1L);
		if(xbody.length() > 0) {
			body_ = xbody.data();
			size_ = xbody.length();
			curl_easy_setopt(easy_, CURLOPT_INFILESIZE, xbody.length());
			curl_easy_setopt(easy_, CURLOPT_READDATA, this);
			curl_easy_setopt(easy_, CURLOPT_READFUNCTION, body_read_cb);
		}
	}
	curl_easy_setopt(easy_, CURLOPT_TIMEOUT_MS, static_cast<long>(prop("timeout")));
	curl_easy_setopt(easy_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
	curl_easy_setopt(easy_, CURLOPT_NOPROGRESS, 1L);
}

void client_request::build() {
	build_option();
	build_header();
	build_cookie();
}

void client_request::close() {
	if(easy_) {
		curl_easy_cleanup(easy_);
		easy_   = nullptr;
	}
	if (header_) {
		curl_slist_free_all(header_);
		header_ = nullptr;
	}
}

void client_request::done_cb(CURLMsg* msg) {
	close();
}


}
}
}
