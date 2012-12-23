/*
 * parallel_vector.hpp
 *
 *  Created on: 2012/12/23
 *      Author: gollerjo
 */

#ifndef PARALLEL_VECTOR_HPP_
#define PARALLEL_VECTOR_HPP_

#include <memory>
#include <vector>
#include <initializer_list>
#include <utility>
#include <type_traits>
#include <future>

namespace rlxutil {

  template <typename Elem, unsigned min_portion = 1000, typename Alloc = std::allocator<Elem>>
  class parallel_vector
  {
  public:
    static_assert(min_portion >= 1,
        "The min_portion parameter of rlxutil::parallel_vector must be 1 or greater.");

    typedef typename std::vector<Elem,Alloc>::value_type      value_type;
    typedef typename std::vector<Elem,Alloc>::allocator_type  allocator_type;
    typedef typename std::vector<Elem,Alloc>::size_type       size_type;
    typedef typename std::vector<Elem,Alloc>::difference_type difference_type;
    typedef typename std::vector<Elem,Alloc>::reference       reference;
    typedef typename std::vector<Elem,Alloc>::const_reference const_reference;
    typedef typename std::vector<Elem,Alloc>::pointer         pointer;
    typedef typename std::vector<Elem,Alloc>::const_pointer   const_pointer;
    typedef typename std::vector<Elem,Alloc>::iterator        iterator;
    typedef typename std::vector<Elem,Alloc>::const_iterator  const_iterator;
    typedef typename std::vector<Elem,Alloc>::reverse_iterator       reverse_iterator;
    typedef typename std::vector<Elem,Alloc>::const_reverse_iterator const_reverse_iterator;

  private:
    typedef const_iterator It;

  public:
    parallel_vector() :
      _data(), _offsets()
    { }

    parallel_vector(std::vector<Elem> &&vec) :
      _data(std::move(vec)), _offsets()
    { }

    parallel_vector(const std::vector<Elem> &vec) :
      _data(vec), _offsets()
    { }

    parallel_vector &operator=(std::vector<Elem> &&vec)
    {
      _data = std::move(vec);
      _offsets.clear();
      return *this;
    }

    parallel_vector &operator=(const std::vector<Elem> &vec)
    {
      _data = vec;
      _offsets.clear();
      return *this;
    }

    std::size_t num_threads() const
    { return _offsets.size(); }

    void num_threads(std::size_t threads)
    { parallelize(threads); }

    /**
     * Apply a function to all elements of the vector, running multiple
     * threads in parallel.
     * @parameter threads The number of threads to be used
     * @parameter fun The function to be applied. The function must take
     *   at least two `const_iterator` arguments, specifying the range
     *   of elements to which it will be applied. It can optionally take
     *   further arguments of any type.
     * @parameter args... Additional arguments to the function
     * @return A vector of futures. The threads may still be running when
     *   the vector is returned. Use `get()` to access each result
     *   (which may imply waiting for the corresponding thread to finish).
     */
    template <typename Fun, typename... Args>
    std::vector<typename std::future<std::result_of<Fun(It,It,Args...)>::type>>
    parallel_perform(std::size_t threads, Fun&& fun, Args&&... args)
    {
      using std::pair;
      using std::vector;
      using std::future;
      using std::async;
      using std::forward;

      typedef typename std::result_of<Fun(It,It,Args...)>::type result_type;

      if (_offsets.size() != threads)
        parallelize(threads);

      vector<future<result_type>> results
      { };

      for (const pair<It,It> &os : _offsets)
      {
        future<result_type> fut
        { async(std::launch::async,forward<Fun>(fun),os.first,os.second,forward<Args>(args)...) };
        results.push_back(std::move(fut));
      }

      return results;
    }

    /**
     * Apply a function to all elements of the vector, running multiple
     * threads in parallel. The number of threads used is the same as was
     * used in any previous call of `parallel_perform()`, or the number
     * specified in the last call to `num_threads()`, whichever happened
     * more recently. If neither function has ever been called on this
     * instance, the number of threads is 1.
     * @parameter fun The function to be applied. The function must take
     *   at least two `const_iterator` arguments, specifying the range
     *   of elements to which it will be applied. It can optionally take
     *   further arguments of any type.
     * @parameter args... Additional arguments to the function
     */
    template <typename Fun, typename... Args>
    std::vector<typename std::future<std::result_of<Fun(It,It,Args...)>::type>>
    parallel_perform(Fun&& fun, Args&&... args)
    {
      using std::forward;
      std::size_t threads = (_offsets.empty() ? 1 : _offsets.size());
      return parallel_perform(threads,forward<Fun>(fun),forward<Args>(args)...);
    }

    void assign(size_type count, const Elem &value)
    {
      _data.assign(count,value);
      _offsets.clear();
    }

    template <typename InputIt>
    void assign(InputIt first, InputIt last)
    {
      _data.assign(first,last);
      _offsets.clear();
    }

    allocator_type get_allocator() const
    { return _data.get_allocator(); }

    reference at(size_type pos)
    { return _data.at(pos); }

    const_reference at(size_type pos) const
    { return _data.at(pos); }

    reference operator[](size_type pos)
    { return _data[pos]; }

    const_reference operator[](size_type pos) const
    { return _data[pos]; }

    reference front()
    { return _data.front(); }

