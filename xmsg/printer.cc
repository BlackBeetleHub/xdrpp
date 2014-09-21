
#include <iomanip>
#include <xdrc/printer.h>

namespace xdr {

std::string
escape_string(const std::string &s)
{
  std::ostringstream os;
  os << '\"';
  for (char c : s) {
    if (c < 0x20 || c >= 0x7f)
      os << '\\' << std::setw(3) << std::setfill('0')
	 << std::oct << (unsigned(c) & 0xff);
    else if (c == '"')
      os << "\\\"";
    else
      os << c;
  }
  os << '\"';
  return os.str();
}

std::string
hexdump(const std::uint8_t *data, size_t len)
{
  std::ostringstream os;
  os.fill('0');
  os.setf(std::ios::hex, std::ios::basefield);
  for (; len > 0; ++data, --len)
    os << std::setw(2) << unsigned(*data);
  return os.str();
}

std::ostream &
Printer::bol(const char *name)
{
  if (skipnl_)
    skipnl_ = false;
  else {
    if (comma_)
      buf_ << ",";
    buf_ << std::endl << std::string(2*indent_, ' ');
  }
  comma_ = true;
  if (name)
    buf_ << name << " = ";
  return buf_;
}

}
