#ifndef SJSON_HPP
#define SJSON_HPP

#include <istream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <map>
#include <memory>

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

            //idea: byte type for better msgpk compat
            typedef std::string string;
            typedef std::map<string, Node> object;
            typedef std::vector<Node> array;
            typedef double real;
            typedef long int integer;

            class coercion_invalid:std::exception {};
            class wrong_type : std::exception {};
            class json_invalid:std::exception {};


            // Parse from json
            static Node parse_from_istream(std::istream&);
            static Node from_file_path(std::string);
            static Node parse_from_string(std::string);
            static Node parse_from_string(const char*);

            void write_as_json(std::ostream&) const;
            string as_json_string() const;

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
            const string& as_string_reference() const;
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
            const array& as_array_reference() const;
            void set_array(std::vector<Node>);

            const object as_object() const;
            object& as_object_mut();
            const object& as_object_reference() const;
            void set_object(const object&);
            
        private:
            static string _array_to_string(array o);
            static string _object_to_string(object o);

            std::string _to_string() const;

            void _destroy_variant();

            void _copy_variant(const Node&);

            NodeType base_type = NONE;
            bool number_is_integer = false;

            // in progress: better way of doing this than using union
            class MultiTypeBase {
                public:
                    // needed for frees
                    virtual ~MultiTypeBase();
                    virtual NodeType get_type() const = 0;

                    virtual real as_real() const;
                    virtual real& as_real_mut();
                    
                    virtual integer as_int() const;
                    virtual integer& as_int_mut();

                    virtual string as_string() const;
                    virtual string& as_string_mut();
                    virtual const string& as_string_reference();

                    virtual array as_array() const;
                    virtual array& as_array_mut();
                    virtual const array& as_array_reference() const;

                    virtual object as_object() const;
                    virtual object& as_object_mut();
                    virtual const object& as_object_reference() const;

            };

            //todo
            MultiTypeBase* variant;

            class Mnull : public MultiTypeBase {
                public:
                    NodeType get_type() const override;
                    real as_real() const override;
                    integer as_int() const override;
            };

            class MInt : public MultiTypeBase {
                public:
                    MInt(const integer& value);

                    NodeType get_type() const override;
                    integer as_int() const override;
                    integer& as_int_mut() override;

                    real as_real() const override;

                    string as_string() const override;
                    array as_array() const override;

                private:
                    integer value;

            };

            class MReal : public MultiTypeBase {
                public:
                    MReal(const real& value);

                    NodeType get_type() const override;
                    real as_real() const override;
                    real& as_real_mut() override;

                    integer as_int() const override;

                    string as_string() const override;
                    array as_array() const override;
                private:
                    real value;
            };

            class MString : public MultiTypeBase {
                public:
                    MString(const string& content);

                    NodeType get_type() const override;
                    real as_real() const override;

                    integer as_int() const override;

                    string as_string() const override;
                    string& as_string_mut() override;
                    const string& as_string_reference() override;

                    //array as_array() const override;
                private:
                    string value;
            };

            class MArray : public MultiTypeBase {
                public:
                    MArray(const array& v);

                    NodeType get_type() const override;
                    string as_string() const override;

                    array as_array() const override;
                    array& as_array_mut() override;
                    const array& as_array_reference() const override;
                private:
                    array value;
            };

            class MObject : public MultiTypeBase {
                public:
                    MObject(const object& v);

                    NodeType get_type() const override;
                    string as_string() const override;

                    object as_object() const override;
                    object& as_object_mut() override;
                    const object& as_object_reference() const override;
                private:
                    object value;
            };

    };

    
    namespace json {
        class json_invalid : public std::exception {};
        class missing_delimeter : public json_invalid {};
        class missing_label : public json_invalid {};
        class wrong_delimeter : public json_invalid {};
        class wrong_label_type : public json_invalid {};
        class wrong_closer : public json_invalid {};
        class trailing_comma : public json_invalid {};
        class duplicate_label : public json_invalid {};
        class label_in_array: public json_invalid{};
        class missing_definition: public json_invalid{};
    }

} // namespace sjson


#endif


#ifdef SJSON_OBJECT
#include <cassert>

