#ifndef lru_cache_pam_h_4569847984759485749857495874958749587495847594875948759485794857498574
#define lru_cache_pam_h_4569847984759485749857495874958749587495847594875948759485794857498574


#include <iostream>
#include <map>
#include <string>

using std::cerr;
using std::endl;

template< class KeyT, class CachedT >
class lru_cache_map
{
	class list_node;

	typedef KeyT key_t;
	
	typedef std::map< key_t, list_node* > map_t;


	struct list_node
	{
		CachedT   value;
		typename map_t::iterator position;
		list_node*      prev;
		list_node*      next;
		
		list_node( typename map_t::iterator p )
			: position(p) , prev(0), next(0)
		{
		}
	};

	const size_t max_size_;

	map_t        map_;
	list_node*   head_;
	list_node*   tail_;

	void move_front( list_node* p )
	{
		if( p == head_ ) return;
//		cerr << "move_front " << p << endl;
//		cerr << " p->prev " << p->prev << endl;
//		cerr << " p->next " << p->next << endl;
//		cerr << "head_ " << head_ << endl;
//		cerr << "tail_ " << tail_ << endl;
		if( p->prev )    p->prev->next = p->next;
		if( p->next )    p->next->prev = p->prev;
		p->next = head_;
		if( head_ )      head_->prev = p;
		head_ = p;
		if( ! tail_ )    tail_ = head_;
		else if( p == tail_ ) tail_ = p->prev;
		p->prev = 0;
//		cerr << " move_front " <<(p->position->first) << endl; 
//		cerr << " head_ " << head_ << endl;
//		cerr << " tail_ " << tail_ << endl;
////		cerr << " tail_->prev " << tail_->prev << endl;
//		cerr << " p->next " << p->next << endl;

	}
	void delete_last()
	{
		list_node* temp = tail_;
		tail_ = temp->prev;
		if( temp == head_ ) head_ = 0;
		cerr << "deleting " <<(temp->position->first) << endl; 
		map_.erase( temp->position );
		delete temp;
		cerr << "tail_ " << tail_ << endl;
	}


	public:
	lru_cache_map( size_t max_size )
		: max_size_(max_size), head_(0), tail_(0)
	{
	}
	~lru_cache_map()
	{
		cerr << "~" << endl;
		while( head_ ) delete_last();
	}


	CachedT* lookup( const key_t& k )
	{
		std::pair< typename map_t::iterator, bool > ibp = map_.insert( typename map_t::value_type(k,0) );
		if( ibp.second )
		{
			list_node* newnode = new list_node( ibp.first );
			cerr << k << " created" << endl;
			move_front(newnode);
			ibp.first->second = newnode;
			
			if( map_.size() > max_size_ ) 
				delete_last();
		}
		else
		{
			cerr << k << " found" << endl;
			move_front( ibp.first->second );

		}
		return &(ibp.first->second->value);

	}


};


#if 0 


int main()
{

	typedef lru_cache_map< std::string, int > cache_t;

	cache_t c(4);


	int* a1 = c.lookup("a1");

	*a1 = 10;

	int* a1b = c.lookup("a1");

	cerr << *a1b << endl;

	int* a2 = c.lookup("a2");
	
	cerr << *a2 << endl;
	
	int* a3 = c.lookup("a3");


	int* a4 = c.lookup("a4");
	int* a1c = c.lookup("a1");
	int* a5 = c.lookup("a5");
	int* a6 = c.lookup("a6");

}


#endif 












#endif


