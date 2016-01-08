#ifndef UTILITY_TXX
#define UTILITY_TXX

#include <iterator>
#include <fstream>
#include <algorithm>

template <typename T>
File<T>::File (const std::string &file)
  : file_ (file)
{
}

template <typename T>
File<T>::~File ()
{
}

template <typename T>
void
File<T>::read (std::vector<T> &data)
{
  std::ifstream file (this->file_);

  std::copy (std::istream_iterator< T > (file),
             std::istream_iterator< T > (),
             std::back_inserter (data));
}

template <typename T>
void
File<T>::write (typename value_type::const_iterator begin,
                typename value_type::const_iterator end)
{
  std::ofstream file (this->file_, std::ios_base::trunc);
  std::copy (begin,
             end,
             std::ostream_iterator< T > (file, "\n"));
}

template <typename T>
void
File<T>::write (const std::vector<T> &data)
{
  this->write (data.begin (), data.end ());
}

template <typename T>
void
insertion_sort (std::vector <T> &data)
{
  typedef typename std::vector <T>::iterator it;

  for (it i = data.begin () + 1; i != data.end (); ++i)
    {
      it j = i;

      while ( (j != data.begin ()) &&
              (*(j - 1) > *j))
        {
          std::swap (*j, *(j - 1));
          --j;
        }
    }
}


#endif
