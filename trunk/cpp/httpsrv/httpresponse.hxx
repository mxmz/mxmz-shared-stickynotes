#include "http_server.h"
#include <boost/enable_shared_from_this.hpp>

  template< class BodyImplT >
  struct httpresponse : public http_response_msg
  {
	  struct http_body_impl:
		  	public http_body, public memory_storage, public boost::enable_shared_from_this<http_body_impl>
	  {
		std::string ctype;
		BodyImplT   body;
		virtual const char* data() const  { return body.data(); }
		virtual size_t      size() const    { return body.size(); }
		virtual const char* content_type() const { return ctype.c_str() ; }
		virtual mem_stor_ptr	load() const { return this->shared_from_this(); }

	  };

	  struct properties
	  {
	  	boost::shared_ptr<http_body_impl> 	bodyptr;
	  	boost::shared_ptr<http_body> 		alt_bodyptr;
		mutable std::map<std::string,std::string>	headers;
		std::string				reason;
		int					code;
		std::string				protocol;

        void swap( properties& that ) {
            reason.swap(that.reason);
            protocol.swap(that.reason);
            std::swap(code, that.code);
            bodyptr.swap(that.bodyptr);
            alt_bodyptr.swap(that.alt_bodyptr);
        }
	  };
  
      properties    data;

	  httpresponse()
	  {
		  data.bodyptr.reset( new http_body_impl );
		  data.protocol = "HTTP/1.0";
		  data.bodyptr->ctype = "text/plain";
		  data.reason = "Undefined Response";
		  data.code = 599;
	  }

      httpresponse( httpresponse&& that ) {
            that.data.swap(data);
      }


	  virtual int code() const  { return data.code; }
	  virtual const std::string& operator[]( const std::string& k ) const { return data.headers[k]; }
	  virtual const std::string& reason() const { return data.reason; }
	  virtual const std::string& version() const { return data.protocol; }

	  virtual const http_body& body() const { return data.alt_bodyptr ? *data.alt_bodyptr: *data.bodyptr; }
  	  virtual http_body_ptr	 shared_body() const { return data.alt_bodyptr ? data.alt_bodyptr : data.bodyptr; }

  };



void slurp_into( ::std::istream& in, std::vector<char>& data ) {
    data.clear();
    data.reserve( 1024*1024 );
    while ( in ) {
        size_t size = data.size();
        data.resize( size + 10*1024 );
        in.read( &data[0] + size, 10*1024 );
        data.resize( size + in.gcount()  );

    }
    if ( in.fail() and not in.eof() ) {
        throw "error while reading file";
    }
}




/*
boost::shared_ptr<strbody>
slurp( ::std::istream& in ) {
    std::vector<char>   data;
    slurp_into(in,data);
    return boost::shared_ptr<strbody>( new strbody( std::move(data), "application/octet-stream") ) ;
}

*/






class extension_content_type {
        std::unordered_map<std::string,std::string> map_;
        const std::string octet_stream_;

        public:
        extension_content_type() : octet_stream_("application/octet-stream") {
                map_[".js"] = "text/javascript";
                map_[".txt"] = "text/plain";
                map_[".html"] = "text/html";
                map_[".htm"] = "text/html";
                map_[".css"] = "text/css";
                map_[".png"] = "image/png";
                map_[".gif"] = "image/gif";
        }
        const std::string& get_content_type( const std::string& ext ) const {
                auto i = map_.find(ext);
                if ( i != map_.end() ) return (*i).second;
                else return octet_stream_;
        }

};












