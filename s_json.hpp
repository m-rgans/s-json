#ifndef SJSON_HPP
#define SJSON_HPP

#include <istream>
#include <string>
#include <vector>
#include <map>

// temporary so vs code isnt a pain about it
#define SJSON_OBJECT
#define SJSON_TEST

// Set this to zero to treat json with trailing commas as invalid.
#define JSON_ALLOW_TRAILING_COMMA 1

namespace sjson
{

    // While there is no difference between reals and ints in json, it could be important
    // to make this distinction in cpp
    typedef enum {
        EMPTY = 0, NONE = 0,
        STRING, INTEGER, REAL, ARRAY, OBJECT,
    } NodeType;

    class Node {
        public:

            typedef std::string string;
            typedef std::map<string, Node> object;
            typedef std::vector<Node> array;
            typedef double real;
            typedef long int integer;

            class coercion_invalid:std::exception {};
            class json_invalid:std::exception {};

            // Parse from json
            static Node parse_from_istream(std::istream);
            static Node from_file_path(std::string);
            static Node parse_from_string(std::string);
            static Node parse_from_string(const char*);

            //todo: messagepack

            Node& operator=(const Node&);

            Node(const string&);
            Node(const real&);
            Node(const integer&);
            Node(const array&);
            Node(const object&);
            // do some template shit for arrays and objects

            // need these since we have heap allocated data to worry about
            Node(const Node&);
            ~Node();

            NodeType get_type() const;
            void set_type(const NodeType&);

            //string
            string as_string() const;
            void set_string(string);

            //Number
            real as_real() const;
            integer as_int() const;
            void set_number(real);
            void set_number(integer);

            //Array
            const array as_array() const;
            void set_array(std::vector<Node>);

            const object as_object() const;
            void set_object(const object&);
            
        private:

            void _destroy_variant();

            void _copy_variant(const Node&);

            NodeType base_type = NONE;
            bool number_is_integer = false;

            // more troublesome, but saves on memory
            union {
                array* array;
                object* object;

                string* string;

                real* real;
                integer* integer;
            } variant;

    };

    

} // namespace sjson


#endif


#ifdef SJSON_OBJECT

namespace sjson {

    Node::Node(const Node& base) {
        _copy_variant(base);
    }

    Node::Node(const string& content) {
        base_type = STRING;
        variant.string = new string(content);
    }

    Node::Node(const integer& content) {
        base_type = INTEGER;
        variant.integer = new integer(content);
    }

    Node::Node(const real& content) {
        base_type = REAL;
        variant.real = new real(content);
    }

    Node::Node(const array& content) {
        base_type = ARRAY;
        variant.array = new array(content);
    }

    Node::Node(const object& content) {
        base_type = OBJECT;
        variant.object = new object(content);
    }

    Node& Node::operator=(const Node& other) {
        _copy_variant(other);
        return *this;
    }

    Node::~Node() {
        _destroy_variant();
    }

    void Node::set_type(const NodeType& t) {
        switch (t)
        {
        case STRING:
            *this = Node(as_string());
            break;
        case INTEGER:
            *this = Node(as_int());
            break;
        case REAL:
            *this = Node(as_real());
            break;
        case ARRAY:
            *this = Node(as_array());
            break;
        case OBJECT:
            *this = Node(as_object());
            break;
        }
    }

    Node::string Node::as_string() const {
        
    }

    void Node::_destroy_variant() {
        switch (base_type)
        {
        case NONE:
            break;

        case INTEGER:
            delete variant.integer;
            break;

        case REAL:
            delete variant.real;
            break;

        case STRING:
            delete variant.string;
            break;

        case ARRAY:
            delete variant.array;
            break;

        case OBJECT:
            delete variant.object;
            break;
        }
    }

    void Node::_copy_variant(const Node& base) {
        base_type = base.base_type;
        switch (base_type)
        {
        case NONE:
            break;

        case INTEGER:
            variant.integer = new integer(*base.variant.integer);
            break;

        case REAL:
            variant.real = new real(*base.variant.real);
            break;

        case STRING:
            variant.string = new string(*base.variant.string);
            break;

        case ARRAY:
            variant.array = new array(*base.variant.array);
            break;

        case OBJECT:
            variant.object = new object(*base.variant.object);
            break;
        }
    }
}

#endif

#ifdef SJSON_TEST

#include <gtest/gtest.h>

int main() {

    return 0;
}

#endif