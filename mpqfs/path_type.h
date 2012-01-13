#ifndef __PATH_TYPE_H
#define __PATH_TYPE_H

#include <vector>
#include <string>

class path_type : protected std::vector<std::string>
{
public:
  path_type();
  path_type (const char* c_str);
  path_type (const_iterator it, const_iterator e);

  std::string file() const;
  std::string full_path ( const std::string& delimiter = "/"
                        , const bool& start_with_delimiter = true
                        ) const;

  bool operator== (const path_type& other) const;
  bool operator< (const path_type& other) const;

  bool is_root() const;

  path_type parent() const;
};

#endif
