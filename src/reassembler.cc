#include "reassembler.hh"
#include "debug.hh"
#include "byte_stream.hh"
#include<iostream>
#include<map>
#include<algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
	//cout<<endl<<"Here I am!!! "<<" "<<first_index<<" "<<data<<" "<<is_last_substring<<endl;
  if(is_last_substring)
    this->_eof = true;

  debug("I was in here 2");

  // skip already being pushed data
  if(first_index + data.size()  <=next_expected_index){
    if(_eof && this->unpushed_data.empty())
      output_.writer().close();
    return;
  }
  if(first_index < this->next_expected_index){
    uint64_t skip = next_expected_index - first_index;
    data = data.substr(skip);
    first_index = next_expected_index;
  }


  if(this->next_expected_index != first_index){
    debug("I was in here");
    size_t pending_bytes = count_bytes_pending();
    size_t available_capacity = output_.writer().available_capacity();
    size_t free_capacity = (available_capacity > pending_bytes) ? (available_capacity - pending_bytes) : 0;

   
    if(data.size() > free_capacity){
      if(_eof && this->unpushed_data.empty())
        output_.writer().close();
      return;
    }





      this->unpushed_data[first_index] = move(data);

    if(_eof && this->unpushed_data.empty())
      output_.writer().close();
    return;
  }
  
  // check for available capacity here
  size_t remaining = output_.writer().available_capacity();
  size_t push_len = min(remaining,data.size());

  // push as much data as you can
  output_.writer().push(data.substr(0,push_len));
  this->next_expected_index += push_len;


  // checking unpushed data if those bad boys deserve to be pushed
  for(auto it = this->unpushed_data.begin(); it != this->unpushed_data.end();){
     if(it->first == next_expected_index){
      remaining = output_.writer().available_capacity();
      push_len = min(remaining,it->second.size());
      output_.writer().push(it->second.substr(0,push_len));
      this->next_expected_index += push_len;

      if(push_len == it->second.size())
        it = this->unpushed_data.erase(it);
      else{
        it->second.erase(0,push_len);
        break;
      }
    } else
      it++;

  }

  // checking if we should close the write buffer in case it is eof and the unpushed_data map is empty
  if(_eof && this->unpushed_data.empty()){
      output_.writer().close();
  }






  //debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;

  for(const auto &entry: this->unpushed_data)
    total+= entry.second.size();
  //debug( "unimplemented count_bytes_pending() called" );
  return total;
}
