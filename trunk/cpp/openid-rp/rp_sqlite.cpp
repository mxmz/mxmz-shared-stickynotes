#include <uuid/uuid.h>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <set>
#include <iterator>
using namespace std;
#include <opkele/exception.h>
#include <opkele/types.h>
#include <opkele/util.h>
#include <opkele/uris.h>
#include <opkele/discovery.h>
#include <opkele/association.h>
#include <opkele/sreg.h>
#include <opkele/oauth_ext.h>
using namespace opkele;
#include <opkele/prequeue_rp.h>
#include <opkele/debug.h>

#include "sqlite.h"

#undef DUMB_RP

#ifdef DUMB_RP
# define DUMBTHROW throw opkele::dumb_RP(OPKELE_CP_ "This RP is dumb")
#else
# define DUMBTHROW (void)0
#endif


#include "split.hxx"

template< char C >
struct char_is {
        bool operator()( char c ) { return c == C ; }
};

typedef std::map< std::string, std::string > map_t;

struct query_string_collector {
    map_t& m;
    query_string_collector( map_t& m ) : m(m) {}

    void operator() ( std::string kvpair ) {
            auto pair =  split < std::vector<std::string> >( kvpair, char_is<'='>(), 2 );
            pair[0] = opkele::util::url_decode(pair[0]);
            pair[1] = opkele::util::url_decode(pair[1]) ;
            std::cerr << pair[0] << " = " << pair[1] << endl;
            m[ pair[0]] = pair[1];
    }
};

map_t parse_query_string( std::string v ) {
    map_t m;
    query_string_collector collect(m);

    split( v, char_is<'&'>(), collect );
    return std::move(m);

}

map_t& remove_prefixes( map_t& m ) {
    map_t newm;
    for ( auto i = m.begin(); i != m.end() ; ++i ) {
        auto& p = *i;
        size_t dot = p.first.find('.') ;
        if ( dot != std::string::npos ) {
                newm[ p.first.substr(dot+1)].swap(p.second);
        }
    }
    newm.swap(m);
    return m;
}






class rpdb_t : public sqlite3_t {
    public:
	rpdb_t()
	    : sqlite3_t("/tmp/openid-rp-cmd.sqlite") {
		assert(_D);
		char **resp; int nrow,ncol; char *errm;
		if(sqlite3_get_table(
			_D,"SELECT a_op FROM assoc LIMIT 0",
			&resp,&nrow,&ncol,&errm)!=SQLITE_OK) {
			throw opkele::exception(OPKELE_CP_ string("DB not ready"));
		}else
		    sqlite3_free_table(resp);

	    }
};

class example_rp_t : public opkele::prequeue_RP {
    public:
	mutable rpdb_t db;
    std::string     htc;
    std::string as_id;
	int ordinal;

	example_rp_t( std::string clientid, std::string asid = "" )
	: htc(clientid), as_id(asid), ordinal(0), have_eqtop(false)  {
        if ( asid.empty() ) { // initiate
		 sqlite3_mem_t<char*> S = sqlite3_mprintf(
			"INSERT INTO ht_sessions (hts_id) VALUES (%Q)",
			htc.c_str());
		 db.exec(S);
        }
	}

	/* Global persistent store */

	opkele::assoc_t store_assoc(
		const string& OP,const string& handle,
		const string& type,const secret_t& secret,
		int expires_in) {
	    DUMBTHROW;
	    DOUT_("Storing '" << handle << "' assoc with '" << OP << "'");
	    time_t exp = time(0)+expires_in;
	    sqlite3_mem_t<char*>
		S = sqlite3_mprintf(
			"INSERT INTO assoc"
			" (a_op,a_handle,a_type,a_ctime,a_etime,a_secret)"
			" VALUES ("
			"  %Q,%Q,%Q,"
			"  datetime('now'), datetime('now','+%d seconds'),"
			"  %Q"
			" );", OP.c_str(), handle.c_str(), type.c_str(),
			expires_in,
			util::encode_base64(&(secret.front()),secret.size()).c_str() );
	    db.exec(S);
	    return opkele::assoc_t(new opkele::association(
			OP, handle, type, secret, exp, false ));
	}

