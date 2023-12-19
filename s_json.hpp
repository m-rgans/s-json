#ifndef SJSON_HPP
#define SJSON_HPP

#include <istream>
#include <sstream>
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

            // todo: own comparison operators

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
            static string _array_to_string(array o);
            static string _object_to_string(object o);

            std::string _to_string() const;

            void _destroy_variant();

            void _copy_variant(const Node&);

            NodeType base_type = NONE;
            bool number_is_integer = false;

            // more troublesome, but saves on memory
            // which is important for huge json files
            union {
                array* varray;
                object* vobject;

                string* vstring;

                real* vreal;
                integer* vinteger;
            } variant;

    };

    

} // namespace sjson


#endif


#ifdef SJSON_OBJECT


/*

    todo: type coercions

*/

namespace sjson {

    #define USE_SUBCLASS(CLASS,MEMBER_CLASS) typedef CLASS::MEMBER_CLASS MEMBER_CLASS

    USE_SUBCLASS(Node, object);
    USE_SUBCLASS(Node, array);
    USE_SUBCLASS(Node, string);
    USE_SUBCLASS(Node, integer);
    USE_SUBCLASS(Node, real);

    Node::Node(const Node& base) {
        _copy_variant(base);
    }

    Node::Node(const string& content) {
        base_type = STRING;
        variant.vstring = new string(content);
    }

    Node::Node(const integer& content) {
        base_type = INTEGER;
        variant.vinteger = new integer(content);
    }

    Node::Node(const real& content) {
        base_type = REAL;
        variant.vreal = new real(content);
    }

    Node::Node(const array& content) {
        base_type = ARRAY;
        variant.varray = new array(content);
    }

    Node::Node(const object& content) {
        base_type = OBJECT;
        variant.vobject = new object(content);
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
        switch (base_type)
        {
        case NONE:
            return "";

        case INTEGER:
            return std::to_string(*variant.vinteger);
            break;

        case REAL:
            return std::to_string(*variant.vreal);
            break;

        case STRING:
            return *variant.vstring;
            break;

        case ARRAY:
            return _array_to_string(*variant.varray);
            break;

        case OBJECT:
            return _object_to_string(*variant.vobject);
            break;
        }
    }

    Node::real Node::as_real() const {
        switch (base_type)
        {
        case NONE:
            return 0.0;

        case INTEGER:
            return (double)*variant.vinteger;
            break;

        case REAL:
            return *variant.vreal;
            break;

        case STRING:
            {
                try {
                    std::stof(*variant.vstring);
                }
                catch (std::invalid_argument) {
                    throw coercion_invalid();
                }
            }
            break;

        case ARRAY:
        case OBJECT:
        default:
            throw coercion_invalid();
            break;
        }
    }

    Node::integer Node::as_int() const {
        switch (base_type)
        {
        case NONE:
            return 0;

        case INTEGER:
            return *variant.vinteger;
            break;

        case REAL:
            return (integer)*variant.vreal;
            break;

        case STRING:
            {
                try {
                    std::stoi(*variant.vstring);
                }
                catch (std::invalid_argument) {
                    throw coercion_invalid();
                }
            }
            break;

        case ARRAY:
        case OBJECT:
        default:
            throw coercion_invalid();
            break;
        }
    }

    const Node::array Node::as_array() const {
        switch (base_type)
        {
        // Return a single element array
        case NONE:
        case INTEGER:
        case REAL:
            return {*this};
            break;
        
        case ARRAY:
            return *variant.varray;
        
        case OBJECT:
        default:
            throw coercion_invalid();
        }
    }

    const object Node::as_object() const {
        switch (base_type)
        {
            case OBJECT:
                return *variant.vobject;
            case NONE:
            case INTEGER:
            case REAL:
            case ARRAY:
            default:
                throw coercion_invalid();
        }
    }

    void Node::_destroy_variant() {
        switch (base_type)
        {
        case NONE:
            break;

        case INTEGER:
            delete variant.vinteger;
            break;

        case REAL:
            delete variant.vreal;
            break;

        case STRING:
            delete variant.vstring;
            break;

        case ARRAY:
            delete variant.varray;
            break;

        case OBJECT:
            delete variant.vobject;
            break;
        }
    }

    void Node::_copy_variant(const Node& base) {
        _destroy_variant();
        base_type = base.base_type;
        switch (base_type)
        {
        case NONE:
            break;

        case INTEGER:
            variant.vinteger = new integer(*base.variant.vinteger);
            break;

        case REAL:
            variant.vreal = new real(*base.variant.vreal);
            break;

        case STRING:
            variant.vstring = new string(*base.variant.vstring);
            break;

        case ARRAY:
            variant.varray = new array(*base.variant.varray);
            break;

        case OBJECT:
            variant.vobject = new object(*base.variant.vobject);
            break;
        }
    }

    Node::string Node::_to_string() const {
        return as_string();
    }

    Node::string Node::_object_to_string(object e) {
        std::stringstream stream;
        stream << "Object:{";
        for (const auto& pair:e) {
            stream << '"' << pair.first << "\":";
            stream << pair.second._to_string() << ",";
        }
        stream << "}";
        return stream.str();
    }
    
    Node::string Node::_array_to_string(array a) {
        std::stringstream stream;
        stream << "[";
        for (const Node& node : a) {
            stream << node._to_string() << ",";
        }
        stream << "]";
        return stream.str();
    }

    Node Node::parse_from_istream(std::istream stream) {

    }

}

#endif

#ifdef SJSON_TEST

#include <gtest/gtest.h>
using sjson::Node;

TEST(multitype, create_and_equivocate) {
    Node a((long int)10);
    ASSERT_EQ(a.as_int(), 10);
    ASSERT_EQ(a.as_real(), 10.0);

    Node b(7.4);
    ASSERT_EQ(b.as_real(), 7.4);

    Node c("this is a string");
    ASSERT_EQ(c.as_string(), "this is a string");

    Node d(Node::array({
        Node(10l),
        Node("String"),
    }));

    Node::array res = d.as_array();
    ASSERT_EQ(res[0].as_int(), 10);
    ASSERT_EQ(res[1].as_string(), "String");
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

#endif