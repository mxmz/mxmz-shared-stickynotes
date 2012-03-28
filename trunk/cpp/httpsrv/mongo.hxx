
#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <vector>
#include <unordered_map>

#include "mongo/client/dbclient.h"
#include "mongo/client/connpool.h"

#include "split.hxx"


template< int UpperLimit = 10000000 >
int random_number() { return (UpperLimit/10) + ( rand() % (9*(UpperLimit/10)) ); }

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
                    :   data(std::move(data)),
                        code(code) 
        {} 
};

typedef boost::shared_ptr< mongo_query_result > mongo_results_ptr;

struct mongo_query_req : public shared_factory<mongo_query_req,boost::shared_ptr> {
            std::string         path;
            int                 skip;
            long long int       lock;
            mongo_query_req() : lock(1) { }
};

enum update_types { None, UpdateData, UpdateMeta, Delete  };

struct mongo_update_req : public shared_factory<mongo_update_req,boost::shared_ptr> {
            std::string         path;
            mongo::BSONObj      data; 
            update_types        type;
            long long int       tag;
            long long int       lock;

            mongo_update_req() : type(None), tag(1), lock(1) {}
};


const char* name_property = "name";
const char* pid_property  = "p_id";
const char* tag_property  = "tag";
const char* data_property = "data";
const char* meta_property = "meta";
const char* mtime_property = "mtime";

const char* locktag_property = "lock";

#define DB_NAME      "test"
#define COLL_NAME    "StickyNotes"

const char* collection = DB_NAME "." COLL_NAME;

const char* database_hostname = "localhost";

struct creation_failed {} ;
struct creation_not_allowed {}  ;


typedef std::pair< mongo::OID, mongo::OID> oid_pair;

oid_pair resolve_path( mongo::DBClientBase& c, const std::string& pathstr, size_t autovivify_min_depth );

struct mongo_query_job {
        mongo_results_ptr operator()( const mongo_query_req::shared_ptr& req ) {
                using namespace mongo;
                mongo_results_t results;
                int code = 200;
                try {

                        ScopedDbConnection c(database_hostname);
                        oid_pair ids = resolve_path( c.conn(), req->path, UINT_MAX );
                        OID id = ids.first;
                        bool childquery = req->path.size() and *(req->path.rbegin()) == '/';
                        auto_ptr<DBClientCursor> cursor =
                            childquery ? 
                            c->query( collection, QUERY( pid_property << id ) , 50, req->skip   )
                            :
                                c->query( collection, QUERY(  "_id" << id ), 50, req->skip   );

                        while( cursor.get() and cursor->more() ) {
                              results.push_back( cursor->next().removeField("_id").removeField("p_id").getOwned() );
                              cerr << *results.begin() << endl;
                        }
                        if ( results.empty() and not childquery ) {
                                code = 404;
                        }
                        c.done();

                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                          code = 581;  
                  } catch( creation_not_allowed& ) {
                          code = 403;
                  } catch( creation_failed& ) {
                          code = 581;  
                  }
                  return mongo_results_ptr( new mongo_query_result( std::move(results), code ) );
        }

};

struct mongo_update_job {
        mongo_results_ptr operator()( const mongo_update_req::shared_ptr& req,  void* key ) {
                using namespace mongo;
                mongo_results_t results;
                int code = 200;
                cerr << req->path << endl;
                try {
                        ScopedDbConnection c(database_hostname);

                        oid_pair ids = resolve_path( c.conn(), req->path, 1 );
                        OID id = ids.first;
                        OID pid = ids.second;
               

                        const char* target_update_property = data_property;

                        switch( req->type ) {
                                case None       : {  
                                                  
                                                  
                                                  }
                                                  break;
                                case UpdateMeta : target_update_property = meta_property;
                                case UpdateData : {
                                                    cerr <<         BSON( "_id" << id << tag_property << req->tag ) << endl;
                                                    struct timeval tvnow;
                                                    gettimeofday(&tvnow, NULL);
                                                    long long int mtime = (long long int)tvnow.tv_sec * 1000 + (tvnow.tv_usec/1000);
                                                    long long int newtag =  mtime * 1000  + random_number<1000>();
                                                    long long int baselock_now = mtime*1000;
                                                    BSONObj query =  BSON( 
                                                                        "_id" << id << 
                                                                        tag_property << req->tag <<
                                                                        "$or" << BSON_ARRAY(
                                                                                    BSON ( locktag_property << req->lock ) 
                                                                                    <<
                                                                                    BSON (
                                                                                            locktag_property << BSON ( 
                                                                                                    "$lt" << baselock_now )
                                                                                            ) 
                                                                                
                                                                                )
                                                                        
                                                                        
                                                                        );

                                                    cerr << query << endl;

                                                    BSONObj command_result;
                                                     c->runCommand( DB_NAME , BSON (
                                                            "findAndModify" << COLL_NAME <<
                                                            "query"     <<  query <<
                                                            "update"    << BSON(
                                                                "$set"      << BSON( 
                                                                                target_update_property  << req->data <<
                                                                                tag_property            << newtag    <<
                                                                                mtime_property << mtime <<
                                                                                locktag_property << 0
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
                                                        results.push_back( v.Obj().removeField("_id").removeField("p_id").getOwned() );
                                                    }
                                                 }
                                                 break;
                                case Delete     :    {
                                                          c->remove( collection, QUERY( "_id" << id << tag_property << req->tag ) );
                                                }
                        }
                        
                        int siblings = c->count( collection, BSON ( pid_property << pid  )  ); 

                        cerr << id << ": siblings = " << siblings << endl;

                        c.done();

                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                          code = 581;  
                  } catch( creation_not_allowed& ) {
                          code = 403;
                  } catch( creation_failed& ) {
                          code = 581;  
                  }

                  return mongo_results_ptr( new mongo_query_result( std::move(results), code ) );
        }

};


oid_pair resolve_path( mongo::DBClientBase& c, const std::string& pathstr, size_t autovivify_min_depth )  {
            using namespace mongo;

            auto path =  split< std::vector<std::string> >(  pathstr, is_slash(), 1 );

            c.ensureIndex(collection, BSON( pid_property << 1 << name_property << 1 ));
            c.ensureIndex(collection, BSON( pid_property << 1 ));

            if ( pathstr.size() >= 25 and pathstr[0] == '=' ) {
               OID oid( pathstr.substr(1,24));
               BSONObj    o = c.findOne( collection, QUERY ( "_id" << oid  )  ); 

               BSONElement id;
               if ( o.getObjectID(id) ) {
                    return oid_pair( oid, o["p_id"].OID() );
               } else {
                    return oid_pair( OID(), OID() );
               }

            }
            OID ppid;
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
                    if ( not o.getObjectID(id)	) {
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
                                                                        "$inc" << BSON( tag_property << 0 << locktag_property << 0 )
                                                                )  
                                                    ),
                                            command_result ); 

                                cerr << "update: "<< command_result.toString() << endl;
                                BSONElement v = command_result["value"];
                                if ( v.isNull()  or not v.Obj().getObjectID(id) ) {
                                        throw creation_failed();
                                } 

                            } else {
                                throw creation_not_allowed();
                            }
                    }
                    ppid = pid;
                    pid = id.OID();

                    cerr << "name:" << name << " : " << pid << endl;
            }

            return oid_pair(pid,ppid);
}














