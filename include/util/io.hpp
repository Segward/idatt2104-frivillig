#ifndef IO_HPP
#define IO_HPP

// Small file IO utility. Throws std::runtime_error on any failure rather than
// returning empty strings, so callers can't silently propagate a missing file
// as valid empty content.
class IO {
  public:
    static std::string read(const std::string& path);
};

#endif
