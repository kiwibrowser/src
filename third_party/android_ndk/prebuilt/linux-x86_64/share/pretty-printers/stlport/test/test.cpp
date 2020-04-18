#include <bitset>
#include <deque>
#include <list>
#include <memory>
#include <map>
#include <queue>
#include <set>
#ifdef _STLPORT_VERSION
#include <slist>
#endif // _STLPORT_VERSION
#include <string>
#include <stack>
#if _STLPORT_VERSION >= 0x520
#include <unordered_map>
#include <unordered_set>
#endif //_STLPORT_VERSION >= 0x520
#include <vector>

#ifdef _STLPORT_VERSION
#define STD_TR1 std::tr1
#else // _STLPORT_VERSION
#define STD_TR1 std
#endif // _STLPORT_VERSION

struct Large {
  char m_foo[100];
};

void foo() { }

int main() {

  std::string string1 = "";
  std::string string2 = "a short string";
  std::string string3 = "a very long string ...";

  std::wstring wstring1 = L"";
  std::wstring wstring2 = L"a short string";
  std::wstring wstring3 = L"a very long string ...";

  std::vector<int> vector1;
  vector1.push_back( 100);
  vector1.push_back( 200);
  vector1.push_back( 300);

  std::vector<bool> vector2;
  vector2.push_back( true);
  vector2.push_back( false);
  vector2.push_back( true);

  std::map<int,int> map1;
  map1[1] = -1;
  map1[21] = -21;
  map1[42] = -42;

  std::multimap<int,int> multimap1;
  multimap1.insert (std::pair<int,int> (1, -1));
  multimap1.insert (std::pair<int,int> (1, -2));
  multimap1.insert (std::pair<int,int> (21, -21));
  multimap1.insert (std::pair<int,int> (21, -22));
  multimap1.insert (std::pair<int,int> (42, -42));
  multimap1.insert (std::pair<int,int> (42, -43));

  std::set<int> set1;
  set1.insert( 100);
  set1.insert( 200);
  set1.insert( 300);

  std::multiset<int> multiset1;
  multiset1.insert( 100);
  multiset1.insert( 100);
  multiset1.insert( 200);
  multiset1.insert( 200);
  multiset1.insert( 300);
  multiset1.insert( 300);

  std::list<int> list1;
  std::list<int> list2;
  list1.push_back( 100);
  list1.push_back( 200);
  list1.push_back( 300);

#ifdef _STLPORT_VERSION
  std::slist<int> slist1;
  std::slist<int> slist2;
  slist1.push_front( 100);
  slist1.push_front( 200);
  slist1.push_front( 300);
#else // _STLPORT_VERSION
  std::string slist1 = "std::slist not supported";
  std::string slist2 = "std::slist not supported";
#endif // _STLPORT_VERSION

  std::deque<int> deque1;
  deque1.push_front( 100);
  deque1.push_front( 200);
  deque1.push_front( 300);

  std::deque<Large> deque2;
  deque2.push_back( Large());
  deque2.push_back( Large());
  deque2.push_front( Large());

  std::stack<int> stack1;
  stack1.push( 100);
  stack1.push( 200);
  stack1.push( 300);

  std::queue<int> queue1;
  queue1.push( 100);
  queue1.push( 200);
  queue1.push( 300);

  std::priority_queue<int> priority_queue1;
  priority_queue1.push( 200);
  priority_queue1.push( 100);
  priority_queue1.push( 300);

  std::bitset<100> bitset1;
  bitset1[2] = 1;
  bitset1[42] = 1;
  bitset1[64] = 1;
  
  std::bitset<1> bitset2;
  bitset2[0] = 1;

#if _STLPORT_VERSION >= 0x520
  STD_TR1::unordered_map<int,int> unordered_map1;
  STD_TR1::unordered_map<int,int> unordered_map2;
  for( int i = 0; i < 5; ++i)
      unordered_map1[i*i] = -i*i;

  STD_TR1::unordered_multimap<int,int> unordered_multimap1;
  STD_TR1::unordered_multimap<int,int> unordered_multimap2;
  for( int i = 0; i < 5; ++i) {
      unordered_multimap1.insert( std::pair<int,int>( i*i, -i*i));
      unordered_multimap1.insert( std::pair<int,int>( i*i,  i*i));
  }

  STD_TR1::unordered_set<int> unordered_set1;
  STD_TR1::unordered_set<int> unordered_set2;
  for( int i = 0; i < 5; ++i)
      unordered_set1.insert( i*i);
  
  STD_TR1::unordered_multiset<int> unordered_multiset1;
  STD_TR1::unordered_multiset<int> unordered_multiset2;
  for( int i = 0; i < 5; ++i) {
      unordered_multiset1.insert( -i*i);
      unordered_multiset1.insert(  i*i);
  }
#else // _STLPORT_VERSION < 0x520
  std::string unordered_map1 = "std::tr1::unordered_map not supported";
  std::string unordered_map2 = "std::tr1::unordered_map not supported";
  std::string unordered_multimap1 = "std::tr1::unordered_multimap not supported";
  std::string unordered_multimap2 = "std::tr1::unordered_multimap not supported";
  std::string unordered_set1 = "std::tr1::unordered_set not supported";
  std::string unordered_set2 = "std::tr1::unordered_set not supported";
  std::string unordered_multiset1 = "std::tr1::unordered_multiset not supported";
  std::string unordered_multiset2 = "std::tr1::unordered_multiset not supported";
#endif // _STLPORT_VERSION < 0x520

  std::auto_ptr<Large> auto_ptr1( new Large());
  std::auto_ptr<Large> auto_ptr2;

#ifdef _STLP_USE_BOOST_SUPPORT
  STD_TR1::shared_ptr<Large> shared_ptr1( new Large);
  STD_TR1::shared_ptr<Large> shared_ptr2;
  
  STD_TR1::weak_ptr<Large> weak_ptr1( shared_ptr1);
  STD_TR1::weak_ptr<Large> weak_ptr2;
#else // _STLP_USE_BOOST_SUPPORT
  std::string shared_ptr1 = "std::tr1::shared_ptr not supported";
  std::string shared_ptr2 = "std::tr1::shared_ptr not supported";

  std::string weak_ptr1 = "std::tr1::weak_ptr not supported";
  std::string weak_ptr2 = "std::tr1::weak_ptr not supported";
#endif // _STLP_USE_BOOST_SUPPORT
  
  foo();
  return 0;
}
