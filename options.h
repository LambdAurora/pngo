#include <ArduinoSTL.h>
#include "color.h"
#include "reflex.h"

typedef void(*menu_option_callback)(Reflex &);

class MenuOption
{
private:
  std::string _name;
  Color _color;
  menu_option_callback _exec;
  
public:
  MenuOption(const std::string name, const Color &color, menu_option_callback exec) : 
    _name(name), 
    _color(color),
    _exec(exec)
  {}

  const std::string &get_name() const
  {
    return _name;
  }

  const Color &get_color() const
  {
    return _color;
  }

  void execute(Reflex &reflex)
  {
    _exec(reflex);
  }
};

const MenuOption BLANK_MENU_OPTION("default", color::from_int_rgba(0, 0, 0), [](Reflex &){});