#ifdef SJSON_TEST
#include <iostream>
#define DEBUG_PRINT(X) std::cerr << "[----------] " << X << '\n'
#else
#define DEBUG_PRINT(X)
#endif

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
    static constexpr char SCIENTIFIC_NOTATION_LOWER = 'e';
    static constexpr char SCIENTIFIC_NOTATION_UPPPER = 'E';
    static constexpr char POSITIVE = '+';
    static constexpr char NEGATIVE = '-';
    static constexpr char JSON_NULL[] = "null";

/*

    todo: type coercions

*/

namespace sjson {

    // by default, all conversion funcs throw invalid
    // there's probably a way to make this more extensible with templates
    // also shorter

    //==================================================
    Node::MultiTypeBase::~MultiTypeBase() {}
    Node::real Node::MultiTypeBase::as_real() const {
        throw Node::coercion_invalid();
    }
    Node::real& Node::MultiTypeBase::as_real_mut() {
        throw Node::wrong_type();
    }

    //==================================================
    Node::integer Node::MultiTypeBase::as_int() const {
        throw Node::coercion_invalid();
    }
    Node::integer& Node::MultiTypeBase::as_int_mut() {
        throw Node::wrong_type();
    }

    //==================================================
    Node::string Node::MultiTypeBase::as_string() const {
        throw coercion_invalid();
    }
    Node::string& Node::MultiTypeBase::as_string_mut() {
        throw wrong_type();
    }
    const Node::string& Node::MultiTypeBase::as_string_reference() {
        throw wrong_type();
    }

    //==================================================
    Node::array Node::MultiTypeBase::as_array() const {
        throw coercion_invalid();
    }
    Node::array& Node::MultiTypeBase::as_array_mut() {
        throw wrong_type();
    }
    const Node::array& Node::MultiTypeBase::as_array_reference() const {
        throw wrong_type();
    }

    //==================================================
    Node::object Node::MultiTypeBase::as_object() const {
        throw coercion_invalid();
    }
    Node::object& Node::MultiTypeBase::as_object_mut() {
        throw wrong_type();
    }
    const Node::object& Node::MultiTypeBase::as_object_reference() const {
        throw wrong_type();
    }

    //= NULL =============================================
    NodeType Node::Mnull::get_type() const {
        return NONE;
    }
    Node::real Node::Mnull::as_real() const {
        return 0;
    }
    Node::integer Node::Mnull::as_int() const {
        return 0;
    }

    //= INTEGER ==========================================
    Node::MInt::MInt(const integer& v) : value(v) {}
    NodeType Node::MInt::get_type() const {
        return INTEGER;
    }
    Node::integer Node::MInt::as_int() const {
        return value;
    }
    Node::integer& Node::MInt::as_int_mut() {
        return value;
    }
    Node::real Node::MInt::as_real() const {
        return static_cast<real>(value);
    }
    Node::string Node::MInt::as_string() const {
        return std::to_string(value);
    }
    Node::array Node::MInt::as_array() const {
        return {value};
    }
    //= REAL =============================================
    Node::MReal::MReal(const real& v) : value(v) {}
    NodeType Node::MReal::get_type() const {
        return REAL;
    }
    Node::real Node::MReal::as_real() const {
        return value;
    }
    Node::real& Node::MReal::as_real_mut() {
        return value;
    }
    Node::integer Node::MReal::as_int() const {
        return static_cast<integer>(value);
    }
    Node::string Node::MReal::as_string() const {
        return std::to_string(value);
    }
    Node::array Node::MReal::as_array() const {
        return {value};
    }
    //= STRING ===========================================
    Node::MString::MString(const string& v) : value(v) {}
    NodeType Node::MString::get_type() const {
        return STRING;
    }
    Node::string Node::MString::as_string() const {
        return value;
    }
    Node::string& Node::MString::as_string_mut() {
        return value;
    }
    const Node::string& Node::MString::as_string_reference() {
        return value;
    }

    Node::integer Node::MString::as_int() const {
        try {
            return std::stol(value);
        }
        catch(std::invalid_argument& e) {
            throw coercion_invalid();
        }
    }
    Node::real Node::MString::as_real() const {
        try {
            return std::stof(value);
        }
        catch (std::invalid_argument& e) {
            throw coercion_invalid();
        }
    }
    //= ARRAY ============================================
    Node::MArray::MArray(const array& v) : value(v) {}
    NodeType Node::MArray::get_type() const  {
        return ARRAY;
    }
    Node::array Node::MArray::as_array() const {
        return value;
    }
    Node::array& Node::MArray::as_array_mut() {
        return value;
    }
    const Node::array& Node::MArray::as_array_reference() const {
        return value;
    }

