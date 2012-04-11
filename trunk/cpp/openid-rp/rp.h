#ifndef rp_h_456405948069485456456049856049856049856456456049856049865049564564
#define rp_h_456405948069485456456049856049856049856456456049856049865049564564


#include <string>


class rp {

    public:
     virtual std::string initalize( std::string clientid, std::string identity, std::string return_base_url, std::string cookie = "" ) = 0;

     struct result {
            std::string identity;
            std::string cookie;
     };

     virtual result confirm( std::string clientid, std::string return_url ) = 0;

     virtual ~rp() {} 

};



#endif
