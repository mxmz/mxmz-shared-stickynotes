
#include "util/time.hxx"
#include "util/sysfuncs.h"

#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <vector>
#include <unordered_map>
#include "url.h"
#include <boost/system/system_error.hpp>
#include "boost/algorithm/string/trim.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "asio_io_service_wrapper.h"
#include "http_server.h"
#include "shared_factory.hxx"
#include "httpresponse.hxx"

#include <stdio.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio.hpp>
#include "async_runner.hxx"

#include "mxm/std.string.hxx"
#include "textutil/uri_string_pairs.hxx"
#include "deco/encode.h"
#include "deco/decode.h"
#include "deco/encode.hxx"


#include "split.hxx"
#include "http_status_codes.hxx"
#include "lru_cache_map.hxx"

#include "mxmz/json/v4/json_std.hxx"

namespace js = mxmz::json::v4::std;
namespace ju = mxmz::json::v4::util;



std::string get_filename_extention ( const std::string& name );

using namespace boost::posix_time;
using namespace boost::gregorian;

#define LOG std::cerr << to_iso_extended_string(second_clock::local_time()) << " [" << getpid()<< "]: "


    using std::cerr; using std::endl;

   
    typedef std::map< std::string, std::string > map_t;
    typedef mxm::uri_string_pair_readwrite<map_t>       uri_string_pairs_t;

template< char C >
 struct is_char {
    bool operator()(char c) { return c == C; }
  };

 typedef is_char<'/'>    is_slash;


char charlist[] = "qwertyuiopasdfghjklzxcvbnm1234567890QWERTYUIOPASDFGHJKLZXCVBNMM";
char random_char()
{
	return charlist[ rand() % (sizeof(charlist)-1) ];
}

template< int Size >
std::string random_string() {
        std::string s(Size, ' ');
		for( size_t n = 0; n < Size ; ++n )
		{
			s[n] = random_char();
		}
        return std::move(s);
    }

template< class HttpHandler >
int http_server_main( short port, int argc, char* argv[] ); 

typedef std::vector< std::string > string_vector_t;

class myhttprequesthandler:
		public http_request_handler,
		public boost::enable_shared_from_this<myhttprequesthandler>
{
	boost::asio::io_service& ios_;
	boost::asio::deadline_timer timer_;
    boost::shared_ptr< async_runner<myhttprequesthandler> > asyncr_;
    const string_vector_t                 static_paths_;
    const extension_content_type          ext2type_;
	public:
	boost::asio::io_service& get_io_service() { return ios_; }


    async_runner<myhttprequesthandler>& get_async_runner() {
        if ( ! asyncr_ ) {
    	    asyncr_.reset( new async_runner<myhttprequesthandler>( shared_from_this(), 2) );
		    timer_.expires_from_now(boost::posix_time::seconds(10));
		    timer_.async_wait( boost::bind( &myhttprequesthandler::handle_timeout, shared_from_this(),  boost::asio::placeholders::error ));
        }
        return *asyncr_;
    }

    static void log_request_begin( const http_srv_connection_ptr&  c ) {
          const http_request_msg& req = c->request();
          LOG << (void*) c.get() << " < " << req.method() << " " << req.path() <<  "?" << req.query_string() << endl;
    }
    
    static void log_request_finish( const http_srv_connection_ptr& c, const http_response_msg& r ) {
         const http_request_msg& req = c->request();
         LOG << (void*) c.get() << " > " 
                 << r.code() << " " << r.body().content_type() << " " << r.body().size() << " bytes" 
                 << " (" << req.method() << " " << req.path() <<  "?" << req.query_string() << ")" 
                 << endl;
    }

	virtual void handle_http_request( const http_srv_connection_ptr& c  );

    void handle_timeout(const boost::system::error_code& ec);

    typedef std::unordered_map< void*, http_srv_connection_ptr > connection_map_t;

    struct pending_connections {
            
            connection_map_t conns_;
            time_t           changed_;

            http_srv_connection_ptr remove( void* key ) {
                    auto it = conns_.find(key);
                    if ( it != conns_.end() ) {
                            http_srv_connection_ptr c( std::move(it->second) );
                            conns_.erase(it);
                            changed_  = time(0);
                            return std::move(c);
                    } else {
                        throw "Programming error";
                    }
            }

            void* insert( http_srv_connection_ptr c ) {
                    void* key = c.get();
                    conns_[key].swap(c);
                    changed_  = time(0);
                    return key;
            }

            void async_restart( void* key, int code, std::string reason, std::string&& body, const std::string& ctype ) {
               httpresponse<std::string> res;
               res.data.code = code;
               res.data.reason = reason;
		       res.data.bodyptr->ctype = ctype;
               res.data.bodyptr->body.swap(body) ;
               http_srv_connection_ptr c = remove(key);
               const http_request_msg& req = c->request();
               std::cout << req.method() << std::endl;
               httpresponse<std::string> thisres( std::move(res));
               log_request_finish(  c, thisres );
               c->async_restart(thisres);	
               cerr << conns_.size() << endl;
               changed_  = time(0);
            }
            void async_restart_all( int code, std::string reason, std::string&& body, const std::string& ctype ) {
               cerr << "restarting "<< conns_.size() << endl;
               httpresponse<std::string> res;
               res.data.code = code;
		       res.data.bodyptr->ctype = ctype;
               res.data.reason = http_status_description(code);
               res.data.bodyptr->body.swap(body) ;
               for( auto i = conns_.begin(); i != conns_.end();  ) {
                   const http_request_msg& req = (*i).second->request();
                   std::cout << req.method() << std::endl;
                   httpresponse<std::string> thisres(res);
                   log_request_finish(  (*i).second, thisres );
                   http_srv_connection_ptr c( std::move(i->second) );
                   c->async_restart(thisres);	
                   auto iprev = i;
                   ++i;
                   conns_.erase(iprev);
               }
               //conns_.clear();
               changed_  = time(0);
            }

            bool    empty() const { return conns_.empty(); }
            time_t  changed() const { return changed_; }
    };


    pending_connections     clients_;
    
    myhttprequesthandler( boost::asio::io_service& ios, string_vector_t&& paths )  // ctor
		:
		ios_(ios),
        timer_(ios),
        static_paths_( std::move(paths))
    {
	}



   

};