    Node::string Node::MArray::as_string() const {
        return _array_to_string(value);
    }

    //todo: tostring
    //= OBJECT ===========================================
    Node::MObject::MObject(const object& v) : value(v) {}
    NodeType Node::MObject::get_type() const {
        return OBJECT;
    }
    Node::object Node::MObject::as_object() const {
        return value;
    }
    Node::object& Node::MObject::as_object_mut() {
        return value;
    }
    const Node::object& Node::MObject::as_object_reference() const {
        return value;
    }
    Node::string Node::MObject::as_string() const {
        return _object_to_string(value);
    }

    #define USE_SUBCLASS(CLASS,MEMBER_CLASS) typedef CLASS::MEMBER_CLASS MEMBER_CLASS

    USE_SUBCLASS(Node, object);
    USE_SUBCLASS(Node, array);
    USE_SUBCLASS(Node, string);
    USE_SUBCLASS(Node, integer);
    USE_SUBCLASS(Node, real);

    NodeType Node::get_type() const {
        return variant->get_type();
    }

    Node::Node() {
        variant = new Mnull();
    }

    Node::Node(const Node& base) {
        variant = base.variant;
    }

    Node::Node(const string& content) {
        variant = new MString(content);
    }

    Node::Node(const integer& content) {
        variant = new MInt(content);
    }

    Node::Node(const real& content) {
        variant = new MReal(content);
    }

    Node::Node(const array& content) {
        variant = new MArray(content);
    }