    const_reference front() const
    { return _data.front(); }

    reference back()
    { return _data.back(); }

    const_reference back() const
    { return _data.back(); }

    Elem *data() noexcept
    { return _data.data(); }

    const Elem *data() const noexcept
    { return _data.data(); }

    iterator begin() noexcept
    { return _data.begin(); }

    const_iterator begin() const noexcept
    { return _data.begin(); }

    const_iterator cbegin() const noexcept
    { return _data.cbegin(); }

    iterator end() noexcept
    { return _data.end(); }

    const_iterator end() const noexcept
    { return _data.end(); }

    const_iterator cend() const noexcept
    { return _data.cend(); }

    iterator rbegin() noexcept
    { return _data.rbegin(); }

    const_iterator rbegin() const noexcept
    { return _data.rbegin(); }

    const_iterator rcbegin() const noexcept
    { return _data.rcbegin(); }

    iterator rend() noexcept
    { return _data.rend(); }

    const_iterator rend() const noexcept
    { return _data.rend(); }

    const_iterator rcend() const noexcept
    { return _data.rcend(); }

    bool empty() const noexcept
    { return _data.empty(); }

    size_type size() const noexcept
    { return _data.size(); }

    size_type max_size() const noexcept
    { return _data.max_size() }

    void reserve(size_type size)
    { _data.reserve(size); }

    size_type capacity() const noexcept
    { return _data.capacity(); }

    void shrink_to_fit()
    { _data.shrink_to_fit(); }

    void clear() noexcept
    {
      _data.clear();
      _offsets.clear();
    }

    iterator insert(iterator pos, const Elem &value)
    {
      _offsets.clear();
      return _data.insert(pos,value);
    }

    iterator insert(const_iterator pos, const Elem &value)
    {
      _offsets.clear();
      return _data.insert(pos,value);
    }

    iterator insert(const_iterator pos, Elem &&value)
    {
      _offsets.clear();
      return _data.insert(pos,value);
    }

    void insert(iterator pos, size_type count, const Elem &value)
    {
      _offsets.clear();
      _data.insert(pos,count,value);
    }

    iterator insert(const_iterator pos, size_type count, const Elem &value)
    {
      _offsets.clear();
      return _data.insert(pos,count,value);
    }

    template <typename InputIt>
    void insert(iterator pos, InputIt first, InputIt last)
    {
      _offsets.clear();
      _data.insert(pos,first,last);
    }

    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
      _offsets.clear();
      return _data.insert(pos,first,last);
    }

    iterator insert(const_iterator pos, std::initializer_list<Elem> ilist)
    {
      _offsets.clear();
      return _data.insert(pos,ilist);
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args &&... args)
    {
      _offsets.clear();
      return _data.emplace(std::forward<Args>(args)...);
    }

    iterator erase(iterator pos)
    {
      _offsets.clear();
      return _data.erase(pos);
    }

    iterator erase(const_iterator pos)
    {
      _offsets.clear();
      return _data.erase(pos);
    }

    iterator erase(iterator first, iterator last)
    {
      _offsets.clear();
      return _data.erase(first,last);
    }

    iterator erase(const_iterator first, iterator last)
    {
      _offsets.clear();
      return _data.erase(first,last);
    }

    void push_back(const Elem &value)
    {
      _data.push_back(value);
      _offsets.clear();
    }

    void push_back(Elem &&value)
    {
      _data.push_back(std::move(value));
      _offsets.clear();
    }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
      _data.emplace_back(std::forward<Args>(args)...);
      _offsets.clear();
    }

    void pop_back()
    {
      _data.pop_back();
      _offsets.clear();
    }

    void resize(size_type count)
    {
      _data.resize(count);
      _offsets.clear();
    }

    void resize(size_type count, const value_type &value)
    {
      _data.resize(count,value);
      _offsets.clear();
    }

    void swap(std::vector<Elem,Alloc> &other) noexcept
    {
      _offsets.clear();
      _data.swap(other);
    }

    void swap(parallel_vector<Elem,min_portion,Alloc> &other) noexcept
    {
      _data.swap(other._data);
      _offsets.swap(other._offsets);
    }

  private:
    typedef typename std::vector<Elem,Alloc>::const_iterator It;

    /** Internal storage. */
    std::vector<Elem,Alloc>        _data;
    /** Start and end offsets for threads. Note that the standard
     * allocator (not Alloc) is **always** used for this vector. */
    std::vector<std::pair<It,It>>  _offsets;

    /**
     * Calculate the start and end offsets for all threads.
     */
    void parallelize(const std::size_t threads)
    {
      /* Sanity. */
      if (threads < 1)
        parallelize(1);
      /* Make sure we don't create too many threads. */
      const std::size_t total
      { _data.size() };
      if ((threads > 1) && (total/threads < min_portion))
        parallelize(total/min_portion==0?1:total/min_portion);

      const std::size_t portion
      { total / threads };

      _offsets.resize(threads);

      It endpos
      { begin(_data) };

      std::generate(begin(_offsets),end(_offsets),[&endpos,portion]()
      {
        It startpos
        { endpos };
        endpos += portion;
        return std::make_pair(startpos,endpos);
      });
      _offsets.back().second = end(_data);
    }
  };
}


#endif /* PARALLEL_VECTOR_HPP_ */