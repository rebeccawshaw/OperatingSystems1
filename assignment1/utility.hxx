#ifndef FILE_HXX_
#define FILE_HXX_

template < typename T >
class File
{
public:
  typedef std::vector<T> value_type;

  File (const std::string &file);

  ~File (void);

  void read (value_type &data);

  void write (const std::vector<T> &data);

  void write (typename value_type::const_iterator begin,
              typename value_type::const_iterator end);

private:
  T convert (const std::string &item);

  // Additional data members that you may require
  std::string file_;
};

template < typename T >
void insertion_sort (std::vector <T> &data);

#include "utility.txx"

#endif