    Node::Node(const object& content) {
        variant = new MObject(content);
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
            break;
        }
    }
 
    Node::string Node::as_string() const {
        return variant->as_string();
    }

    Node::string& Node::as_string_mut() {
        return variant->as_string_mut();
    }

    const Node::string& Node::as_string_reference() const {
        return variant->as_string_reference();
    }

    Node::real Node::as_real() const {
        return variant->as_real();
    }

    Node::real& Node::as_real_mut() {
        return variant->as_real_mut();
    }

    Node::integer Node::as_int() const {
        return variant->as_int();
    }

    Node::integer& Node::as_int_mut() {
        return variant->as_int_mut();
    }

    const Node::array Node::as_array() const {
        return variant->as_array();
    }

    Node::array& Node::as_array_mut() {
        return variant->as_array_mut();
    }

    const Node::array& Node::as_array_reference() const {
        return variant->as_array_reference();
    }

    const object Node::as_object() const {
        return variant->as_object();
    }

    Node::object& Node::as_object_mut() {
        return variant->as_object_mut();
    }

    const Node::object& Node::as_object_reference() const {
        return variant->as_object_reference();
    }

    void Node::_destroy_variant() {
        delete variant;
        variant = nullptr;
    }

    void Node::_copy_variant(const Node& base) {
        _destroy_variant();
        
        switch (base.get_type())
        {
        case NONE:
            variant = new Mnull();
            break;

        case INTEGER:
            variant = new MInt(base.as_int());
            break;

        case REAL:
            variant = new MReal(base.as_real());
            break;

        case STRING:
            variant = new MString(base.as_string_reference());
            break;

        case ARRAY:
            variant = new MArray(base.as_array_reference());
            break;

        case OBJECT:
            variant = new MObject(base.as_object_reference());
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

    //todo: test this
    void write_as_json_recurse(const Node& node, int layer, std::ostream& stream, bool has_label = false, std::string label = "") {

        stream << std::string( layer, '\t' );

        if (has_label) {
            stream << QUOTE_OPEN << label << QUOTE_CLOSE << NAME_SPECIFIER;
        }

        switch (node.get_type())
        {
        case REAL:
            stream << node.as_real();
            break;

        case INTEGER:
            stream << node.as_int();
            break;
        
        case STRING:
            stream << QUOTE_OPEN << node.as_string() << QUOTE_CLOSE;
            break;
        
        case NONE:
            stream << JSON_NULL;
            break;

        case ARRAY:
            {
                stream << ARRAY_OPEN << '\n';

                unsigned int line_number = 0;
                const Node::array& ref = node.as_array_reference();
                for (const Node& c : ref) {
                    write_as_json_recurse(c, layer + 1, stream);

                    if ( (line_number++) < ref.size() ) stream << END_PHRASE << '\n';
                    else stream << '\n';
                    
                }

                stream << std::string( layer, '\t' ) << ARRAY_CLOSE;

            }
            break;

        case OBJECT:
            {
                stream << OBJECT_OPEN << '\n';
                unsigned int line_number = 0;
                const Node::object& ref = node.as_object_reference();

                for (const auto& pair : ref) {
                    write_as_json_recurse(pair.second, layer + 1, stream, true, pair.first);
                    if ( (line_number++) < ref.size() ) stream << ",\n";
                    else stream << '\n';
                }
                stream << std::string( layer, '\t' ) << OBJECT_CLOSE;
            }

        default:
            break;
        }
    }

    void Node::write_as_json(std::ostream& stream) const {
        write_as_json_recurse(*this, 0, stream);
    }

    Node::string Node::as_json_string() const {
        std::stringstream stream("");
        write_as_json(stream);
        return stream.str();
    }

    bool case_insensitive_equals(const char& c1, const char& c2) {
        return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
    }

    bool case_insensitive_equals(const std::string& s1, const std::string& s2) {
        if (s1.length() != s2.length()) return false;

        for (unsigned i = 0; i < s1.length(); i++) {
            if (!case_insensitive_equals(s1[i], s2[1])) return false;
        }

        return true;
    }

    

    bool json_is_numeric_char(const char& c) {
        switch (c)
        {
        case DECIMAL:
        case POSITIVE:
        case NEGATIVE:
        case SCIENTIFIC_NOTATION_LOWER:
        case SCIENTIFIC_NOTATION_UPPPER:

            return true;
        
        default:
            return std::isdigit( (unsigned char) c );
            break;
        }

        return false;
    }

    // NOTE: just because the string contents are detected as a certain type,
    // does NOT necessarily mean its valid as that type.
    // expects the format of the tokenizer (leading quotes included)
    NodeType detect_node_type_str(const std::string& str) {
        
        if (case_insensitive_equals(JSON_NULL, str)) {
            return NONE;
        }

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
                if (json_is_numeric_char(c)) {
                    if (c == '.') {
                        has_decimal = true;
                    }
                }
                else {
                    // I don't know of any other way to break out of this if branch
                    DEBUG_PRINT("Non numeric character (" << c << "), interpreting as string.");
                    goto NON_NUMERIC;
                }
            }
            return (has_decimal)? REAL:INTEGER;
        }
        NON_NUMERIC:

        return NONE;
    }

    Node parse_str_to_node(const std::string& str) {
        NodeType type = detect_node_type_str(str);
        Node a(str);
        if (type == STRING) {
            a.as_string_mut() = a.as_string_reference().substr(1);
            return a;
        }
        
        try {
            a.set_type(detect_node_type_str(str));
        }
        catch (Node::coercion_invalid& e) {
            // should throw or something.
            // just interperet as an str for now
        }
        
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
        
        if (stream.eof()) {
            return "";
        }

        bool string = false;
        bool escaped = false;

        std::stringstream collect;

        char c = stream.get();

        //get to next meaningful value
        while (isspace(c)) {stream.get(c);}

        if (json_is_delimeter(c)) return std::string(1,c);
        if (c == QUOTE_OPEN) {string = true;}

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

    Node Node::parse_from_istream(std::istream& stream) {

        class ParseHelper {
            public:
                void parse_token(const std::string& token) {

                    DEBUG_PRINT("Token: " << token);

                    if (token.length() == 1) {
                        const char c = token[0];

                        switch (c)
                        {
                        case OBJECT_OPEN:
                            push_object();
                            return;
                        case OBJECT_CLOSE:
                            if (!top_is_object()) throw json::wrong_closer();
                            pop();
                            return;
                        
                        case ARRAY_OPEN:
                            push_array();
                            return;
                        case ARRAY_CLOSE:
                            if (!top_is_array()) throw json::wrong_closer();
                            pop();
                            return;

                        case NAME_SPECIFIER:
                            current.set_label_as_node();
                            return;
                        
                        case END_PHRASE:
                            append_to_top(current);
                            current = StatementBuilder();
                            return;

                        default:
                            break;
                        }
                    }
                    
                    // must be definition
                    if (current.is_node_assigned()) {
                        throw json::missing_delimeter();
                    }


                    Node derived = parse_str_to_node(token);
                    current.set_node(derived);
                    
                }

                bool done() const {
                    return _done;
                }

                Node get_root() const {
                    if (done()) {
                        return root;
                    }
                    else {
                        DEBUG_PRINT( __LINE__ << ": STACK NOT EMPTY!!!" );
                        assert(false);
                        return root;
                    }
                }

            private:

                // Class represents partial statements
                class StatementBuilder {
                    public:

                        class no_label : public std::exception {};
                        class no_token : public std::exception {};

                        bool is_label_assigned() const {
                            return label_assigned;
                        }
                        void set_label(const Node::string base) {
                            label = base;
                            label_assigned = true;
                        }
                        const Node::string& get_label() const {
                            if (label_assigned) {
                                return label;
                            }
                            else {
                                throw no_label();
                            }
                        }

                        bool is_node_assigned() const {
                            return token_assigned;
                        }
                        void set_node(const Node base) {
                            token = base;
                            token_assigned = true;
                        }
                        Node& get_node_mut() {
                            if (token_assigned) {
                                return token;
                            }
                            else {
                                throw no_token();
                            }
                        }
                        const Node& get_node() const {
                            if (token_assigned) {
                                return token;
                            }
                            else {
                                throw no_token();
                            }
                        }

                        void set_label_as_node() {
                            if (token.get_type() == STRING) {
                                set_label(token.as_string_reference());
                                token = Node();
                                token_assigned = false;
                            }
                            else {
                                //throw
                            }
                        }
                    private:
                        Node::string label;
                        bool label_assigned = false;

                        Node token;
                        bool token_assigned = false;
                };

                bool top_is_array() {
                    return top_type() == ARRAY;
                }

                bool top_is_object() {
                    return top_type() == OBJECT;
                }

                void push_array() {
                    Node dummy = Node(Node::array());
                    current.set_node(dummy);
                    push_current();
                }

                void push_object() {
                    Node dummy = Node(Node::object());
                    current.set_node(dummy);
                    push_current();
                }

                void push_current() {
                    nest_stack.push(current);
                    current = StatementBuilder();
                }

                void pop() {
                    if (current.is_node_assigned()) {
                        append_to_top(current);
                    }
                    #if !JSON_ALLOW_TRAILING_COMMA
                    else {
                        throw json::trailing_comma();
                    }
                    #endif
                    StatementBuilder popped = top();
                    nest_stack.pop();
                    if (nest_stack.size() == 0) {
                        root = popped.get_node();
                        _done = true;
                        return;
                    }
                    else {
                        current = popped;
                    }
                    
                    //append_to_top(popped);
                }

                void append_to_top(const StatementBuilder& st) {
                    if (top_is_object()) {
                        try {
                            const Node::string label = st.get_label();
                            Node::object& map = top_mut().get_node_mut().as_object_mut();
                            if (map.count(label) > 0) throw json::duplicate_label();
                            map[label] = st.get_node();
                        }
                        catch (StatementBuilder::no_label& e) {
                            throw json::missing_label();
                        }
                    }
                    else if (top_is_array()) {
                        if (st.is_label_assigned()) {
                            throw json::label_in_array();
                        }
                        try {
                            Node::array& array = top_mut().get_node_mut().as_array_mut();
                            array.push_back(st.get_node());
                        }
                        catch (StatementBuilder::no_token& e) {
                            throw json::missing_definition();
                        }
                    }
                }
                
                NodeType top_type() const {
                    if (!done()) {
                        return top().get_node().get_type();
                    }
                    else {
                        return root.get_type();
                    }
                }

                StatementBuilder& top_mut() {
                    return nest_stack.top();
                }

                const StatementBuilder& top() const {
                    return nest_stack.top();
                }

                std::stack<StatementBuilder> nest_stack;
                StatementBuilder current;

                // only valid after everything is popped
                bool _done = false;
                Node root;
        };

        ParseHelper helper;

        //pushes initial object
        helper.parse_token(get_next_json_token(stream));

        while (!stream.eof() && !helper.done())
        {
            helper.parse_token(get_next_json_token(stream));
        }
        
        return Node(helper.get_root());

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

TEST(multitype, to_json) {
    const Node::array node_array = {Node("my string"), Node((Node::integer)2)};
    const Node array_node(node_array);
    const Node::string test_str = "this is a string";
    const Node::integer test_number = 64;
    const Node::real test_real = 33.2;

    const Node obj_node = Node((Node::object)
        {
            {"my_array", Node(node_array)}, 
            {"my_string", Node(test_str)},
            {"my_real" , Node(test_real)},
            {"my_int", Node(test_number)}
        });

    std::string as_json = obj_node.as_json_string();

    DEBUG_PRINT('\n' << as_json);
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

TEST(json_parser, type_detection) {
    {
        const std::string str = "\"string or ... something";
        EXPECT_EQ(sjson::detect_node_type_str(str), sjson::STRING);
    }
    {
        const std::string str = "262.848";
        EXPECT_EQ(sjson::detect_node_type_str(str), sjson::REAL);
    }
    {
        const std::string str = "44866";
        EXPECT_EQ(sjson::detect_node_type_str(str), sjson::INTEGER);
    }
    {
        const std::string str = "nULl";
        EXPECT_EQ(sjson::detect_node_type_str(str), sjson::NONE);
    }
}

TEST(json_parser, string_translation) {
    {
        const std::string str = "\"string or ... something";
        EXPECT_EQ(sjson::parse_str_to_node(str).as_string(), str.substr(1));
    }
    {
        const std::string str = "262.848";
        const float test_value = 262.848;
        EXPECT_EQ(sjson::parse_str_to_node(str).as_real(), test_value);
    }
    {
        const std::string str = "44866";
        const long int test_value = 44866;
        EXPECT_EQ(sjson::parse_str_to_node(str).as_int(), test_value);
    }
}

// todo: write json export for debugging visuals

TEST(json_parser, tokenizer) {
    const std::string test_str = "{fortnite ,balls  \t \n\n  : \"i'm   }\"gay I,like]boys";
    auto stream_proto = std::istringstream(test_str);
    std::istream& stream = stream_proto;

    ASSERT_EQ(sjson::get_next_json_token(stream),"{");
    ASSERT_EQ(sjson::get_next_json_token(stream),"fortnite");
    ASSERT_EQ(sjson::get_next_json_token(stream),",");
    ASSERT_EQ(sjson::get_next_json_token(stream),"balls");
    ASSERT_EQ(sjson::get_next_json_token(stream),":");
    ASSERT_EQ(sjson::get_next_json_token(stream),"\"i'm   }");
    ASSERT_EQ(sjson::get_next_json_token(stream),"gay");
    ASSERT_EQ(sjson::get_next_json_token(stream),"I");
    ASSERT_EQ(sjson::get_next_json_token(stream),",");
    ASSERT_EQ(sjson::get_next_json_token(stream),"like");
    ASSERT_EQ(sjson::get_next_json_token(stream),"]");
    ASSERT_EQ(sjson::get_next_json_token(stream),"boys");
}

TEST(json_parser, basic_file) {
    const std::string test_str = "{\"basic\":123}";
    auto stream_proto = std::istringstream(test_str);
    std::istream& stream = stream_proto;
    
    Node parsed = sjson::Node::parse_from_istream(stream);
    DEBUG_PRINT("PARSE RESULT:\n" << parsed.as_json_string());

    ASSERT_EQ(parsed.get_type(), sjson::OBJECT);
    ASSERT_TRUE(parsed.as_object().count("basic") > 0);
    Node basic = parsed.as_object().at("basic");
    EXPECT_EQ(basic.get_type(), sjson::INTEGER);
    EXPECT_EQ(basic.as_int(), 123);

}

#include <fstream>
// this crashes, todo
TEST(json_parser, all_types) {
    std::ifstream stream("tests/all_types.json");
    Node parsed = sjson::Node::parse_from_istream(stream);
    DEBUG_PRINT("PARSE RESULT:\n" << parsed.as_json_string());
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

#endif