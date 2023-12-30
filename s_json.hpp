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

    // returns a negative number on failure
    int get_next_non_space(const std::string& str, const unsigned int& start_position) {

        for (int i = start_position; i < str.length(); i++) {
            if (!std::isspace(str[i])) {
                return i;
            }
        }

        return -1;
    }

    //obsolete
    std::string clean_json_clause(const std::string& str) {
        std::stringstream collect;
        
        bool in_string = false;
        bool escaped = false; // allow for escaped quotes
        for (const char& c : str) {
            if (c == '"' && !escaped) {
                in_string = !in_string;
                collect << c;
                continue;
            }
            if (in_string) {
                if (c == '\\') {
                    escaped = true;
                    continue;
                }
                else {
                    collect << c;
                }
            }
            else {
                if (!std::isspace(c)) {
                    collect << c;
                }
            }
        }

        // remove trailing comma
        std::string ret = collect.str();
        if (ret[ret.length() - 1] == ',') {
            return ret.substr(0,ret.length() - 1);
        }
        else {
            return ret;
        }

    }

    // mixed recursion is the way to do it.
    // left off here.

    

    struct SeperatedClause {
        bool has_name = false;
        bool is_collection = false;
        NodeType detected_type = NONE;
        std::string name;
        std::string content;

        //returns a none if invalid
        Node try_parse() {

            try
            {
                Node node(content);
                node.set_type(detect_type());
            }
            catch(const Node::coercion_invalid& e) {
                
            }

            return Node();            

        }
        NodeType detect_type() {
            if (content[0] == '"') {
                return STRING;
            }

            if (is_collection) {
                if (content[0] == '{') {
                    return OBJECT;
                }
                else {
                    return ARRAY;
                }
            }

            // try numerics

            {
                bool decimal_point = false;
                for (const char& c : content) {
                    if (!isdigit(c) && c != '.') {
                        goto NOT_NUMERIC; // im sorry
                    }
                    if (c == '.') {
                        decimal_point = true;
                    }
                }
                if (decimal_point) {
                    return REAL;
                }
                else {
                    return INTEGER;
                }
            }
            NOT_NUMERIC:

            return NONE;
        }
        SeperatedClause(const std::string& str) {

            std::stringstream collect;

            bool in_string = false;
            bool escaped = false;
            for (const char& c : str) {
                // signifigant quote
                if (c == '"' && !escaped) {
                    in_string = !in_string;
                    collect << c;
                }
                else if (in_string && c == '\\') {
                    escaped = true;
                }
                else if (in_string) {
                    // mutate char for escape if needed
                    char fixed = c;
                    if (escaped) {
                        // fixed = escape_to_raw(c);
                        escaped = false;
                    }
                    
                    collect << fixed;
                }
                else if (c == '[' || c == '{') {
                    // let the parse function handle this one
                    is_collection = true;
                    collect << c;
                    break;
                }
                else if (c == ':') {
                    name = collect.str();
                    has_name = true;
                    collect.str(std::string());
                }
                else {
                    if (!std::isspace(c)) {
                        collect << c;
                    }
                }
            }

            content = collect.str();

            detected_type = detect_type();
        }
    };

    //NOTE: just because the string contents are detected as a certain type,
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

    Node Node::parse_from_istream(std::istream stream) {
        /*
            for each char in stream:
                if char is { -> push object
                if char is } -> pop object

                if char is " set instring flag to true
                if escaped is on, 
        */

       // todo:
       // * comment support
       // * trailing commas (should already be supported, but untested)
       // im not supporting unclosed objects thats ambiguous

        class ParseHelper {
            public:


                void parse(std::istream& stream) {

                }

                Node get_rood() {
                    return root;
                }
            private:
                
                void apply_name() {
                    Node filler;
                    if (nest_stack.top().get_type() == OBJECT) {
                        nest_stack.top().as_object_mut()[label] = filler;
                    }
                    // DONT CLEAR THE NAME! ITS NEEDED FOR COMMIT!
                }

                // This reference must point to the 
                // node as it exists within root, not 
                Node& add_node_assume_next(const Node& node) {
                    Node& top = nest_stack.top();
                    if (top.get_type() == OBJECT) {
                        top.as_object_mut()[label] = node;
                    }
                    else if (top.get_type() == ARRAY) {
                        top.as_array_mut().push_back(node);
                    }
                    else {
                        assert(false);
                    }
                }

                void push_array() {
                    Node array(Node::array());
                    add_node_assume_next(array);
                }

                void commit_collected() {
                    Node& top = nest_stack.top();
                    Node created = parse_str_to_node(collect.str());
                    if (top.get_type() == OBJECT) {
                        top.as_object_mut()[label] = created;
                    }
                    else if (top.get_type() == ARRAY) {
                        
                        top.as_array_mut().push_back(created);
                    }

                    label = "";
                    collect.str("");
                }

                void parsechar(const char& c) {
                    //todo
                }

                Node root;
                // This represents a serious
                // problem.
                /*
                    these nodes are getting shuffled around
                    and reallocd in the vectors that hold them.
                    these references simply wont survive.

                    approaches:
                        - emplace collections when they are popped, keeping them
                          on the stack.

                          This may work as it will always be the next item when popped.
                          problem is that they might have labels, which would
                          have been destroyed.
                          
                          parallel label stack, perhaps?
                */
                std::stack<Node&> nest_stack;

                std::string label;
                std::stringstream collect;

                bool instring = false;
                bool escaped = false;
        } helper;

        Node root;

        // These are all owned by root
        // maybe this should be a nested class or something
        std::stack<Node&> nesting;
        std::stringstream collect;

        std::string label = "";

        bool instring = false;
        bool escaped = false;

        while (char c = stream.get()) {

            if (instring) {
                switch (c)
                {
                case '\"':
                    if (!escaped) {
                        instring = false;
                    }
                    else {
                        escaped = false;
                    }
                    break;
                
                case '\\':
                    if (!escaped) {
                        escaped = true;
                    }
                    else {
                        escaped = false;
                    }
                    break;

                default:
                    escaped = false;
                }

                collect << c;
            }
            else {
                switch (c) {
                    case '{':
                        //push object
                        break;
                    case '}':
                        //parse collect
                        //pop object
                        break;
                    
                    case '[':
                        //push array
                    case ']':
                        //parse collect
                        //pop array
                    
                    case ':':
                        // set name
                        {
                            label = collect.str();
                            //todo: this isn't gonna work without
                            //parsing collect as an str

                            // clear collect
                            collect.str("");
                        }
                        break;
                    
                    case ',':
                        //parse object and return
                    
                    case ' ':
                        // don't care
                        break;

                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                    case '.':
                    case 'E':
                    case 'e':
                    case '+':
                    case '-':
                        collect << c;
                    
                    default:
                        throw Node::json_invalid();
                        break;
                }
            }

            switch (c)
            {
            case '{':
                //push object
                continue;
            case '}':
                
                continue;
            
            case '[':
                //push array
                continue;
            case ']':
                //pop array
                continue;

            case '\\':
                if (!escaped) {
                    escaped = true;
                }

            case '\"':
                instring = !instring;
                
            case ',':
                if (!escaped && !instring) {
                    Node& top = nesting.top();
                    if (top.get_type() == OBJECT) {

                    }
                    else if (top.get_type() == ARRAY) {

                    }
                    else {
                        //assert false;
                    }
                }
            break;

            case ':':
                {
                    label = collect.str();
                    collect.str("");
                }
                break;

            case ' ':
                break;
            
            default:
                collect << c;
                break;
            }
        }
    }



}

#endif

#ifdef SJSON_TEST

#define DEBUG_PRINT(X) std::cerr << "[----------] " << X << '\n'

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

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

#endif