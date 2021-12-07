/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ==" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashSet() { reset(); }

   // TODO: implement the HashSet<Data>::iterator
   // o An iterator should be able to go through all the valid Data
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashSet<Data>;

   public:
      iterator(const HashSet<Data>* h){Hash = h;}
      iterator(typename vector<Data>::iterator i,const HashSet<Data>* h){Hash = h;ite = i;}
      iterator(const iterator& i){Hash = i.Hash;ite = i.ite;}
      ~iterator(){}
      const Data& operator * () const { return *ite; }
      iterator operator ++(int){
         iterator tmp = *this;
         ++this;
         return tmp;
      }
      iterator& operator ++ () { 
         size_t idx = Hash->bucketNum(*ite);
         ++ite;
         if(ite!=Hash->_buckets[idx].end())  return *this;
         else{
            idx++;
            while(idx<Hash->_numBuckets){
               if(!Hash->_buckets[idx].empty())   {
                  ite = Hash->_buckets[idx].begin();
                  return *this;
               }
               idx++;
            }
            if(idx == Hash->_numBuckets){
               ite = Hash->_buckets[idx-1].end();
               return *this;
            }
         }
         return *this;
      }
      iterator operator --(int){}
      iterator& operator --(){
         size_t idx = Hash->bucketNum(*ite);
         typename vector<Data>::iterator it = Hash->_buckets[idx].begin();
         if(ite != Hash->_buckets[idx].begin()){
            --ite;
            return *this;
         }
         else{
            idx--;
            while(idx>=0){
               if(!Hash->_buckets[idx].empty())   {
                  ite = Hash->_buckets[idx].end();
                  --ite;
                  return *this;
               }
               idx--;
            }
            if(idx == -1){
               return *this;
            }
         }  
      }
      iterator& operator = (const iterator& i){ite = i.ite;Hash = i.Hash;return *this;}
      bool operator == (const iterator& i) const {return (ite == i.ite);}
      bool operator != (const iterator& i) const {return !(ite == i.ite);}
   private:
      //member
      const HashSet<Data> *Hash;
      typename vector<Data>::iterator  ite;
   };

   void init(size_t b) { _numBuckets = b; _buckets = new vector<Data>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<Data>& operator [] (size_t i) { return _buckets[i]; }
   const vector<Data>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const { 
      for(size_t i = 0;i<_numBuckets;i++){
         if(!_buckets[i].empty())  {iterator it(_buckets[i].begin(),this);return it;}
      }
      return end(); }
   // Pass the end
   iterator end() const { 
      auto it = _buckets[_numBuckets-1].end();
      iterator ite(it,this);
      return ite; }
   // return true if no valid data
   bool empty() const { 
      for (size_t i = 0;i<_numBuckets;i++){
         if(_buckets[i].size())  return false;
      }
      return true; }
   // number of valid data
   size_t size() const { 
      size_t s = 0; 
      for (size_t i = 0;i<_numBuckets;i++){
         s += _buckets[i].size();
      }
      return s; }

   // check if d is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const Data& d) const { 
      size_t bkt_d = bucketNum(d);
      for(auto it = _buckets[bkt_d].begin();it != _buckets[bkt_d].end();++it){
         if(d == *it)   return true;
      }
      return false; }

   // query if d is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;      //why const?
   bool query(Data& d) const { 
      size_t bkt_d = bucketNum(d);
      for(auto it = _buckets[bkt_d].begin();it != _buckets[bkt_d].end();++it){
         if(d == *it){d = *it;return true;}
      }
      return false; }

   // update the entry in hash that is equal to d (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const Data& d) { 
      size_t bkt_d = bucketNum(d);
      for(auto it = _buckets[bkt_d].begin();it != _buckets[bkt_d].end();++it){
         if(d == *it){*it = d;return true;}
      }
      return false; }

   // return true if inserted successfully (i.e. d is not in the hash)
   // return false is d is already in the hash ==> will not insert
   bool insert(const Data& d) { 
      size_t bkt_d = bucketNum(d);
      for(auto it = _buckets[bkt_d].begin();it != _buckets[bkt_d].end();++it){
         if(d == *it){return false;}
      }
      _buckets[bkt_d].push_back(d);
      return true; }
   // return true if removed successfully (i.e. d is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const Data& d) { 
      size_t bkt_d = bucketNum(d);
      for(auto it = _buckets[bkt_d].begin();it != _buckets[bkt_d].end();++it){
         if(d == *it){
            auto iter = _buckets[bkt_d].end();
            --iter;
            *it=*iter;
            _buckets[bkt_d].pop_back();
            return true;}
      }
      return false;
   }

   friend class HashSet<Data>::iterator;
private:
   // Do not add any extra data member
   size_t            _numBuckets; 
   vector<Data>*     _buckets;

   size_t bucketNum(const Data& d) const {
      return (d() % _numBuckets); }
};

#endif // MY_HASH_SET_H