	opkele::assoc_t find_assoc(
		const string& OP) {
	    DUMBTHROW;
	    DOUT_("Looking for an assoc with '" << OP << '\'');
	    sqlite3_mem_t<char*>
		S = sqlite3_mprintf(
			"SELECT"
			"  a_op,a_handle,a_type,a_secret,"
			"  strftime('%%s',a_etime) AS a_etime"
			" FROM assoc"
			" WHERE a_op=%Q AND a_itime IS NULL AND NOT a_stateless"
			"  AND ( a_etime > datetime('now','-30 seconds') )"
			" LIMIT 1",
			OP.c_str());
	    sqlite3_table_t T;
	    int nr,nc;
	    db.get_table(S,T,&nr,&nc);
	    if(nr<1)
		throw opkele::failed_lookup(OPKELE_CP_ "Couldn't find unexpired handle");
	    assert(nr==1);
	    assert(nc==5);
	    secret_t secret;
	    util::decode_base64(T.get(1,3,nc),secret);
	    DOUT_(" found '" << T.get(1,1,nc) << '\'');
	    return opkele::assoc_t(new opkele::association(
			T.get(1,0,nc), T.get(1,1,nc), T.get(1,2,nc),
			secret, strtol(T.get(1,4,nc),0,0), false ));
	}

	opkele::assoc_t retrieve_assoc(
		const string& OP,const string& handle) {
	    DUMBTHROW;
	    DOUT_("Retrieving assoc '" << handle << "' with '" << OP << '\'');
	    sqlite3_mem_t<char*>
		S = sqlite3_mprintf(
			"SELECT"
			"  a_op,a_handle,a_type,a_secret,"
			"  strftime('%%s',a_etime) AS a_etime"
			" FROM assoc"
			" WHERE a_op=%Q AND a_handle=%Q"
			"  AND a_itime IS NULL AND NOT a_stateless"
			" LIMIT 1",
			OP.c_str(),handle.c_str());
	    sqlite3_table_t T;
	    int nr,nc;
	    db.get_table(S,T,&nr,&nc);
	    if(nr<1)
		throw opkele::failed_lookup(OPKELE_CP_ "couldn't retrieve valid association");
	    assert(nr==1); assert(nc==5);
	    secret_t secret; util::decode_base64(T.get(1,3,nc),secret);
	    DOUT_(" found. type=" << T.get(1,2,nc) << '\'');
	    return opkele::assoc_t(new opkele::association(
			T.get(1,0,nc), T.get(1,1,nc), T.get(1,2,nc),
			secret, strtol(T.get(1,4,nc),0,0), false ));
	}

	void invalidate_assoc(
		const string& OP,const string& handle) {
	    DUMBTHROW;
	    DOUT_("Invalidating assoc '" << handle << "' with '" << OP << '\'');
	    sqlite3_mem_t<char*>
		S = sqlite3_mprintf(
			"UPDATE assoc SET a_itime=datetime('now')"
			" WHERE a_op=%Q AND a_handle=%Q",
			OP.c_str(), handle.c_str() );
	    db.exec(S);
	}

	void check_nonce(const string& OP,const string& nonce) {
	    DOUT_("Checking nonce '" << nonce << "' from '" << OP << '\'');
	    sqlite3_mem_t<char*>
		S = sqlite3_mprintf(
			"SELECT 1 FROM nonces WHERE n_op=%Q AND n_once=%Q",
			OP.c_str(), nonce.c_str());
	    sqlite3_table_t T;
	    int nr,nc;
	    db.get_table(S,T,&nr,&nc);
	    if(nr)
		throw opkele::id_res_bad_nonce(OPKELE_CP_ "already seen that nonce");
	    sqlite3_mem_t<char*>
		SS = sqlite3_mprintf(
			"INSERT INTO nonces (n_op,n_once) VALUES (%Q,%Q)",
			OP.c_str(), nonce.c_str());
	    db.exec(SS);
	}

	/* Session perisistent store */

	void begin_queueing() {
	    assert(! as_id.empty() );
	    DOUT_("Resetting queue for session " << htc << "/" << as_id);
	    sqlite3_mem_t<char*> S = sqlite3_mprintf(
		    "DELETE FROM endpoints_queue"
		    " WHERE as_id=%Q",
		    as_id.c_str() );
	    db.exec(S);
	}

	void queue_endpoint(const opkele::openid_endpoint_t& ep) {
	    assert(not as_id.empty() );
	    DOUT_("Queueing endpoint " << ep.claimed_id << " : " << ep.local_id << " @ " << ep.uri);
	    sqlite3_mem_t<char*> S = sqlite3_mprintf(
		    "INSERT INTO endpoints_queue"
		    " (as_id,eq_ctime,eq_ordinal,eq_uri,eq_claimed_id,eq_local_id)"
		    " VALUES (%Q,strftime('%%s','now'),%d,%Q,%Q,%Q)",
		    as_id.c_str() ,ordinal++,
		    ep.uri.c_str(),ep.claimed_id.c_str(),ep.local_id.c_str());
	    db.exec(S);
	}

