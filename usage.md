
# Usage

## Node breakdown

A node is the representation of an sml document. It contains other nodes that are its children, or string content.

```c++
Node n;
n.tagName; // the name of the tag within the sml document
n.content; // the string content of the tag if it contains any
n.children; // a list of ordered nodes that are this nodes children
n.attributes; // map of attributes which are named strings 
```

## Frequent operations

### Parsing from a stream

```c++
std::ifstream ss{ "../smlexample/test-gui.gml" };

sml::Node node;
ss >> node;
```
or 

```c++
std::ifstream ss{ "../smlexample/test-gui.gml" };

sml::Node node = sml::parse(ss);
```


### Parsing from a string

```c++
std::string s{ "<tag> My Content </tag>" };

sml::Node node = sml::parse(s);
```

### Writing to a string

```c++
std::string s{ "<tag> My Content </tag>" };

sml::Node node = sml::parse(s);

// writing
std::string out;
sml::write(node, out);
```

### Writing to a stream

```c++
std::string s{ "<tag> My Content </tag>" };

sml::Node node = sml::parse(s);

// writing
std::stringstream out;
sml::write(node, out);
```

## Exceptions

Parser errors are handled through the sml::ParserError class. This exception type is thrown when a parser error occurs. Once the error has been handled the parser object will be in an unspecified state and will need to be reset using `.reset()` to recover from the error. 

## Generating Documentation

Documentation is created using doxygen. Install doxygen and run it using the Doxyfile at the root of this repo. This will generate documentation for the project within the `docs` directory.