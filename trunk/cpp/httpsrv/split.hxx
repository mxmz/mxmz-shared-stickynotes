#ifndef split_hxx_34958739485374958732356762357623547234059640598405698
#define split_hxx_34958739485374958732356762357623547234059640598405698

template< class VectorT, class PredT >
VectorT split( const std::string& input, PredT pred, size_t minlen = 0, size_t maxlen = UINT_MAX ) {

        VectorT v;
        std::string chunk;
        size_t sizelim = maxlen > 0 ?  maxlen - 1 : 0;
        
        std::string::const_iterator i = input.begin();
        while( i != input.end() ) {
            if ( v.size() < sizelim && pred(*i) ) {
                if ( chunk.size() ) {
                    v.push_back( std::move(chunk) );
                    chunk.clear();
                }
            }
            else {
                chunk += *i;
            }
            ++i;
        }
        if ( chunk.size() ) {
                v.push_back( std::move(chunk) );
        }
        if ( v.size() < minlen ) v.resize(minlen);
        return std::move(v);
}

template< class PredT, class CollectT >
void split( const std::string& input, PredT pred, CollectT collect, size_t minlen = 0, size_t maxlen = UINT_MAX ) {

        std::string chunk;
        size_t sizelim = maxlen > 0 ?  maxlen - 1 : 0;
        
        std::string::const_iterator i = input.begin();
        while( i != input.end() ) {
            if ( sizelim && pred(*i) ) {
                if ( chunk.size() ) {
                    collect( std::move(chunk) );
                    chunk.clear();
                    --sizelim;
                }
            }
            else {
                chunk += *i;
            }
            ++i;
        }
        if ( chunk.size() ) {
                collect( std::move(chunk) );
        }
}



#endif
