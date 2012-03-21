
#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <vector>
#include <unordered_map>

#include "mongo/client/dbclient.h"
#include "mongo/client/connpool.h"

#include "split.hxx"

int random_number() { return (100000000) + ( rand() % (900000000) ); }

    template< char C >
    struct is_char {
        bool operator()(char c) { return c == C; }
    };

    typedef is_char<'/'>    is_slash;

typedef std::list< mongo::BSONObj > mongo_results_t;
struct mongo_query_result {
        mongo_results_t data;
        int             code;

        mongo_query_result() : code(500) {} 
        mongo_query_result( mongo_results_t&& data, int code ) 
                    :   
                            data(std::move(data)),
                            code(code) {} 
};

typedef boost::shared_ptr< mongo_query_result > mongo_results_ptr;

struct mongo_query_req : public shared_factory<mongo_query_req,boost::shared_ptr> {
            std::string         path;
            int                 skip;
};

struct mongo_update_req : public shared_factory<mongo_update_req,boost::shared_ptr> {
            std::string         path;
            mongo::BSONObj      data; 
            bool                meta;
            int                 tag;
            mongo_update_req() : meta(false), tag(0) {}
};


const char* name_property = "name";
const char* pid_property  = "p_id";
const char* tag_property  = "tag";
const char* data_property = "data";
const char* meta_property = "meta";

#define DB_NAME      "test"
#define COLL_NAME    "StickyNotes"

const char* collection = DB_NAME "." COLL_NAME;

const char* database_hostname = "localhost";

mongo::OID resolve_path( mongo::DBClientBase& c, const std::string& pathstr, size_t autovivify_min_depth );

struct mongo_query_job {
        mongo_results_ptr operator()( const mongo_query_req::shared_ptr& req ) {
                using namespace mongo;
                mongo_results_t results;
                int code = 200;
                try {
                    ScopedDbConnection c(database_hostname);
                    OID id = resolve_path( c.conn(), req->path, UINT_MAX );
                    if ( id != OID() ) {
                        bool childquery = req->path.size() and *(req->path.rbegin()) == '/';
                        auto_ptr<DBClientCursor> cursor =
                            childquery ? 
                            c->query( collection, QUERY( pid_property << id ) , 50, req->skip   )
                            :
                                c->query( collection, QUERY(  "_id" << id ), 50, req->skip   );

                        while( cursor.get() and cursor->more() ) {
                              results.push_back( cursor->next().getOwned() );
                        }
                    } else {
                            code = 404;
                    }
                    c.done();
                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                  }
                  return mongo_results_ptr( new mongo_query_result( std::move(results), code ) );
        }

};

mongo::OID resolve_path( mongo::DBClientBase& c, const std::string& pathstr, size_t autovivify_min_depth )  {
            using namespace mongo;

            auto path =  split< std::vector<std::string> >(  pathstr, is_slash(), 1 );

            c.ensureIndex(collection, BSON( pid_property << 1 << name_property << 1 ));
            c.ensureIndex(collection, BSON( pid_property << 1 ));

            OID pid;
            BSONObj o;
            BSONElement id;

            for( size_t i = 0; i != path.size(); ++i ) {
                    const std::string& name = path[i];

                    o = c.findOne( collection,
                                    QUERY ( 
                                            pid_property << pid
                                            << name_property << name 
                                            ) 
                                    );
                    if (not o.getObjectID(id)	) {
                        if ( i >=  autovivify_min_depth ) {
                                BSONObj command_result;
                                c.runCommand( DB_NAME , BSON (
                                                            "findAndModify" << COLL_NAME <<
                                                            "query"     <<  BSON( pid_property << pid << name_property << name ) <<
                                                            "new"       << 1 <<
                                                            "upsert"    << 1 <<
                                                            "update"    << BSON(
                                                                "$set"      << BSON( 
                                                                            pid_property << pid <<
                                                                            name_property << name 
                                                                        ) <<
                                                                        "$inc" << BSON( tag_property << 0 )
                                                                )  
                                                    ),
                                            command_result ); 

                                cerr << "update: "<< command_result.toString() << endl;
                                command_result["value"].Obj().getObjectID(id);

                            } else {
                                return OID();
                            }
                    }

                    pid = id.OID();

                    cerr << "name:" << name << " : " << pid << endl;
            }

            return pid;
}


struct mongo_update_job {
        mongo_results_ptr operator()( const mongo_update_req::shared_ptr& req,  void* key ) {
                using namespace mongo;
                mongo_results_t results;
                int code = 200;
                cerr << req->path << endl;
                int newtag = random_number();
                try {
                    ScopedDbConnection c(database_hostname);

                    OID id = resolve_path( c.conn(), req->path, 1 );

                    if ( id != OID() ) {

                        BSONObj command_result;

                        const char* target_update_property = req->meta ? meta_property : data_property;
                    
                        c->runCommand( DB_NAME , BSON (
                                                    "findAndModify" << COLL_NAME <<
                                                    "query"     <<  BSON( "_id" << id << tag_property << req->tag ) <<
                                                    "update"    << BSON(
                                                        "$set"      << BSON( 
                                                                        target_update_property  << req->data <<
                                                                        tag_property            << newtag
                                                                )
                                                        ) <<
                                                    "new" << 1
                                            ),
                                    command_result ); 

                        cerr << "update: "<< command_result.toString() << endl;
                        BSONElement v = command_result["value"];
                        if ( v.isNull() ) {
                                code = 412;
                        } else {
                            results.push_back( v.Obj().getOwned() );
                        }

                    } else {
                        code = 404;
                    }
                    
                    c.done();

                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                  }
                  return mongo_results_ptr( new mongo_query_result( std::move(results), code ) );
        }

};























