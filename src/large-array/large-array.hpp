/**
 By Johannes Goller, created on 2012-05-01
 @author $Author: jogojapan $
 @version $Revision: 1 $
 @lastrevision $Date: 2010-05-07 20:16:23 +0900 (金, 07 5 2010) $

 Very large arrays, organized in blocks and living in boost memory
 pools.
 */

#ifndef _LARGE_ARRAY_H_
#define _LARGE_ARRAY_H_

#include <utility>
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <boost/pool/pool.hpp>

#include "../logger/logger.hpp"

namespace sufex {

  template <class T1, class T2>
  inline T1 int_div(const T1 dividend, const T2 divisor) {
    static_assert(std::is_integral<T1> && std::is_integral<T2>,
		  "sufex::int_div() (defined in large-array.hpp) must be "
		  "used with integer arguments.");
    return (dividend / divisor) + ((dividend % divisor) != 0?1:0);
  }

  template <typename ElementType, typename PoolType = boost::pool<> >
  class LargeArray {
    static_assert(std::is_scalar<ElementType>::value,
		  "LargeArray<T> can only be used if T is a scalar data type.");
  public:
    typedef uint64_t                              pos_t;
    typedef uint32_t                              block_size_t;
    typedef std::pair<ElementType *,block_size_t> dir_entry_t;
    typedef std::vector<dir_entry_t>              directory_t;
  private:
    /** Message logging. */
    std::shared_ptr<logger_t> _logger;
    /** Memory management. */
    std::shared_ptr<PoolType> _pool;
    /** Block size. */
    block_size_t              _units_per_block;
    /** Block directory. */
    directory_t               _directory;
    /** Total number of elements. */
    pos_t                     _total_size;

  public:
    static ElementType *block_addr(const dir_entry_t &dir_entry) {
      return dir_entry.first;
    }
    static block_size_t block_total(const dir_entry_t &dir_entry) {
      return dir_entry.second;
    }

    LargeArray(const std::shared_ptr<logger_t> &logger,
	       const std::shared_ptr<PoolType> &pool,
	       const blocksize_t                units_per_block = 10000000ul)
      :_logger(logger),
       _pool(pool),
       _units_per_block(units_per_block),
       _directory(),
       _total_size(0)
    {
      if (sizeof(ElementType) != pool->get_requested_size())
	throw std::invalid_argument("Attempt to initialise LargeArray for ElementType "
				    "that does match the memory pool's element size.");
    }

    /** Copy conctructor. TODO. */
    LargeArray(const LargeArray& other) = delete;

    /** Move constructor. I think this is actually the same as what the default
	move constructor would do. */
    LargeArray(LargeArray&& other) noexcept
    :_logger(std::move(other._logger)),
      _pool(std::move(other._pool)),
      _units_per_block(other._units_per_block),
      _directory(std::move(other._directory)),
      _total_size(other._total_size)
    {}

    /** Copy assignment. */
    LargeArray<ElementType,PoolType> &
    operator=(LargeArray<ElementType,PoolType> other) noexcept
    {
      std::swap(*this,other);
      return *this;
    }

    /** Reset the array to empty, but do not free the memory. Do this only
	if you are sure you purge the memory pool later. */
    void leak()
    {
      BOOST_LOG_SEV(_logger,debug) << "Resetting, but not freeing an array";
      _directory.clear();
      _total_size = 0;
    }

    /** Proper destruction (including freeing the pool memory. If you don't
     want that, because you purge the pool in one go later on, apply leak()
     first). */
    ~LargeArray()
    {
      for (const auto &entry : _directory)
	_pool->free(block_addr(entry));
    }

