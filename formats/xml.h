#ifndef FORMATS_XML_H_
#define FORMATS_XML_H_

#include "../lib/pugixml/pugixml.hpp"

#include "format.h"

extern bool is_verbose;

class Xml : public Format
{
public:
    explicit Xml(void *p, size_t s = 0);

    bool IsValid() const
    {
        return is_valid_;
    }

    size_t Leanify(size_t size_leanified = 0) override;

private:
    bool is_valid_;
    pugi::xml_document doc_;
    pugi::xml_encoding encoding_;
};

#endif  // FORMATS_XML_H_
