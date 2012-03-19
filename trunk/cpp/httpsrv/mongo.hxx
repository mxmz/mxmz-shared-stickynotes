
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

typedef boost::shared_ptr< mongo_results_t > mongo_results_ptr;

struct mongo_query_req : public shared_factory<mongo_query_req,boost::shared_ptr> {
            std::string         path;
            int                 skip;
};

struct mongo_update_req : public shared_factory<mongo_update_req,boost::shared_ptr> {
            std::string         path;
            mongo::BSONObj      data; 
            int                 tag;

            mongo_update_req() : tag(0) {}
};


const char* name_property = "name";
const char* pid_property  = "p_id";
const char* tag_property  = "tag";
const char* data_property = "data";

#define DB_NAME      "test"
#define COLL_NAME    "StickyNotes"

const char* collection = DB_NAME "." COLL_NAME;

const char* database_hostname = "localhost";

mongo::OID resolve_path( mongo::DBClientBase& c, const std::string& pathstr, bool autovivify );

struct mongo_query_job {
        mongo_results_ptr operator()( const mongo_query_req::shared_ptr& req ) {
                using namespace mongo;
                mongo_results_t results;
                try {
                    ScopedDbConnection c(database_hostname);
                    OID id = resolve_path( c.conn(), req->path, true );
                    bool childquery = req->path.size() && *(req->path.rbegin()) == '/';
                    auto_ptr<DBClientCursor> cursor =
                        childquery ? 
                        c->query( collection, QUERY( pid_property << id ) , 50, req->skip   )
                        :
                        c->query( collection, QUERY(  "_id" << id ), 50, req->skip   );

                    while( cursor.get() && cursor->more() ) {
                          results.push_back( cursor->next().getOwned() );
                    }
                    c.done();
                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                  }
                  return mongo_results_ptr( new mongo_results_t( std::move(results) ) );
        }

};

mongo::OID resolve_path( mongo::DBClientBase& c, const std::string& pathstr, bool autovivify )  {
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
                cerr << req->path << endl;
                int newtag = random_number();
                try {
                    ScopedDbConnection c(database_hostname);

                    OID id = resolve_path( c.conn(), req->path, true );

                    BSONObj command_result;
                    
                    c->runCommand( DB_NAME , BSON (
                                                    "findAndModify" << COLL_NAME <<
                                                    "query"     <<  BSON( "_id" << id << tag_property << req->tag ) <<
                                                    "update"    << BSON(
                                                        "$set"      << BSON( 
                                                                        data_property   << req->data <<
                                                                        tag_property    << newtag
                                                                )
                                                        ) <<
                                                    "new" << 1
                                            ),
                                    command_result ); 

                    cerr << "update: "<< command_result.toString() << endl;
                    results.push_back( command_result["value"].Obj().getOwned() );
                    
                    c.done();

                  } catch( DBException &e ) {
                    cerr << "caught " << e.what() << endl;
                  }
                  return mongo_results_ptr( new mongo_results_t( std::move(results) ) );
        }

};






