	mutable openid_endpoint_t eqtop;
	mutable bool have_eqtop;

	const openid_endpoint_t& get_endpoint() const {
	    assert(! as_id.empty() );
	    if(!have_eqtop) {
		sqlite3_mem_t<char*>
		    S = sqlite3_mprintf(
			    "SELECT"
			    "  eq_uri, eq_claimed_id, eq_local_id"
			    " FROM endpoints_queue"
			    "  JOIN auth_sessions USING(as_id)"
			    " WHERE hts_id=%Q AND as_id=%Q"
			    " ORDER BY eq_ctime,eq_ordinal"
			    " LIMIT 1",htc.c_str(),as_id.c_str() );
		sqlite3_table_t T; int nr,nc;
		db.get_table(S,T,&nr,&nc);
		if(nr<1)
		    throw opkele::exception(OPKELE_CP_ "No more endpoints queued");
		assert(nr==1); assert(nc==3);
		eqtop.uri = T.get(1,0,nc);
		eqtop.claimed_id = T.get(1,1,nc);
		eqtop.local_id = T.get(1,2,nc);
		have_eqtop = true;
	    }
	    return eqtop;
	}

	void next_endpoint() {
	    assert(! as_id.empty() );
	    get_endpoint();
	    have_eqtop = false;
	    sqlite3_mem_t<char*> S = sqlite3_mprintf(
		    "DELETE FROM endpoints_queue"
		    " WHERE as_id=%Q AND eq_uri=%Q AND eq_local_id=%Q",
		    htc.c_str(),as_id.c_str(),
		    eqtop.uri.c_str());
	    db.exec(S);
	}

	mutable string _nid;

	void set_normalized_id(const string& nid) {
	    assert(! as_id.empty() );
	    sqlite3_mem_t<char*> S = sqlite3_mprintf(
		    "UPDATE auth_sessions"
		    " SET as_normalized_id=%Q"
		    " WHERE hts_id=%Q and as_id=%Q",
		    nid.c_str(),
		    htc.c_str(),as_id.c_str());
	    db.exec(S);
	    _nid = nid;
	}
	const string get_normalized_id() const {
	    assert(! as_id.empty() );
	    if(_nid.empty()) {
		sqlite3_mem_t<char*> S = sqlite3_mprintf(
			"SELECT as_normalized_id"
			" FROM"
			"  auth_sessions"
			" WHERE"
			"  hts_id=%Q AND as_id=%Q",
			htc.c_str(),as_id.c_str());
		sqlite3_table_t T; int nr,nc;
		db.get_table(S,T,&nr,&nc);
		assert(nr==1); assert(nc==1);
		_nid = T.get(1,0,nc);
	    }
	    return _nid;
	}

	void initiate(const string& usi) {
	    allocate_asid();
	    prequeue_RP::initiate(usi);
	}

    std::string thisurl;

    void set_this_url( string s ) {
                thisurl.swap(s);
    }

    const std::string get_this_url() const {
        return thisurl;
    }



	string get_self_url() const {
	    string rv = get_this_url();
	    string::size_type q = rv.find('?');
	    if(q!=string::npos)
		rv.erase(q);
	    return rv;
	}

	void allocate_asid() {
        uuid_t uuid; 
        char    uuidchr[40];
        uuid_generate(uuid);
        uuid_unparse(uuid,uuidchr);
        
	    as_id = uuidchr;

	    sqlite3_mem_t<char*> S = sqlite3_mprintf(
		    "INSERT INTO auth_sessions (as_id, hts_id)"
		    " VALUES (%Q, %Q)",
		    as_id.c_str(), htc.c_str());
	    db.exec(S);
	    DOUT_("Allocated authentication session id "<<as_id);
	}

#ifdef DUMB_RP
	virtual assoc_t associate(const string& OP) {
	    DUMBTHROW;
	}
#endif
};

