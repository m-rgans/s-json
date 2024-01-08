#ifndef SJSON_HPP
#define SJSON_HPP

#include <istream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <map>

// temporary so vs code isnt a pain about it
#define SJSON_OBJECT
#define SJSON_TEST

// Set this to zero to treat json with trailing commas as invalid.
#define JSON_ALLOW_TRAILING_COMMA true

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
            class wrong_type : std::exception {};
            class json_invalid:std::exception {};


            // Parse from json
            static Node parse_from_istream(std::istream);
            static Node from_file_path(std::string);
            static Node parse_from_string(std::string);
            static Node parse_from_string(const char*);

            //todo: messagepack

            Node& operator=(const Node&);

            // todo: own comparison operators
            Node();
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
            string& as_string_mut();
            void set_string(string);

            //Number
            real as_real() const;
            real& as_real_mut();

            integer as_int() const;
            integer& as_int_mut();

            void set_number(real);
            void set_number(integer);

            //Array
            const array as_array() const;
            array& as_array_mut();
            void set_array(std::vector<Node>);

            const object as_object() const;
            object& as_object_mut();
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
#include <cassert>

#ifdef SJSON_TEST
#define DEBUG_PRINT(X) std::cerr << "[----------] " << X << '\n'
#else
#define DEBUG_PRINT(X)
#endif

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

    NodeType Node::get_type() const {
        return base_type;
    }

    Node::Node() {
        base_type = NONE;
        variant = { nullptr };
    }

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
        if (&other == this) {
            // dont overwrite with self, that will crash
            return *this;
        }
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
        case NONE:
            *this = Node();
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

    Node::string& Node::as_string_mut() {
        if (base_type != STRING) {
            throw wrong_type();
        }
        return *variant.vstring;
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

        // Should be unreachable
        assert(false);
        return 0.0; // remove a warning
    }

    Node::real& Node::as_real_mut() {
        if (base_type != REAL) {
            throw wrong_type();
        }
        return *variant.vreal;
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

        // unreachable
        assert(false);
        return 0;
    }

    Node::integer& Node::as_int_mut() {
        if (base_type != INTEGER) {
            throw wrong_type();
        }
        return *variant.vinteger;
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

    Node::array& Node::as_array_mut() {
        if (base_type != ARRAY) {
            throw wrong_type();
        }
        return *variant.varray;
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

    Node::object& Node::as_object_mut() {
        if (base_type != ARRAY) {
            throw wrong_type();
        }
        return *variant.vobject;
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

    // NOTE: just because the string contents are detected as a certain type,
    // does NOT necessarily mean its valid as that type.
    NodeType detect_node_type_str(const std::string& str) {
        const char first_char = str[0];
        if (first_char == '"') {
            return STRING;
        }
        else if (first_char == '{') {
            return OBJECT;
        }
        else if (first_char == '[') {
            return ARRAY;
        }
        // try numeric
        else {
            bool has_decimal = false;
            for (const char& c : str) {
                if (!isdigit(c)) {
                    if (c != '+' && c != '-' && c != 'E' && c != 'e') {
                        break;
                    }
                    else if (c == '.') {
                        has_decimal = true;
                    }
                }
            }
        }
        return NONE;
    }

    Node parse_str_to_node(const std::string& str) {
        Node a(str);
        a.set_type(detect_node_type_str(str));
        return a;
    }

    char escape_to_raw(const char& code) {
        switch (code)
        {
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case '"':
            return '"';
        case '\\':
            return '\\';
        default:
            return '?';
            break;
        }

        //shouldnt happen
        return '#';
    }

    static constexpr char QUOTE_OPEN = '"';
        static constexpr char QUOTE_CLOSE = '"';

        static constexpr char OBJECT_OPEN = '{';
        static constexpr char OBJECT_CLOSE = '}';

        static constexpr char ARRAY_OPEN = '[';
        static constexpr char ARRAY_CLOSE = ']';

        static constexpr char ESCAPE = '\\';

        static constexpr char NAME_SPECIFIER = ':';
        static constexpr char END_PHRASE = ',';

        static constexpr char DECIMAL = '.';

    // This function does not include
    // trailing quotes in the string it returns.
    // First is left in for signaling purposes

    bool json_is_delimeter(const char& c) {
        switch (c)
        {
        case OBJECT_OPEN:
        case OBJECT_CLOSE:
        case ARRAY_OPEN:
        case ARRAY_CLOSE:
        case NAME_SPECIFIER:
        case END_PHRASE:
            return true;
            break;
        default:
            return false;
        }
    }

    std::string get_next_json_token(std::istream& stream) {
        bool string = false;
        bool escaped = false;

        std::stringstream collect;

        char c = stream.get();

        //get to next value
        while (isspace(c)) {c = stream.get();}

        // switch off first character
        switch (c)
        {
        case QUOTE_OPEN:
            string = true;
            break;
        
        // These are all their own token, so return them seperately
        case OBJECT_OPEN:
        case OBJECT_CLOSE:
        case ARRAY_OPEN:
        case ARRAY_CLOSE:
        case NAME_SPECIFIER:
        case END_PHRASE:
            return std::string(&c);

        default:
            break;
        }

        collect << c;

        while (stream.get(c))
        {
            if (string) {
                if (escaped) {
                    collect << escape_to_raw(c);
                    continue;
                }
                else if (c == ESCAPE) {
                    escaped = true;
                    continue;
                }
                else if (c == QUOTE_CLOSE) {
                    break;
                }
                else {
                    collect << c;
                }
            }
            // build until space
            else {
                if (isspace(c)) break;
                if (json_is_delimeter(c)) {
                    stream.putback(c);
                    break;
                }
                collect << c;
            }
        }
        return collect.str();
    }



    class JsonParseHelper {
        public:

            typedef Node::string string;

            // using these to make localization easier
            

            void parse_stream(std::istream& stream) {
                do {
                }
                while (!nesting.empty());
            }

        private:
            std::stack<Node> nesting;

    };

    Node Node::parse_from_istream(std::istream stream) {

    }

}

#endif

#ifdef SJSON_TEST



#include <gtest/gtest.h>
using sjson::Node;



TEST(parse_helper, init) {

}

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

/*
TEST(json_parsing, clean_string) {
    const std::string sample_str = "ddd  : \"m  \" pp,";
    const std::string cleaned_str_correct = "ddd:\"m  \"pp";
    const std::string cleaned = sjson::clean_json_clause(sample_str);
    ASSERT_EQ(cleaned, cleaned_str_correct);
}


TEST(json_parsing, seperate_string) {

    {
        const std::string sample = "   dfd : abcde.f  f ";
        const std::string correct_clause1 = "dfd";
        const std::string correct_clause2 = "abcde.ff";

        sjson::SeperatedClause test(sample);

        ASSERT_EQ(test.name, correct_clause1);
        ASSERT_EQ(test.content, correct_clause2);
    }

    {
        const std::string sample_noname = "  443,33  2";
        const std::string sample_noname_correct = "443,332";
        sjson::SeperatedClause test2(sample_noname);
        ASSERT_FALSE(test2.has_name);
        ASSERT_EQ(test2.content, sample_noname_correct);
    }

    {
        const std::string sample_collection = " big_thing:{abs:true;b:20}";
        const std::string sample_collection_name = "big_thing";

        sjson::SeperatedClause test(sample_collection);

        ASSERT_TRUE(test.is_collection);
        ASSERT_EQ(test.name, sample_collection_name);
    }
}


TEST(json_parsing, type_detection) {
    //numeric
    {
        const std::string int_example = "name:1234";
        sjson::SeperatedClause test(int_example);
        ASSERT_EQ(test.detected_type, sjson::INTEGER);
    }
    {
        const std::string int_example = "name:123.34";
        sjson::SeperatedClause test(int_example);
        ASSERT_EQ(test.detected_type, sjson::REAL);
    }

    //string
    {
        const std::string int_example = "name:\"fucking deborah\"";
        sjson::SeperatedClause test(int_example);
        DEBUG_PRINT("Name:" << test.name);
        DEBUG_PRINT("Content:" << test.content);
        ASSERT_EQ(test.detected_type, sjson::STRING);
    }

    //array
    {
        DEBUG_PRINT("Array detection");
        const std::string int_example = "name:[1,2,4,5,6]";
        sjson::SeperatedClause test(int_example);
        DEBUG_PRINT("Name:" << test.name);
        DEBUG_PRINT("Content:" << test.content);
        ASSERT_TRUE(test.is_collection);
        ASSERT_EQ(test.detected_type, sjson::ARRAY);
    }

    //object
    {
        DEBUG_PRINT("Object Detection");
        const std::string int_example = "name:{first:\"jay\", last: \"biz\"}";
        sjson::SeperatedClause test(int_example);
        DEBUG_PRINT("Name:" << test.name);
        DEBUG_PRINT("Content:" << test.content);
        ASSERT_EQ(test.detected_type, sjson::OBJECT);
    }
    
}
*/

TEST(json_parsing, tokenizer) {
    const std::string test_str = "{fortnite ,balls  \t \n\n  : \"i'm   }\"gay I,like]boys";
    auto stream_proto = std::istringstream(test_str);
    std::istream& stream = stream_proto;

    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),"{");
    ASSERT_EQ(sjson::get_next_json_token(stream),"fortnite");
    ASSERT_EQ(sjson::get_next_json_token(stream),",");
    ASSERT_EQ(sjson::get_next_json_token(stream),"balls");
    ASSERT_EQ(sjson::get_next_json_token(stream),":");
    ASSERT_EQ(sjson::get_next_json_token(stream),"\"i'm   }");
    ASSERT_EQ(sjson::get_next_json_token(stream),"gay");
    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),"I");
    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),",");
    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),"like");
    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),"]");
    DEBUG_PRINT(__LINE__);
    ASSERT_EQ(sjson::get_next_json_token(stream),"boys");
    DEBUG_PRINT(__LINE__);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

#endif