#include "rp_sqlite.h"



void 
myhttprequesthandler::handle_http_request( const http_srv_connection_ptr& c  )
{
        time_t now = time(0);
        log_request_begin(c);
        const http_request_msg& req = c->request();
        map_t uspm;
	    uri_string_pairs_t usp(uspm);
	    usp.parse( req.query_string().data(), req.query_string().size() );

        auto path =  split< std::vector<std::string> >(  req.path(), is_slash(), 3, 3 );
        const std::string& reqtype = path[1];
        const std::string& method =  c->request().method();
        if ( reqtype == "echo") {
                 mem_stor_ptr body = req.body().load();
                 std::string bodystr( body->data(), body->size() );
                 try {
                     
                     js::json_value_handle json;
                     bodystr >> json;
                     std::string echoed ;
                     echoed << json;
                     
                     httpresponse< std::string > res;
                     res.data.code = 200;
                     res.data.reason = "OK";
                     res.data.bodyptr->ctype = "application/json";
                     res.data.bodyptr->body.swap(echoed);
                     log_request_finish(  c, res );
                     c->async_restart(res);

                 } catch ( std::exception& e ) {
                    cerr << e.what() << endl;
                 }
        }
}

void 
myhttprequesthandler::handle_timeout(const boost::system::error_code& ec) {
//        cerr << "timeout expired" << endl;
        timer_.expires_from_now(boost::posix_time::seconds(30));
	    timer_.async_wait( boost::bind( &myhttprequesthandler::handle_timeout, shared_from_this(),  boost::asio::placeholders::error ));
}


int main( int argc, char* argv[] ) {
	return http_server_main<myhttprequesthandler>( 22735, argc, argv );
}




template< class HttpHandler >
int http_server_main( short port, int argc, char* argv[] ) {
    srand( time(0) + getpid() );
	io_service_wrapper_ptr 	ios = 	make_asio_io_service();
    string_vector_t doc_paths = split < string_vector_t >(mxm::enviro_default("DOCUMENT_ROOT", ".:..:../.."), is_char<':'>(), 1  );
	{
		char port_s[100];
		snprintf( port_s, 100, "%d", (int)port );
		boost::shared_ptr<myhttprequesthandler> httphndl( new myhttprequesthandler( ios->io_service(),  doc_paths   ) );
		boost::shared_ptr< http_server > httpsrv( new http_server( ios->io_service(), "0.0.0.0", port_s, httphndl ) );
		ios->run();
	}
	return 0;
}






std::string get_filename_extention ( const std::string& name ) {
    auto last_dot = name.find_last_of(".");
    if ( last_dot != std::string::npos ) {
            return name.substr( last_dot );
    } else return "";
}