#if 0
int main(int argc, char * argv[] ) {
  try {
    std::vector< std::string> args( argv, argv + argc );
    args.resize(6);

	const string& op = args[1];

	if(op=="initiate") {
        const string& clientid = args[2];
        const string& usi = args[3];
        const string& return_base_url = args[4];
        const string& return_cookie = args[5];
	    example_rp_t rp( clientid);
        rp.set_this_url(return_base_url);
	    rp.initiate(usi);
	    opkele::sreg_t ext(opkele::sreg_t::fields_NONE,opkele::sreg_t::fields_ALL);
	    //opkele::oauth_ext_t ext("blablabla");
	    opkele::openid_message_t cm;
        string return_to_args = "?cookie=" + opkele::util::url_encode(return_cookie) + "&" + "asid="+rp.as_id;
	    string loc;
			loc = rp.checkid_(cm,opkele::mode_checkid_setup,
			rp.get_this_url()+ return_to_args, 
			rp.get_self_url(),&ext).append_query(rp.get_endpoint().uri);

	     cout<< loc << endl;
	}else if(op=="confirm") {
        const string& clientid = args[2];
        const string& url = args[3];
        if ( url == "-") {
            std::cin >> args[3];
        }


        auto urlparts = split < std::vector<std::string> >(url, char_is<'?'>(), 2 );

        map_t qsmap = parse_query_string(urlparts[1]);
        const std::string asid =  qsmap["asid"] ;
        qsmap.erase("asid");
        map_t omap = std::move(remove_prefixes(qsmap));

	    opkele::openid_message_t om;
        om.swap(omap);

	    example_rp_t rp(clientid, asid);

        //rp.set_this_url(urlparts[0] + "?asid=" + asid );
        rp.set_this_url( url );

	    opkele::sreg_t ext(opkele::sreg_t::fields_NONE,opkele::sreg_t::fields_ALL);
	    rp.id_res(om,&ext);
        om.to_keyvalues(cout);
	    cout << endl
		<< "SREG fields: " << ext.has_fields << endl;
	}else if(op=="testqs") {
            const string& qs = args[2];
            map_t qsmap = parse_query_string(qs);
            cerr << "asid" << qsmap["asid"];
            qsmap.erase("asid");
            map_t omap = std::move(remove_prefixes(qsmap));
            cerr << "ns " << omap["ns"] << endl;
            cerr << "mode " << omap["mode"] << endl;
            cerr << "assoc_handle " << omap["assoc_handle"] << endl;
	}else{

	}
#ifdef OPKELE_HAVE_KONFORKA
       }catch(konforka::exception& e) {
#else
    }catch(std::exception& e){
#endif
	DOUT_("Oops: " << e.what());
	cout << 
	    "Exception:\n"
	    " what: " << e.what() << endl;
#ifdef OPKELE_HAVE_KONFORKA
	cout << " where: " << e.where() << endl;
	if(!e._seen.empty()) {
	    cout << " seen:" << endl;
	    for(list<konforka::code_point>::const_iterator
		    i=e._seen.begin();i!=e._seen.end();++i) {
		cout << "  " << i->c_str() << endl;
	    }
	}
#endif
    }
}




#endif


#include "rp.h"

class rp_sqlite : public rp {



     public:
     virtual std::string initalize( std::string clientid, std::string usi, std::string return_base_url, std::string return_cookie = "" ) {
	    example_rp_t rp( clientid);
        rp.set_this_url(return_base_url);
	    rp.initiate(usi);
	    opkele::sreg_t ext(opkele::sreg_t::fields_NONE,opkele::sreg_t::fields_ALL);
	    //opkele::oauth_ext_t ext("blablabla");
	    opkele::openid_message_t cm;
        string return_to_args = "?0=" + opkele::util::url_encode(return_cookie) + "&" + "_="+rp.as_id;
	    string loc;
			loc = rp.checkid_(cm,opkele::mode_checkid_setup,
			rp.get_this_url()+ return_to_args, 
			rp.get_self_url(),&ext).append_query(rp.get_endpoint().uri);

	     return loc;
     }


     virtual result confirm( std::string clientid, std::string url ) {

        auto urlparts = split < std::vector<std::string> >(url, char_is<'?'>(), 2 );

        map_t qsmap = parse_query_string(urlparts[1]);
        const std::string asid =  qsmap["_"] ;
        result rv;
        rv.cookie = qsmap["0"];
        qsmap.erase("_");

        map_t omap = std::move(remove_prefixes(qsmap));

	    opkele::openid_message_t om;
        om.swap(omap);

	    example_rp_t rp(clientid, asid);

        //rp.set_this_url(urlparts[0] + "?asid=" + asid );
        rp.set_this_url( url );

	    opkele::sreg_t ext(opkele::sreg_t::fields_NONE,opkele::sreg_t::fields_ALL);
	    rp.id_res(om,&ext);
        om.to_keyvalues(cout);
	    cout << endl;
    
        rv.identity = om.get_field("identity");
        rv.confirmed = om.get_field("mode") != "cancelled";
        return rv;
     }

};


std::unique_ptr<rp> make_rp_sqlite() {
    return std::unique_ptr<rp>( new rp_sqlite );
}



