    /** Change the size. This may cause deallocations, allocations and
     * reallocations of blocks as necessary. */
    void set_size(const pos_t new_size)
    {
      if (new_size == 0) {
	for (const auto &entry : _directory)
	  _pool->free(block_addr(entry));
	_pool->clear();
	this->_total_size = 0;
	return;
      }
      const std::size_type new_num_blocks = int_div(new_size,this->_units_per_block);
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

    class iterator;
    iterator begin() {
      return { _directory.begin(),_directory.end(),0,_units_per_block };
    }

    ElementType get_at(const pos_t pos) const {
      const std::size_type block_index = pos / _units_per_block;
      if (block_index >= _directory.size())
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      ElementType *const block_begin = block_addr(_directory[block_index]);
      const block_size_t in_block_pos = pos % units_per_block;
      if (in_block_pos >= block_total(_directory[block_index]))
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      return block_begin[in_block_pos];
    }

    void set_at(const pos_t pos, const ElementType new_val) {
      const std::size_type block_index = pos / _units_per_block;
      if (block_index >= _directory.size())
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      ElementType *const block_begin = block_addr(_directory[block_index]);
      const block_size_t in_block_pos = pos % units_per_block;
      if (in_block_pos >= block_total(_directory[block_index]))
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      block_begin[in_block_pos] = new_val;
    }

    void inc_at(const bit64_t pos, const ElementType amount) {
      const std::size_type block_index = pos / _units_per_block;
      if (block_index >= _directory.size())
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      ElementType *const block_begin = block_addr(_directory[block_index]);
      const block_size_t in_block_pos = pos % units_per_block;
      if (in_block_pos >= block_total(_directory[block_index]))
	throw std::out_of_range("Argument of LargeArray::get_val() too large.");
      block_begin[in_block_pos] += new_val;
    }

    /** Set all elements to null. */
    void zero_all() {
      for (const auto& entry : _directory) {
	ElementType *block = block_addr(entry);
	const auto size = block_total(entry);
	memset(reinterpret_cast<void*>(block),0,sizeof(ElementType) * size);
      }
    }
};

template <typename ElementType, typename PoolType>
class LargeArray<ElementType,PoolType>::iterator
{
  /** What block we are in. */
  typename directory_t::iterator  _block_it;
  /** What the maximum block is that we can reach. */
  typename directory_t::iterator  _block_end_it;
  /** Where the current block is located. */
  ElementType                    *_block_begin;
  /** Where we are within that block. */
  block_size_t                    _in_block_pos;
  /** The length of the block (same for all blocks). */
  const block_size_t              _units_per_block;
  /** Whether we have reached the end of iteration already. */
  bool                            _eoi;

public:
  iterator(const typename directory_t::iterator block_it,
           const typename directory_t::iterator block_end_it,
           const block_size_t in_block_pos,
           const block_size_t units_per_block)
  :_block_it(block_it),
   _block_end_it(block_end_it),
   _block_begin(nullptr),
   _in_block_pos(in_block_pos),
   _units_per_block(units_per_block),
   _eoi(false)
  {
    if (block_it != block_end_it)
      _block_begin = block_addr(*_block_it);
    else
      _eoi = true;
  }
  iterator(const typename directory_t::iterator block_it,
           const typename directory_t::iterator block_end_it,
           const block_size_t units_per_block)
  :_block_it(block_it),
   _block_end_it(block_end_it),
   _block_begin(nullptr),
   _in_block_pos(0),
   _units_per_block(units_per_block),
   _eoi(false)
  {
    if (block_it != block_end_it)
      _block_begin = block_addr(*_block_it);
    else
      _eoi = true;
  }

  bool eoi() const {
    return this->_eoi;
  }

  iterator &operator++() {
    if (_eoi)
      return *this;
    ++_in_block_pos;
    if (_in_block_pos >= block_total(*_block_it)) {
      _in_block_pos = 0;
      ++_block_it;
      if (_block_it == _block_end_it)
        _eoi = true;
    }
    return *this;
  }

  iterator &operator+=(const pos_t x) {
    if (_eoi)
      return *this;
    pos_t new_pos = static_cast<pos_t>(_in_block_pos) + x;
    if (new_pos >= block_total(*_block_it)) {
      do {
        new_pos -= block_total(*_block_it);
        ++_block_it;
        if (_block_it == _block_end_it) {
          _eoi = true;
          return *this;
        }
      } while (new_pos >= block_total(_block_it->second));
    }
    _in_block_pos = static_cast<block_size_t>(new_pos);
    return *this;
  }

  ElementType &operator*() const {
    if (_eoi)
      throw std::out_of_range("Attempt to dereference an invalid LargeArray::iterator.");
    return _block_begin[_in_block_pos];
  }

};

#endif
