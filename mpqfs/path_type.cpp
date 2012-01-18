// path_type.cpp is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

#include "path_type.h"

#include <sstream>

static void replace ( std::string& string
                    , const std::string& old
                    , const std::string& with
                    )
{
  size_t pos = string.find (old);
  while (pos != std::string::npos)
  {
    string.replace (pos, old.length(), with);
    pos = string.find (old);
  }
}


path_type::path_type()
  : std::vector<std::string> (0)
{
}

path_type::path_type (const char* c_str)
  : std::vector<std::string> (0)
{
  std::string s (c_str);
  replace (s, "\\", "/");

  std::stringstream ss (s);
  std::string item;
  while (std::getline (ss, item, '/'))
  {
    if (item != "")
    {
      push_back (item);
    }
  }
}

path_type::path_type ( const_iterator it
                     , const_iterator end
                     )
  : std::vector<std::string> (it, end)
{ }

path_type path_type::parent() const
{
  if (is_root())
  {
    return *this;
  }
  return path_type (begin(), end() - 1);
}

bool path_type::is_root() const
{
  return size() == 0;
}

bool path_type::operator== (const path_type& other) const
{
  return size() == other.size()
      && std::equal ( begin()
                    , end()
                    , other.begin()
                    );
}

bool path_type::operator< (const path_type& other) const
{
  return *this < static_cast<std::vector<std::string> > (other);
}

std::string path_type::file() const
{
  return back();
}

std::string path_type::full_path ( const std::string& delimiter
                                 , const bool& start_with_delimiter
                                 ) const
{
  if (is_root())
  {
    return delimiter;
  }

  std::string return_value;
  for (const_iterator it (begin()); it != end(); ++it)
  {
    return_value += delimiter + *it;
  }
  if (!start_with_delimiter)
  {
    return return_value.substr (1);
  }
  else
  {
    return return_value;
  }
}
