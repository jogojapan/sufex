/**
 By Johannes Goller, created on 2009-05-12
 @author $Author: goller $
 @version $Revision: 623 $
 @lastrevision $Date: 2009-05-12 18:52:37 +0900 (火, 12  5月 2009) $

 Very large arrays, organized in blocks.

 Copyright (c) 2009-2010 Glossatec KK
 Yokohama, Kachida-Minami 1-11-20, 224-0036 Japan
 ALL RIGHTS RESERVED
 */

#ifndef _LARGE_ARRAY_T_
#define _LARGE_ARRAY_T_

#include "./large-array.hpp"
#include <cstring>
#include <vector>

namespace cis {

template <class T, class mem_T>
large_array_t<T,mem_T>::large_array_t(const large_array_t &LA)
:_logger(LA._logger),_mem(LA._mem),_units_per_block(LA._units_per_block),
_directory(),_total_size(LA._total_size)
{
  this->_directory.reserve(LA._directory.size());
  typename directory_t::const_iterator itD(LA._directory.begin());
  while (itD != LA._directory.end()) {
    this->_directory.push_back(std::make_pair(this->_mem->alloc(itD->second),
                                              itD->second));
    T *src_ptr = this->_mem->access(itD->first);
    T *ptr = this->_mem->access(this->_directory.back().first);
    memcpy(reinterpret_cast<void *>(ptr),
           reinterpret_cast<void *>(src_ptr),sizeof(T) * itD->second);
    this->_mem->leave(itD->first);
    this->_mem->leave(this->_directory.back().first);
    ++itD;
  }
}

template <class T, class mem_T>
large_array_t<T,mem_T> &large_array_t<T,mem_T>::operator=
  (const large_array_t<T,mem_T> &LA)
{
  this->dealloc();
  this->_logger          = LA._logger;
  this->_mem             = LA._mem;
  this->_units_per_block = LA._units_per_block;
  this->_total_size      = LA._total_size;
  this->_directory.reserve(LA._directory.size());
  typename directory_t::const_iterator itD(LA._directory.begin());
  while (itD != this->_directory.end()) {
    this->_directory.push_back(std::make_pair(this->_mem->alloc(itD->second),
                                              itD->second));
    T *src_ptr = this->_mem->access(itD->first);
    T *ptr = this->_mem->access(this->_directory.back().first);
    memcpy(reinterpret_cast<void *>(ptr),
           reinterpret_cast<void *>(src_ptr),sizeof(T) * itD->second);
    this->_mem->leave(itD->first);
    this->_mem->leave(this->_directory.back().first);
    ++itD;
  }
  return *this;
}

template <class T, class mem_T>
void large_array_t<T,mem_T>::dealloc() {
  typename directory_t::iterator itE(this->_directory.begin());
  while (itE != this->_directory.end()) {
    this->_mem->dealloc(itE->first,itE->second);
    itE->second = 0;
    ++itE;
  }
  this->_directory.clear();
}

template <class T, class mem_T>
void large_array_t<T,mem_T>::set_size(const bit64_t new_size) {
  if (new_size == 0) {
    this->dealloc();
    this->_total_size = 0;
    return;
  }
  const bit32_t new_num_blocks = int_div(new_size,this->_units_per_block);
  while (new_num_blocks < this->_directory.size()) {
    this->_mem->dealloc(this->_directory.back().first,
        this->_directory.back().second);
    this->_directory.pop_back();
  }
  if (new_num_blocks > this->_directory.size()) {
    while ((new_num_blocks-1) > this->_directory.size()) {
      this->_directory.push_back(std::make_pair(this->_mem->alloc(this->_units_per_block),
                                                this->_units_per_block));
    }
    bit32_t new_num_last_block = (new_size % this->_units_per_block);
    if (new_num_last_block == 0)
      new_num_last_block = this->_units_per_block;
    this->_directory.push_back(std::make_pair(this->_mem->alloc(new_num_last_block),
                                              new_num_last_block));
  } else {
    bit32_t new_num_last_block = (new_size % this->_units_per_block);
    if (new_num_last_block == 0)
      new_num_last_block = this->_units_per_block;
    dir_entry_t &last_entry = this->_directory.back();
    last_entry.first = this->_mem->realloc(last_entry.first,new_num_last_block);
    last_entry.second = new_num_last_block;
  }
  this->_total_size = new_size;
}

template <class T, class mem_T>
void large_array_t<T,mem_T>::add(large_array_t<T,mem_T> &src_LA,
                                 bit16_t max_threads /*= 1*/)
{
  /* Check assumptions. */
  assert(max_threads > 0);
  assert(this->_total_size == src_LA._total_size);
  assert(this->_total_size != 0);

  /* Create thread vector. */
  std::vector<large_array_plus_thread_t<T,mem_T> *> vec;
  if (this->_total_size < (static_cast<bit64_t>(max_threads) * 500000ull))
    max_threads = 1 + (this->_total_size / 500000);
  iterator dest_it(this->begin());
  iterator src_it(src_LA.begin());
  const bit64_t avg_range = this->_total_size / max_threads;
  for (bit16_t cur_thread = 0; cur_thread < (max_threads - 1); ++cur_thread) {
    vec.push_back(new large_array_plus_thread_t<T,mem_T>(src_it,dest_it,avg_range));
    src_it += avg_range;
    dest_it += avg_range;
  }
  vec.push_back(new large_array_plus_thread_t<T,mem_T>(src_it,dest_it,
      (this->_total_size - ((max_threads-1) * avg_range))));

  /* Run the threads. */
  run_thread_vec(vec,max_threads,true,this->_logger);

  /* Clean up. */
  typename std::vector<large_array_plus_thread_t<T,mem_T> *>::iterator
    itT(vec.begin());
  while (itT != vec.end()) {
    delete *itT;
    ++itT;
  }
}

template <class T, class mem_T>
void large_array_t<T,mem_T>::set_all(const T val)
{
  typename directory_t::iterator itD(this->_directory.begin());
  while (itD != this->_directory.end()) {
    T *ptr = this->_mem->access(itD->first);
    memset(reinterpret_cast<void *>(ptr),
           val,sizeof(T) * itD->second);
    this->_mem->leave(itD->first);
    ++itD;
  }
}

} // end of namespace

#endif
