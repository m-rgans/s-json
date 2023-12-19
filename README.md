# s-json
Single file json parser and interaction class

## Classes

### Node (Name subject to change)
#### Overview
Node represents a javascript type object. Unlike javascript, however, Nodes differentiate between integral and floating point values. This is because these kinds of values have meaningfully different ranges in C++. When parsing a json file, a number without a decimal point will be treated as integral, and a number with as a float. All floats are `double`, and all integers are `long int`.

The form that a Node takes is referred to as its base type. This type can be queried using `get_type()` and can be changed using `set_type(const NodeType&)`. The types are in the NodeType enum.

#### Coercion

Type coercion happens whenever a Node is attempted to be used in a form other than its base type. This happens either when its type is changed, or when the user attempts to interpret it differently using `as_int`, `as_string`, etc. An invalid coercion will throw an instance of `coercion_invalid`.

## Testing
Since the library isn't yet able to actually parse json files, the only testing that can be done is on the Node class. `make test` should compile and run the tests.

## Todo
At this point, most of the functionality.
- Read JSON files
- Save JSON files
- Read MessagePack files
- Save Messagepack files
- More thorough testing.